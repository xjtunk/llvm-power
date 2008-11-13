/* exec-STM.cpp - Simple(r) Timing Model */
/*
 * Copyright (C) 2007 by Gabriel H. Loh and the Georgia Tech Research Corporation
 * Atlanta, GA  30332-0415
 * All Rights Reserved.
 * 
 * THIS IS A LEGAL DOCUMENT BY DOWNLOADING ZESTO, YOU ARE AGREEING TO THESE
 * TERMS AND CONDITIONS.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * NOTE: Portions of this release are directly derived from the SimpleScalar
 * Toolset (property of SimpleScalar LLC), and as such, those portions are
 * bound by the corresponding legal terms and conditions.  All source files
 * derived directly or in part from the SimpleScalar Toolset bear the original
 * user agreement.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the Georgia Tech Research Coporation nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * 4. Zesto is distributed freely for commercial and non-commercial use.  Note,
 * however, that the portions derived from the SimpleScalar Toolset are bound
 * by the terms and agreements set forth by SimpleScalar, LLC.  In particular:
 * 
 *   "Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 *   downloaded, compiled, executed, copied, and modified solely for nonprofit,
 *   educational, noncommercial research, and noncommercial scholarship
 *   purposes provided that this notice in its entirety accompanies all copies.
 *   Copies of the modified software can be delivered to persons who use it
 *   solely for nonprofit, educational, noncommercial research, and
 *   noncommercial scholarship purposes provided that this notice in its
 *   entirety accompanies all copies."
 * 
 * User is responsible for reading and adhering to the terms set forth by
 * SimpleScalar, LLC where appropriate.
 * 
 * 5. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 6. Noncommercial and nonprofit users may distribute copies of Zesto in
 * compiled or executable form as set forth in Section 2, provided that either:
 * (A) it is accompanied by the corresponding machine-readable source code, or
 * (B) it is accompanied by a written offer, with no time limit, to give anyone
 * a machine-readable copy of the corresponding source code in return for
 * reimbursement of the cost of distribution. This written offer must permit
 * verbatim duplication by anyone, or (C) it is distributed by someone who
 * received only the executable form, and is accompanied by a copy of the
 * written offer of source code.
 * 
 * 7. Zesto was developed by Gabriel H. Loh, Ph.D.  US Mail: 266 Ferst Drive,
 * Georgia Institute of Technology, Atlanta, GA 30332-0765
 * 
 */


#ifdef ZESTO_PARSE_ARGS
  if(!strcasecmp(exec_opt_string,"NAIVE"))
    return new core_exec_NAIVE_t(core);
#else

class core_exec_NAIVE_t:public core_exec_t
{
  /* readyQ for scheduling */
  struct readyQ_node_t {
    struct uop_t * uop;
    seq_t uop_seq; /* seq id of uop when inserted - for proper sorting even after uop recycled */
    seq_t action_id;
    tick_t when_assigned;
    struct readyQ_node_t * next;
  };

  /* struct for a squashable in-flight uop (for example, a uop making its way
     down an ALU pipeline).  Changing the original uop's tag will make the tags
     no longer match, thereby invalidating the in-flight action. */
  struct uop_action_t {
    struct uop_t * uop;
    seq_t action_id;
    tick_t pipe_exit_time;
  };

  /* struct for a generic pipelined ALU */
  struct ALU_t {
    struct uop_action_t * pipe; /* implemented as min-heap */
    int occupancy;
    int latency;    /* number of cycles from start of execution to end */
    int issue_rate; /* number of cycles between issuing independent instructions on this ALU */
    tick_t when_executable; /* cycle when next instruction can actually start executing on this ALU */
  };

  public:

  core_exec_NAIVE_t(struct core_t * core);
  virtual void reg_stats(struct stat_sdb_t *sdb);
  virtual void freeze_stats(void);
  virtual void update_occupancy(void);

  virtual void ALU_exec(void);
  virtual void LDST_exec(void);
  virtual void RS_schedule(void);
  virtual void LDQ_schedule(void);

  virtual void recover(struct Mop_t * Mop);
  virtual void recover(void);

  virtual void insert_ready_uop(struct uop_t * uop);

  virtual bool RS_available(void);
  virtual void RS_insert(struct uop_t * uop);
  virtual void RS_fuse_insert(struct uop_t * uop);
  virtual void RS_deallocate(struct uop_t * uop);

  virtual bool LDQ_available(void);
  virtual void LDQ_insert(struct uop_t * uop);
  virtual void LDQ_deallocate(struct uop_t * uop);
  virtual void LDQ_squash(struct uop_t * dead_uop);

  virtual bool STQ_available(void);
  virtual void STQ_insert_sta(struct uop_t * uop);
  virtual void STQ_insert_std(struct uop_t * uop);
  virtual void STQ_deallocate_sta(void);
  virtual bool STQ_deallocate_std(struct uop_t * uop);
  virtual void STQ_deallocate_senior(void);
  virtual void STQ_squash_sta(struct uop_t * dead_uop);
  virtual void STQ_squash_std(struct uop_t * dead_uop);
  virtual void STQ_squash_senior(void);

  virtual void recover_check_assertions(void);

  protected:
  struct readyQ_node_t * readyQ_free_pool; /* for scheduling readyQ's */
  int readyQ_free_pool_debt;

  struct uop_t ** RS;
  int RS_num;

  struct LDQ_t {
    bool valid; /* is this entry allocated/ */
    struct uop_t * uop;
    md_addr_t virt_addr;
    md_paddr_t phys_addr;
    int mem_size;
    bool addr_valid;
    int store_color;   /* STQ index of most recent store before this load */
    bool hit_in_STQ;    /* received value from STQ */
    tick_t when_issued; /* when load actually issued */
  } * LDQ;
  int LDQ_head;
  int LDQ_tail;
  int LDQ_num;

  struct STQ_t {
    struct uop_t * sta;
    struct uop_t * std;
    seq_t uop_seq;
    md_addr_t virt_addr;
    md_paddr_t phys_addr;
    int mem_size;
    union val_t value;
    bool addr_valid;
    bool value_valid;
    int next_load; /* LDQ index of next load in program order */

    /* for commit */
    int translation_complete; /* dtlb access finished */
    int write_complete; /* write access finished */
    seq_t action_id; /* need to squash commits when a core resets (multi-core mode only) */
  } * STQ;
  int STQ_head;
  int STQ_tail;
  int STQ_num;

  struct exec_port_t {
    struct ALU_t * FU[NUM_FU_CLASSES];
    struct readyQ_node_t * readyQ;
    struct ALU_t * STQ; /* store-queue lookup/search pipeline for load execution */
  } * port;

  /* various exec utility functions */

  struct readyQ_node_t * get_readyQ_node(void);
  void return_readyQ_node(struct readyQ_node_t * p);
  bool check_load_issue_conditions(struct uop_t * uop);

  void load_writeback(struct uop_t * uop);

  /* callbacks need to be static */
  static void DL1_callback(void * op);
  static void DTLB_callback(void * op);
  static bool translated_callback(void * op,seq_t);
  static seq_t get_uop_action_id(void * op);

  /* callbacks used by commit for stores */
  static void store_dl1_callback(void * op);
  static void store_dtlb_callback(void * op);
  static bool store_translated_callback(void * op,seq_t);

  /* min-heap priority-Q management functions */
  void ALU_heap_balance(struct uop_action_t * pipe, int insert_position);
  void ALU_heap_remove(struct uop_action_t * pipe, int pipe_num);

};

/*******************/
/* SETUP FUNCTIONS */
/*******************/

core_exec_NAIVE_t::core_exec_NAIVE_t(struct core_t * arg_core):
  readyQ_free_pool(NULL), readyQ_free_pool_debt(0),
  RS_num(0), LDQ_head(0), LDQ_tail(0), LDQ_num(0),
  STQ_head(0), STQ_tail(0), STQ_num(0)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;

  RS = (struct uop_t**) calloc(knobs->exec.RS_size,sizeof(*RS));
  if(!RS)
    fatal("couldn't calloc RS");

  LDQ = (core_exec_NAIVE_t::LDQ_t*) calloc(knobs->exec.LDQ_size,sizeof(*LDQ));
  if(!LDQ)
    fatal("couldn't calloc LDQ");

  STQ = (core_exec_NAIVE_t::STQ_t*) calloc(knobs->exec.STQ_size,sizeof(*STQ));
  if(!STQ)
    fatal("couldn't calloc STQ");

  int i;
  /* This shouldn't be necessary, but I threw it in because valgrind (memcheck) was reporting
     that STQ[i].sta was being used uninitialized. */
  for(i=0;i<knobs->exec.STQ_size;i++)
    STQ[i].sta = NULL;

  /*********************************************************/
  /* Data cache parsing first, functional units after this */
  /*********************************************************/
  char name[256];
  int sets, assoc, linesize, latency, banks, bank_width, MSHR_entries, WBB_entries;
  char rp, ap, wp, wc;
  
  /* note: caches must be instantiated from the level furthest from the core first (e.g., L2) */


  /* DL1 */
  if(sscanf(knobs->memory.DL1_opt_str,"%[^:]:%d:%d:%d:%d:%d:%d:%c:%c:%c:%d:%d:%c",
      name,&sets,&assoc,&linesize,&banks,&bank_width,&latency,&rp,&ap,&wp, &MSHR_entries, &WBB_entries, &wc) != 13)
    fatal("invalid DL1 options: <name:sets:assoc:linesize:banks:bank-width:latency:repl-policy:alloc-policy:write-policy:num-MSHR:WB-buffers:write-combining>\n\t(%s)",knobs->memory.DL1_opt_str);

  core->memory.DL1 = cache_create(core,name,CACHE_READWRITE,sets,assoc,linesize,
                                   rp,ap,wp,wc,banks,bank_width,latency,
                                   WBB_entries,MSHR_entries,1,uncore->LLC,uncore->LLC_bus);

  core->memory.DL1->prefetcher = (struct prefetch_t**) calloc(knobs->memory.DL1_num_PF,sizeof(*core->memory.DL1->prefetcher));
  core->memory.DL1->num_prefetchers = knobs->memory.DL1_num_PF;
  if(!core->memory.DL1->prefetcher)
    fatal("couldn't calloc %s's prefetcher array",core->memory.DL1->name);
  for(i=0;i<knobs->memory.DL1_num_PF;i++)
    core->memory.DL1->prefetcher[i] = prefetch_create(knobs->memory.DL1PF_opt_str[i],core->memory.DL1);
  if(core->memory.DL1->prefetcher[0] == NULL)
    core->memory.DL1->num_prefetchers = knobs->memory.DL1_num_PF = 0;

  core->memory.DL1->PFF_size = knobs->memory.DL1_PFFsize;
  core->memory.DL1->PFF = (cache_t::PFF_t*) calloc(knobs->memory.DL1_PFFsize,sizeof(*core->memory.DL1->PFF));
  if(!core->memory.DL1->PFF)
    fatal("failed to calloc %s's prefetch FIFO",core->memory.DL1->name);
  prefetch_buffer_create(core->memory.DL1,knobs->memory.DL1_PF_buffer_size);
  prefetch_filter_create(core->memory.DL1,knobs->memory.DL1_PF_filter_size,knobs->memory.DL1_PF_filter_reset);
  core->memory.DL1->prefetch_threshold = knobs->memory.DL1_PFthresh;
  core->memory.DL1->prefetch_max = knobs->memory.DL1_PFmax;
  core->memory.DL1->PF_low_watermark = knobs->memory.DL1_low_watermark;
  core->memory.DL1->PF_high_watermark = knobs->memory.DL1_high_watermark;
  core->memory.DL1->PF_sample_interval = knobs->memory.DL1_WMinterval;

  /* DTLBs */
  core->memory.DTLB_bus = bus_create("DTLB_bus",1,1);

  /* DTLB2 */
  if(sscanf(knobs->memory.DTLB2_opt_str,"%[^:]:%d:%d:%d:%d:%c:%d",
      name,&sets,&assoc,&banks,&latency, &rp, &MSHR_entries) != 7)
    fatal("invalid DTLB2 options: <name:sets:assoc:banks:latency:repl-policy:num-MSHR>");

  core->memory.DTLB2 = cache_create(core,name,CACHE_READONLY,sets,assoc,1,rp,'w','t','n',banks,1,latency,4,MSHR_entries,1,uncore->LLC,uncore->LLC_bus); /* on a complete TLB miss, go to the L2 cache to simulate the traffice from a HW page-table walker */

  /* DTLB */
  if(sscanf(knobs->memory.DTLB_opt_str,"%[^:]:%d:%d:%d:%d:%c:%d",
      name,&sets,&assoc,&banks,&latency, &rp, &MSHR_entries) != 7)
    fatal("invalid DTLB options: <name:sets:assoc:banks:latency:repl-policy:num-MSHR>");

  core->memory.DTLB = cache_create(core,name,CACHE_READONLY,sets,assoc,1,rp,'w','t','n',banks,1,latency,4,MSHR_entries,1,core->memory.DTLB2,core->memory.DTLB_bus);


  /*******************/
  /* execution ports */
  /*******************/
  port = (core_exec_NAIVE_t::exec_port_t*) calloc(knobs->exec.num_exec_ports,sizeof(*port));
  if(!port)
    fatal("couldn't calloc exec ports");

  /***************************/
  /* execution port bindings */
  /***************************/
  for(i=0;i<NUM_FU_CLASSES;i++)
  {
    int j;
    knobs->exec.port_binding[i].ports = (int*) calloc(knobs->exec.port_binding[i].num_FUs,sizeof(int));
    if(!knobs->exec.port_binding[i].ports) fatal("couldn't calloc %s ports",MD_FU_NAME(i));
    for(j=0;j<knobs->exec.port_binding[i].num_FUs;j++)
    {
      if((knobs->exec.fu_bindings[i][j] < 0) || (knobs->exec.fu_bindings[i][j] >= knobs->exec.num_exec_ports))
        fatal("port binding for %s is negative or exceeds the execution width (should be > 0 and < %d)",MD_FU_NAME(i),knobs->exec.num_exec_ports);
      knobs->exec.port_binding[i].ports[j] = knobs->exec.fu_bindings[i][j];
    }
  }

  /***************************************/
  /* functional unit execution pipelines */
  /***************************************/
  for(i=0;i<NUM_FU_CLASSES;i++)
  {
    int j;
    for(j=0;j<knobs->exec.port_binding[i].num_FUs;j++)
    {
      int port_num = knobs->exec.port_binding[i].ports[j];
      int latency = knobs->exec.latency[i];
      int issue_rate = knobs->exec.issue_rate[i];
      int heap_size = 1<<((int)rint(ceil(log(latency+1)/log(2.0))));
      port[port_num].FU[i] = (struct ALU_t*) calloc(1,sizeof(struct ALU_t));
      if(!port[port_num].FU[i])
        fatal("couldn't calloc IEU");
      port[port_num].FU[i]->latency = latency;
      port[port_num].FU[i]->issue_rate = issue_rate;
      port[port_num].FU[i]->pipe = (struct uop_action_t*) calloc(heap_size,sizeof(struct uop_action_t));
      if(!port[port_num].FU[i]->pipe)
        fatal("couldn't calloc %s function unit execution pipeline",MD_FU_NAME(i));

      if(i==FU_LD) /* load has AGEN and STQ access pipelines */
      {
        port[port_num].STQ = (struct ALU_t*) calloc(1,sizeof(struct ALU_t));
        latency = core->memory.DL1->latency; /* assume STQ latency matched to DL1's */
        heap_size = 1<<((int)rint(ceil(log(latency+1)/log(2.0))));
        port[port_num].STQ->latency = latency;
        port[port_num].STQ->issue_rate = issue_rate;
        port[port_num].STQ->pipe = (struct uop_action_t*) calloc(heap_size,sizeof(struct uop_action_t));
        if(!port[port_num].STQ->pipe)
          fatal("couldn't calloc load's STQ exec pipe on port %d",j);
      }
    }
  }
}

void
core_exec_NAIVE_t::reg_stats(struct stat_sdb_t *sdb)
{
  char buf[1024];
  char buf2[1024];
  struct thread_t * arch = core->current_thread;

  stat_reg_note(sdb,"#### EXEC STATS ####");
  sprintf(buf,"c%d.exec_uops_issued",arch->id);
  stat_reg_counter(sdb, true, buf, "number of uops issued", &core->stat.exec_uops_issued, core->stat.exec_uops_issued, NULL);
  sprintf(buf,"c%d.exec_uPC",arch->id);
  sprintf(buf2,"c%d.exec_uops_issued/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "average number of uops executed per cycle", buf2, NULL);
  sprintf(buf,"c%d.exec_uops_replayed",arch->id);
  stat_reg_counter(sdb, true, buf, "number of uops replayed", &core->stat.exec_uops_replayed, core->stat.exec_uops_replayed, NULL);
  sprintf(buf,"c%d.exec_avg_replays",arch->id);
  sprintf(buf2,"c%d.exec_uops_replayed/c%d.exec_uops_issued",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "average replays per uop", buf2, NULL);
  sprintf(buf,"c%d.exec_uops_snatched",arch->id);
  stat_reg_counter(sdb, true, buf, "number of uops snatched-back", &core->stat.exec_uops_snatched_back, core->stat.exec_uops_snatched_back, NULL);
  sprintf(buf,"c%d.exec_avg_snatched",arch->id);
  sprintf(buf2,"c%d.exec_uops_snatched/c%d.exec_uops_issued",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "average snatch-backs per uop", buf2, NULL);
  sprintf(buf,"c%d.num_jeclear",arch->id);
  stat_reg_counter(sdb, true, buf, "number of branch mispredictions", &core->stat.num_jeclear, core->stat.num_jeclear, NULL);
  sprintf(buf,"c%d.num_wp_jeclear",arch->id);
  stat_reg_counter(sdb, true, buf, "number of branch mispredictions in the shadow of an earlier mispred", &core->stat.num_wp_jeclear, core->stat.num_wp_jeclear, NULL);

  sprintf(buf,"c%d.RS_occupancy",arch->id);
  stat_reg_counter(sdb, false, buf, "total RS occupancy", &core->stat.RS_occupancy, core->stat.RS_occupancy, NULL);
  sprintf(buf,"c%d.RS_empty",arch->id);
  stat_reg_counter(sdb, false, buf, "total cycles RS was empty", &core->stat.RS_empty_cycles, core->stat.RS_empty_cycles, NULL);
  sprintf(buf,"c%d.RS_full",arch->id);
  stat_reg_counter(sdb, false, buf, "total cycles RS was full", &core->stat.RS_full_cycles, core->stat.RS_full_cycles, NULL);
  sprintf(buf,"c%d.RS_avg",arch->id);
  sprintf(buf2,"c%d.RS_occupancy/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "average RS occupancy", buf2, NULL);
  sprintf(buf,"c%d.RS_frac_empty",arch->id);
  sprintf(buf2,"c%d.RS_empty/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles RS was empty", buf2, NULL);
  sprintf(buf,"c%d.RS_frac_full",arch->id);
  sprintf(buf2,"c%d.RS_full/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles RS was full", buf2, NULL);

  sprintf(buf,"c%d.LDQ_occupancy",arch->id);
  stat_reg_counter(sdb, true, buf, "total LDQ occupancy", &core->stat.LDQ_occupancy, core->stat.LDQ_occupancy, NULL);
  sprintf(buf,"c%d.LDQ_empty",arch->id);
  stat_reg_counter(sdb, true, buf, "total cycles LDQ was empty", &core->stat.LDQ_empty_cycles, core->stat.LDQ_empty_cycles, NULL);
  sprintf(buf,"c%d.LDQ_full",arch->id);
  stat_reg_counter(sdb, true, buf, "total cycles LDQ was full", &core->stat.LDQ_full_cycles, core->stat.LDQ_full_cycles, NULL);
  sprintf(buf,"c%d.LDQ_avg",arch->id);
  sprintf(buf2,"c%d.LDQ_occupancy/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "average LDQ occupancy", buf2, NULL);
  sprintf(buf,"c%d.LDQ_frac_empty",arch->id);
  sprintf(buf2,"c%d.LDQ_empty/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles LDQ was empty", buf2, NULL);
  sprintf(buf,"c%d.LDQ_frac_full",arch->id);
  sprintf(buf2,"c%d.LDQ_full/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles LDQ was full", buf2, NULL);

  sprintf(buf,"c%d.STQ_occupancy",arch->id);
  stat_reg_counter(sdb, true, buf, "total STQ occupancy", &core->stat.STQ_occupancy, core->stat.STQ_occupancy, NULL);
  sprintf(buf,"c%d.STQ_empty",arch->id);
  stat_reg_counter(sdb, true, buf, "total cycles STQ was empty", &core->stat.STQ_empty_cycles, core->stat.STQ_empty_cycles, NULL);
  sprintf(buf,"c%d.STQ_full",arch->id);
  stat_reg_counter(sdb, true, buf, "total cycles STQ was full", &core->stat.STQ_full_cycles, core->stat.STQ_full_cycles, NULL);
  sprintf(buf,"c%d.STQ_avg",arch->id);
  sprintf(buf2,"c%d.STQ_occupancy/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "average STQ occupancy", buf2, NULL);
  sprintf(buf,"c%d.STQ_frac_empty",arch->id);
  sprintf(buf2,"c%d.STQ_empty/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles STQ was empty", buf2, NULL);
  sprintf(buf,"c%d.STQ_frac_full",arch->id);
  sprintf(buf2,"c%d.STQ_full/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles STQ was full", buf2, NULL);

  stat_reg_note(sdb,"\n#### DATA CACHE STATS ####");
  cache_reg_stats(sdb, core, core->memory.DL1);
  cache_reg_stats(sdb, core, core->memory.DTLB);
  cache_reg_stats(sdb, core, core->memory.DTLB2);
}

void core_exec_NAIVE_t::freeze_stats(void)
{
}

void core_exec_NAIVE_t::update_occupancy(void)
{
}

/* Functions to support dependency tracking 
   NOTE: "Ready" Queue is somewhat of a misnomer... uops are placed
   in the readyQ when all of their data-flow parents have issued.
   However, that doesn't necessarily mean that the corresponding
   inputs are "ready" due to non-unit execution latencies.  So it's
   really the Ready-And-Soon-To-Be-Ready-Queue... */
core_exec_NAIVE_t::readyQ_node_t * core_exec_NAIVE_t::get_readyQ_node(void)
{
}

void core_exec_NAIVE_t::return_readyQ_node(struct readyQ_node_t * p)
{
}

/* Add the uop to the corresponding readyQ (based on port binding - we maintain
   one readyQ per execution port) */
void core_exec_NAIVE_t::insert_ready_uop(struct uop_t * uop)
{
}

/*****************************/
/* MAIN SCHED/EXEC FUNCTIONS */
/*****************************/

void core_exec_NAIVE_t::RS_schedule(void) /* for uops in the RS */
{
}

/* returns true if load is allowed to issue (or is predicted to be ok) */
bool core_exec_NAIVE_t::check_load_issue_conditions(struct uop_t * uop)
{
}

/* The callback functions below (after load_writeback) mark flags
   in the uop to specify the completion of each task, and only when
   both are done do we call the load-writeback function to finish
   off execution of the load. */

void core_exec_NAIVE_t::load_writeback(struct uop_t * uop)
{
}

void core_exec_NAIVE_t::DL1_callback(void * op)
{
}

void core_exec_NAIVE_t::DTLB_callback(void * op)
{
}

/* returns true if TLB translation has completed */
bool core_exec_NAIVE_t::translated_callback(void * op, seq_t action_id)
{
}

/* Used by the cache processing functions to recover the id of the
   uop without needing to know about the uop struct. */
seq_t core_exec_NAIVE_t::get_uop_action_id(void * op)
{
}

/* process loads exiting the STQ search pipeline, update caches */
void core_exec_NAIVE_t::LDST_exec(void)
{
}

/* Schedule load uops to execute from the LDQ.  Load execution occurs in a two-step
   process: first the address gets computed (issuing from the RS), and then the
   cache/STQ search occur (issuing from the LDQ). */
void core_exec_NAIVE_t::LDQ_schedule(void)
{
}

/* Process execution (in ALUs) of uops */
void core_exec_NAIVE_t::ALU_exec(void)
{
}

void core_exec_NAIVE_t::recover(struct Mop_t * Mop)
{
}

void core_exec_NAIVE_t::recover(void)
{
}

bool core_exec_NAIVE_t::RS_available(void)
{
}

/* assumes you already called RS_available to check that
   an entry is available */
void core_exec_NAIVE_t::RS_insert(struct uop_t * uop)
{
}

/* add uops to an existing entry (where all uops in the same
   entry are assumed to be fused) */
void core_exec_NAIVE_t::RS_fuse_insert(struct uop_t * uop)
{
}

void core_exec_NAIVE_t::RS_deallocate(struct uop_t * dead_uop)
{
}

bool core_exec_NAIVE_t::LDQ_available(void)
{
}

void core_exec_NAIVE_t::LDQ_insert(struct uop_t * uop)
{
}

/* called by commit */
void core_exec_NAIVE_t::LDQ_deallocate(struct uop_t * uop)
{
}

void core_exec_NAIVE_t::LDQ_squash(struct uop_t * dead_uop)
{
}

bool core_exec_NAIVE_t::STQ_available(void)
{
}

void core_exec_NAIVE_t::STQ_insert_sta(struct uop_t * uop)
{
}

void core_exec_NAIVE_t::STQ_insert_std(struct uop_t * uop)
{
}

void core_exec_NAIVE_t::STQ_deallocate_sta(void)
{
}

/* returns true if successful */
bool core_exec_NAIVE_t::STQ_deallocate_std(struct uop_t * uop)
{
}

void core_exec_NAIVE_t::STQ_deallocate_senior(void)
{
}

void core_exec_NAIVE_t::STQ_squash_sta(struct uop_t * dead_uop)
{
}

void core_exec_NAIVE_t::STQ_squash_std(struct uop_t * dead_uop)
{
}

void core_exec_NAIVE_t::STQ_squash_senior(void)
{
}

void core_exec_NAIVE_t::recover_check_assertions(void)
{
}

/* Stores don't write back to cache/memory until commit.  When D$
   and DTLB accesses complete, these functions get called which
   update the status of the corresponding STQ entries.  The STQ
   entry cannot be deallocated until the store has completed. */
void core_exec_NAIVE_t::store_dl1_callback(void * op)
{
}

void core_exec_NAIVE_t::store_dtlb_callback(void * op)
{
}

bool core_exec_NAIVE_t::store_translated_callback(void * op, seq_t action_id /* ignored */)
{
}

/* input state: assumes that the new node has just been inserted at
   insert_position and all remaining nodes obey the heap property */
void core_exec_NAIVE_t::ALU_heap_balance(struct uop_action_t * pipe, int insert_position)
{
}

/* input state: pipe_num is the number of elements in the heap
   prior to removing the root node. */
void core_exec_NAIVE_t::ALU_heap_remove(struct uop_action_t * pipe, int pipe_num)
{
}

#endif
