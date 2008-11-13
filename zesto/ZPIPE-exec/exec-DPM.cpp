/* exec-DPM.cpp - Detailed Pipeline Model */
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
  if(!strcasecmp(exec_opt_string,"DPM"))
    return new core_exec_DPM_t(core);
#else

class core_exec_DPM_t:public core_exec_t
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
  };

  /* struct for a generic pipelined ALU */
  struct ALU_t {
    struct uop_action_t * pipe;
    int occupancy;
    int latency;    /* number of cycles from start of execution to end */
    int issue_rate; /* number of cycles between issuing independent instructions on this ALU */
    tick_t when_scheduleable; /* cycle when next instruction can be scheduled for this ALU */
    tick_t when_executable; /* cycle when next instruction can actually start executing on this ALU */
  };

  public:

  core_exec_DPM_t(struct core_t * core);
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
  int RS_eff_num;

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
    bool speculative_broadcast; /* made a speculative tag broadcast */
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
  int STQ_senior_num;
  int STQ_senior_head;

  struct exec_port_t {
    struct uop_action_t * payload_pipe;
    int occupancy;
    struct ALU_t * FU[NUM_FU_CLASSES];
    struct readyQ_node_t * readyQ;
    struct ALU_t * STQ; /* store-queue lookup/search pipeline for load execution */
    tick_t when_bypass_used; /* to make sure only one inst writes back per cycle, which
                                could happen due to insts with different latencies */
  } * port;

  struct memdep_t * memdep;


  /* various exec utility functions */

  struct readyQ_node_t * get_readyQ_node(void);
  void return_readyQ_node(struct readyQ_node_t * p);
  bool check_load_issue_conditions(struct uop_t * uop);
  void snatch_back(struct uop_t * replayed_uop);

  void load_writeback(struct uop_t * uop);

  /* callbacks need to be static */
  static void DL1_callback(void * op);
  static void DTLB_callback(void * op);
  static bool translated_callback(void * op,seq_t);
  static seq_t get_uop_action_id(void * op);
  static void load_miss_reschedule(void * op, int new_pred_latency);

  /* callbacks used by commit for stores */
  static void store_dl1_callback(void * op);
  static void store_dtlb_callback(void * op);
  static bool store_translated_callback(void * op,seq_t);

};

/*******************/
/* SETUP FUNCTIONS */
/*******************/

core_exec_DPM_t::core_exec_DPM_t(struct core_t * arg_core):
  readyQ_free_pool(NULL), readyQ_free_pool_debt(0),
  RS_num(0), RS_eff_num(0), LDQ_head(0), LDQ_tail(0), LDQ_num(0),
  STQ_head(0), STQ_tail(0), STQ_num(0), STQ_senior_num(0),
  STQ_senior_head(0)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;

  RS = (struct uop_t**) calloc(knobs->exec.RS_size,sizeof(*RS));
  if(!RS)
    fatal("couldn't calloc RS");

  LDQ = (core_exec_DPM_t::LDQ_t*) calloc(knobs->exec.LDQ_size,sizeof(*LDQ));
  if(!LDQ)
    fatal("couldn't calloc LDQ");

  STQ = (core_exec_DPM_t::STQ_t*) calloc(knobs->exec.STQ_size,sizeof(*STQ));
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


  /************************************/
  /* execution port payload pipelines */
  /************************************/
  port = (core_exec_DPM_t::exec_port_t*) calloc(knobs->exec.num_exec_ports,sizeof(*port));
  if(!port)
    fatal("couldn't calloc exec ports");
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    port[i].payload_pipe = (struct uop_action_t*) calloc(knobs->exec.payload_depth,sizeof(*port->payload_pipe));
    if(!port[i].payload_pipe)
      fatal("couldn't calloc payload pipe");
  }

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
      port[port_num].FU[i] = (struct ALU_t*) calloc(1,sizeof(struct ALU_t));
      if(!port[port_num].FU[i])
        fatal("couldn't calloc IEU");
      port[port_num].FU[i]->latency = latency;
      port[port_num].FU[i]->issue_rate = issue_rate;
      port[port_num].FU[i]->pipe = (struct uop_action_t*) calloc(latency,sizeof(struct uop_action_t));
      if(!port[port_num].FU[i]->pipe)
        fatal("couldn't calloc %s function unit execution pipeline",MD_FU_NAME(i));

      if(i==FU_LD) /* load has AGEN and STQ access pipelines */
      {
        port[port_num].STQ = (struct ALU_t*) calloc(1,sizeof(struct ALU_t));
        latency = core->memory.DL1->latency; /* assume STQ latency matched to DL1's */
        port[port_num].STQ->latency = latency;
        port[port_num].STQ->issue_rate = issue_rate;
        port[port_num].STQ->pipe = (struct uop_action_t*) calloc(latency,sizeof(struct uop_action_t));
        if(!port[port_num].STQ->pipe)
          fatal("couldn't calloc load's STQ exec pipe on port %d",j);
      }
    }
  }

  memdep = memdep_create(knobs->exec.memdep_opt_str);
}

void
core_exec_DPM_t::reg_stats(struct stat_sdb_t *sdb)
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
  sprintf(buf,"c%d.load_nukes",arch->id);
  stat_reg_counter(sdb, true, buf, "num pipeflushes due to load-store order violation", &core->stat.load_nukes, core->stat.load_nukes, NULL);
  sprintf(buf,"c%d.wp_load_nukes",arch->id);
  stat_reg_counter(sdb, true, buf, "num pipeflushes due to load-store order violation on wrong-path", &core->stat.wp_load_nukes, core->stat.wp_load_nukes, NULL);

  sprintf(buf,"c%d.RS_occupancy",arch->id);
  stat_reg_counter(sdb, false, buf, "total RS occupancy", &core->stat.RS_occupancy, core->stat.RS_occupancy, NULL);
  sprintf(buf,"c%d.RS_eff_occupancy",arch->id);
  stat_reg_counter(sdb, false, buf, "total RS effective occupancy", &core->stat.RS_eff_occupancy, core->stat.RS_eff_occupancy, NULL);
  sprintf(buf,"c%d.RS_empty",arch->id);
  stat_reg_counter(sdb, false, buf, "total cycles RS was empty", &core->stat.RS_empty_cycles, core->stat.RS_empty_cycles, NULL);
  sprintf(buf,"c%d.RS_full",arch->id);
  stat_reg_counter(sdb, false, buf, "total cycles RS was full", &core->stat.RS_full_cycles, core->stat.RS_full_cycles, NULL);
  sprintf(buf,"c%d.RS_avg",arch->id);
  sprintf(buf2,"c%d.RS_occupancy/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "average RS occupancy", buf2, NULL);
  sprintf(buf,"c%d.RS_eff_avg",arch->id);
  sprintf(buf2,"c%d.RS_eff_occupancy/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "effective average RS occupancy", buf2, NULL);
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

  memdep->reg_stats(sdb, core);

  stat_reg_note(sdb,"\n#### DATA CACHE STATS ####");
  cache_reg_stats(sdb, core, core->memory.DL1);
  cache_reg_stats(sdb, core, core->memory.DTLB);
  cache_reg_stats(sdb, core, core->memory.DTLB2);
}

void core_exec_DPM_t::freeze_stats(void)
{
  memdep->freeze_stats();
}

void core_exec_DPM_t::update_occupancy(void)
{
    /* RS */
  core->stat.RS_occupancy += RS_num;
  core->stat.RS_eff_occupancy += RS_eff_num;
  if(RS_num >= core->knobs->exec.RS_size)
    core->stat.RS_full_cycles++;
  if(RS_num <= 0)
    core->stat.RS_empty_cycles++;

    /* LDQ */
  core->stat.LDQ_occupancy += LDQ_num;
  if(LDQ_num >= core->knobs->exec.LDQ_size)
    core->stat.LDQ_full_cycles++;
  if(LDQ_num <= 0)
    core->stat.LDQ_empty_cycles++;

    /* STQ */
  core->stat.STQ_occupancy += STQ_num;
  if(STQ_num >= core->knobs->exec.STQ_size)
    core->stat.STQ_full_cycles++;
  if(STQ_num <= 0)
    core->stat.STQ_empty_cycles++;
}

/* Functions to support dependency tracking 
   NOTE: "Ready" Queue is somewhat of a misnomer... uops are placed
   in the readyQ when all of their data-flow parents have issued.
   However, that doesn't necessarily mean that the corresponding
   inputs are "ready" due to non-unit execution latencies.  So it's
   really the Ready-And-Soon-To-Be-Ready-Queue... */
core_exec_DPM_t::readyQ_node_t * core_exec_DPM_t::get_readyQ_node(void)
{
  struct readyQ_node_t * p = NULL;
  if(readyQ_free_pool)
  {
    p = readyQ_free_pool;
    readyQ_free_pool = p->next;
  }
  else
  {
    p = (struct readyQ_node_t*) calloc(1,sizeof(*p));
    if(!p)
      fatal("couldn't calloc a readyQ node");
  }
  assert(p);
  p->next = NULL;
  p->when_assigned = sim_cycle;
  readyQ_free_pool_debt++;
  return p;
}

void core_exec_DPM_t::return_readyQ_node(struct readyQ_node_t * p)
{
  assert(p);
  assert(p->uop);
  p->next = readyQ_free_pool;
  readyQ_free_pool = p;
  p->uop = NULL;
  p->when_assigned = -1;
  readyQ_free_pool_debt--;
}

/* Add the uop to the corresponding readyQ (based on port binding - we maintain
   one readyQ per execution port) */
void core_exec_DPM_t::insert_ready_uop(struct uop_t * uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert((uop->alloc.port_assignment >= 0) && (uop->alloc.port_assignment < knobs->exec.num_exec_ports),(void)0);
  zesto_assert(uop->timing.when_completed == TICK_T_MAX,(void)0);
  zesto_assert(uop->timing.when_exec == TICK_T_MAX,(void)0);
  zesto_assert(uop->timing.when_issued == TICK_T_MAX,(void)0);
  zesto_assert(!uop->exec.in_readyQ,(void)0);

  struct readyQ_node_t ** RQ = &port[uop->alloc.port_assignment].readyQ;
  struct readyQ_node_t * new_node = get_readyQ_node();
  new_node->uop = uop;
  new_node->uop_seq = uop->decode.uop_seq;
  uop->exec.in_readyQ = true;
  uop->exec.action_id = core->new_action_id();
  new_node->action_id = uop->exec.action_id;

  if(!*RQ) /* empty ready queue */
  {
    *RQ = new_node;
    return;
  }

  struct readyQ_node_t * current = *RQ, * prev = NULL;

  /* insert in age order: first find insertion point */
  while(current && (current->uop_seq < uop->decode.uop_seq))
  {
    prev = current;
    current = current->next;
  }

  if(!prev) /* uop is oldest */
  {
    new_node->next = *RQ;
    *RQ = new_node;
  }
  else
  {
    new_node->next = current;
    prev->next = new_node;
  }
}

/*****************************/
/* MAIN SCHED/EXEC FUNCTIONS */
/*****************************/

void core_exec_DPM_t::RS_schedule(void) /* for uops in the RS */
{
  struct core_knobs_t * knobs = core->knobs;
  int i;

  /* select/pick from ready instructions and send to exec ports */
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    struct readyQ_node_t * rq = port[i].readyQ;
    struct readyQ_node_t * prev = NULL;
    int issued = false;

    if(port[i].payload_pipe[0].uop == NULL) /* port is free */
      while(rq) /* if anyone's waiting to issue to this port */
      {
        struct uop_t * uop = rq->uop;

        if(uop->exec.action_id != rq->action_id) /* RQ entry has been squashed */
        {
          struct readyQ_node_t * next = rq->next;
          /* remove from readyQ */
          if(prev)
            prev->next = next;
          else
            port[i].readyQ = next;
          return_readyQ_node(rq);
          /* and go on to next node */
          rq = next;
        }
        else if((uop->timing.when_ready <= sim_cycle) &&
                !issued &&
                (port[i].FU[uop->decode.FU_class]->when_scheduleable <= sim_cycle) &&
                ((!uop->decode.in_fusion) || uop->decode.fusion_head->alloc.full_fusion_allocated))
        {
          zesto_assert(uop->alloc.port_assignment == i,(void)0);

          port[i].payload_pipe[0].uop = uop;
          port[i].payload_pipe[0].action_id = uop->exec.action_id;
          port[i].occupancy++;
          zesto_assert(port[i].occupancy <= knobs->exec.payload_depth,(void)0);
          uop->timing.when_issued = sim_cycle;

          if(uop->decode.is_load)
          {
            int fp_penalty = REG_IS_FPR(uop->decode.odep_name)?knobs->exec.fp_penalty:0;
            uop->timing.when_otag_ready = sim_cycle + port[i].FU[uop->decode.FU_class]->latency + core->memory.DL1->latency + fp_penalty;
          }
          else
          {
            int fp_penalty = ((REG_IS_FPR(uop->decode.odep_name) && !(uop->decode.opflags & F_FCOMP)) ||
                             (!REG_IS_FPR(uop->decode.odep_name) && (uop->decode.opflags & F_FCOMP)))?knobs->exec.fp_penalty:0;
            uop->timing.when_otag_ready = sim_cycle + port[i].FU[uop->decode.FU_class]->latency + fp_penalty;
          }

          port[i].FU[uop->decode.FU_class]->when_scheduleable = sim_cycle + port[i].FU[uop->decode.FU_class]->issue_rate;

          /* tag broadcast to dependents */
          struct odep_t * odep = uop->exec.odep_uop;
          while(odep)
          {
            int j;
            tick_t when_ready = 0;
            odep->uop->timing.when_itag_ready[odep->op_num] = uop->timing.when_otag_ready;
            for(j=0;j<MAX_IDEPS;j++)
            {
              if(when_ready < odep->uop->timing.when_itag_ready[j])
                when_ready = odep->uop->timing.when_itag_ready[j];
            }
            odep->uop->timing.when_ready = when_ready;

            if(when_ready < TICK_T_MAX)
              insert_ready_uop(odep->uop);

            odep = odep->next;
          }

          if(uop->decode.is_load)
          {
            zesto_assert((uop->alloc.LDQ_index >= 0) && (uop->alloc.LDQ_index < knobs->exec.LDQ_size),(void)0);
            LDQ[uop->alloc.LDQ_index].speculative_broadcast = true;
          }

          struct readyQ_node_t * next = rq->next;
          /* remove from readyQ */
          if(prev)
            prev->next = next;
          else
            port[i].readyQ = next;
          return_readyQ_node(rq);
          uop->exec.in_readyQ = false;
          ZESTO_STAT(core->stat.exec_uops_issued++;)

          /* only one uop schedules from an issue port per cycle */
          issued = true;
          rq = next;
        }
        else
        {
          /* node valid, but not ready; skip over it */
          prev = rq;
          rq = rq->next;
        }
      }
  }
}

/* returns true if load is allowed to issue (or is predicted to be ok) */
bool core_exec_DPM_t::check_load_issue_conditions(struct uop_t * uop)
{
  struct core_knobs_t * knobs = core->knobs;
  /* are all previous STA's known? If there's a match, is the STD ready? */
  bool sta_unknown = false;
  bool regular_match = false;
  bool partial_match = false;
  bool oracle_regular_match = false;
  bool oracle_partial_match = false;
  int i;
  int match_index = -1;
  int oracle_index = -1;
  int num_stores = 0; /* need this extra condition because STQ could be full with all stores older than the load we're considering */

  /* don't reissue someone who's already issued */
  zesto_assert((uop->alloc.LDQ_index >= 0) && (uop->alloc.LDQ_index < knobs->exec.LDQ_size),false);
  if(LDQ[uop->alloc.LDQ_index].when_issued != TICK_T_MAX)
    return false;

  md_addr_t ld_addr1 = LDQ[uop->alloc.LDQ_index].uop->oracle.virt_addr;
  md_addr_t ld_addr2 = LDQ[uop->alloc.LDQ_index].virt_addr + uop->decode.mem_size - 1;

  /* this searches the senior STQ as well. */
  for(i=LDQ[uop->alloc.LDQ_index].store_color;
      (((i+1)%knobs->exec.STQ_size) != STQ_senior_head) && (STQ[i].uop_seq < uop->decode.uop_seq) && (num_stores < STQ_senior_num);
      i=(i-1+knobs->exec.STQ_size) % knobs->exec.STQ_size)
  {
    /* check addr match */
    int st_mem_size = STQ[i].mem_size;
    int ld_mem_size = uop->decode.mem_size;
    md_addr_t st_addr1, st_addr2;
    
    if(STQ[i].addr_valid)
      st_addr1 = STQ[i].virt_addr; /* addr of first byte */
    else
    {
      zesto_assert(STQ[i].sta,false);
      st_addr1 = STQ[i].sta->oracle.virt_addr; /* addr of first byte */
      sta_unknown = true;
    }
    st_addr2 = st_addr1 + st_mem_size - 1; /* addr of last byte */

    zesto_assert(st_mem_size,false);
    zesto_assert(ld_mem_size,false);
    zesto_assert(uop->Mop->oracle.spec_mode || (ld_addr1 && ld_addr2),false);
    zesto_assert((st_addr1 && st_addr2) || (STQ[i].sta && STQ[i].sta->Mop->oracle.spec_mode),false);

    if((st_addr1 <= ld_addr1) && (st_addr2 >= ld_addr2)) /* store write fully overlaps the load address range */
    {
      if((match_index == -1) && (STQ[i].addr_valid))
      {
        match_index = i;
        regular_match = true;
      }

      if(oracle_index == -1)
      {
        oracle_index = i;
        oracle_regular_match = true;
      }
    }
    else if((st_addr2 < ld_addr1) || (st_addr1 > ld_addr2)) /* store addr is completely before or after the load */
    {
      /* no conflict here */
    }
    else /* partial store */
    {
      if((match_index == -1) && (STQ[i].addr_valid))
      {
        match_index = i;
        partial_match = true;
      }

      if(oracle_index == -1)
      {
        oracle_index = i;
        oracle_partial_match = true;
      }
    }

    num_stores++;
  }

  if(partial_match)
  {
    /* in presence of (known) partial store forwarding, stall until store *commits* */
    return false;
  }

  return memdep->lookup(uop->Mop->fetch.PC,sta_unknown,oracle_regular_match,oracle_partial_match);
}

/* helper function to remove issued uops after a latency misprediction */
void core_exec_DPM_t::snatch_back(struct uop_t * replayed_uop)
{
  struct core_knobs_t * knobs = core->knobs;
  int i;
  int index = 0;
  struct uop_t ** stack = (struct uop_t**) alloca(sizeof(*stack) * knobs->exec.RS_size);
  if(!stack)
    fatal("couldn't alloca snatch_back_stack");
  stack[0] = replayed_uop;

  while(index >= 0)
  {
    struct uop_t * uop = stack[index];

    /* reset current uop */
    uop->timing.when_issued = TICK_T_MAX;
    uop->timing.when_exec = TICK_T_MAX;
    uop->timing.when_completed = TICK_T_MAX;
    uop->timing.when_otag_ready = TICK_T_MAX;
    if(uop->decode.is_load)
    {
      zesto_assert((uop->alloc.LDQ_index >= 0) && (uop->alloc.LDQ_index < knobs->exec.LDQ_size),(void)0);
      LDQ[uop->alloc.LDQ_index].hit_in_STQ = false;
      LDQ[uop->alloc.LDQ_index].addr_valid = false;
      LDQ[uop->alloc.LDQ_index].when_issued = TICK_T_MAX;
    }

    /* remove uop from payload RAM pipe */
    if(port[uop->alloc.port_assignment].occupancy > 0)
      for(i=0;i<knobs->exec.payload_depth;i++)
        if(port[uop->alloc.port_assignment].payload_pipe[i].uop == uop)
        {
          port[uop->alloc.port_assignment].payload_pipe[i].uop = NULL;
          port[uop->alloc.port_assignment].occupancy--;
          zesto_assert(port[uop->alloc.port_assignment].occupancy >= 0,(void)0);
          ZESTO_STAT(core->stat.exec_uops_snatched_back++;)
          break;
        }

    /* remove uop from readyQ */
    uop->exec.action_id = core->new_action_id();
    uop->exec.in_readyQ = false;

    /* remove uop from squash list */
    stack[index] = NULL;
    index --;

    /* add dependents to squash list */
    struct odep_t * odep = uop->exec.odep_uop;
    while(odep)
    {
      /* squash this input tag */
      odep->uop->timing.when_itag_ready[odep->op_num] = TICK_T_MAX;

      if(odep->uop->timing.when_issued != TICK_T_MAX) /* if the child issued */
      {
        index ++;
        stack[index] = odep->uop;
      }
      if(odep->uop->exec.in_readyQ) /* if child is in the readyQ */
      {
        odep->uop->exec.action_id = core->new_action_id();
        odep->uop->exec.in_readyQ = false;
      }
      odep = odep->next;
    }
  }

  tick_t when_ready = 0;
  for(i=0;i<MAX_IDEPS;i++)
    if(replayed_uop->timing.when_itag_ready[i] > when_ready)
      when_ready = replayed_uop->timing.when_itag_ready[i];
  /* we assume if a when_ready has been reset to TICK_T_MAX, then
     the parent will re-wakeup the child.  In any case, we also
     assume that a parent that takes longer to execute than
     originally predicted will update its children's
     when_itag_ready's to the new value */
  if(when_ready != TICK_T_MAX) /* otherwise reschedule the uop */
  {
    replayed_uop->timing.when_ready = when_ready;
    insert_ready_uop(replayed_uop);
  }
}

/* The callback functions below (after load_writeback) mark flags
   in the uop to specify the completion of each task, and only when
   both are done do we call the load-writeback function to finish
   off execution of the load. */

void core_exec_DPM_t::load_writeback(struct uop_t * uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert((uop->alloc.LDQ_index >= 0) && (uop->alloc.LDQ_index < knobs->exec.LDQ_size),(void)0);
  if(!LDQ[uop->alloc.LDQ_index].hit_in_STQ) /* no match in STQ, so use cache value */
  {

    int fp_penalty = REG_IS_FPR(uop->decode.odep_name)?knobs->exec.fp_penalty:0;

    port[uop->alloc.port_assignment].when_bypass_used = sim_cycle+fp_penalty;
    uop->exec.ovalue = uop->oracle.ovalue; /* XXX: just using oracle value for now */
    uop->exec.ovalue_valid = true;
    zesto_assert(uop->timing.when_completed == TICK_T_MAX,(void)0);
    uop->timing.when_completed = sim_cycle+fp_penalty;
    last_completed = sim_cycle+fp_penalty; /* for deadlock detection */
    if(uop->decode.is_ctrl && (uop->Mop->oracle.NextPC != uop->Mop->fetch.pred_NPC)) /* XXX: for RETN */
    {
      core->oracle->pipe_recover(uop->Mop,uop->Mop->oracle.NextPC);
      ZESTO_STAT(core->stat.num_jeclear++;)
      if(uop->Mop->oracle.spec_mode)
        ZESTO_STAT(core->stat.num_wp_jeclear++;)
    }


    if(uop->timing.when_otag_ready > (sim_cycle+fp_penalty))
      /* we thought this output would be ready later in the future,
         but it's ready now! */
      uop->timing.when_otag_ready = sim_cycle+fp_penalty;

    /* bypass output value to dependents, but also check to see if
       dependents were already speculatively scheduled; if not,
       wake them up. */
    struct odep_t * odep = uop->exec.odep_uop;

    while(odep)
    {
      /* check scheduling info (tags) */
      if(odep->uop->timing.when_itag_ready[odep->op_num] > (sim_cycle+fp_penalty))
      {
        int j;
        tick_t when_ready = 0;

        odep->uop->timing.when_itag_ready[odep->op_num] = sim_cycle+fp_penalty;

        for(j=0;j<MAX_IDEPS;j++)
          if(when_ready < odep->uop->timing.when_itag_ready[j])
            when_ready = odep->uop->timing.when_itag_ready[j];

        if(when_ready < TICK_T_MAX)
        {
          odep->uop->timing.when_ready = when_ready;
          if(!odep->uop->exec.in_readyQ)
            insert_ready_uop(odep->uop);
        }
      }

      /* bypass value */
      zesto_assert(!odep->uop->exec.ivalue_valid[odep->op_num],(void)0);
      odep->uop->exec.ivalue_valid[odep->op_num] = true;
      if(odep->aflags) /* shouldn't happen for loads? */
      {
        warn("load modified flags?\n");
        odep->uop->exec.ivalue[odep->op_num].dw = uop->exec.oflags;
      }
      else
        odep->uop->exec.ivalue[odep->op_num] = uop->exec.ovalue;
      odep->uop->timing.when_ival_ready[odep->op_num] = sim_cycle+fp_penalty;

      odep = odep->next;
    }

  }
}

void core_exec_DPM_t::DL1_callback(void * op)
{
  struct uop_t * uop = (struct uop_t*) op;
#ifndef DEBUG
  struct core_t * core = uop->core;
#endif
  class core_exec_DPM_t * E = (core_exec_DPM_t*)uop->core->exec;
  if(uop->alloc.LDQ_index != -1)
  {
    zesto_assert(uop->exec.when_data_loaded == TICK_T_MAX,(void)0);
    uop->exec.when_data_loaded = sim_cycle;
    if((uop->exec.when_data_loaded <= sim_cycle) &&
       (uop->exec.when_addr_translated <= sim_cycle) &&
        !E->LDQ[uop->alloc.LDQ_index].hit_in_STQ) /* no match in STQ, so use cache value */
    {
      /* if load received value from STQ, it could have already
         committed by the time this gets called (esp. if we went to
         main memory) */
      E->load_writeback(uop);
    }
  }
}

void core_exec_DPM_t::DTLB_callback(void * op)
{
  struct uop_t * uop = (struct uop_t*) op;
#ifndef DEBUG
  struct core_t * core = uop->core;
#endif
  struct core_exec_DPM_t * E = (core_exec_DPM_t*)uop->core->exec;
  if(uop->alloc.LDQ_index != -1)
  {
    zesto_assert(uop->exec.when_addr_translated == TICK_T_MAX,(void)0);
    uop->exec.when_addr_translated = sim_cycle;
    if((uop->exec.when_data_loaded <= sim_cycle) &&
       (uop->exec.when_addr_translated <= sim_cycle) &&
        !E->LDQ[uop->alloc.LDQ_index].hit_in_STQ)
      E->load_writeback(uop);
  }
}

/* returns true if TLB translation has completed */
bool core_exec_DPM_t::translated_callback(void * op, seq_t action_id)
{
  struct uop_t * uop = (struct uop_t*) op;
  if((uop->alloc.LDQ_index != -1) && (uop->exec.action_id == action_id))
    return uop->exec.when_addr_translated <= sim_cycle;
  else
    return true;
}

/* Used by the cache processing functions to recover the id of the
   uop without needing to know about the uop struct. */
seq_t core_exec_DPM_t::get_uop_action_id(void * op)
{
  struct uop_t * uop = (struct uop_t*) op;
  return uop->exec.action_id;
}

/* This function takes care of uops that have been misscheduled due
   to a latency misprediction (e.g., they got scheduled assuming
   the parent uop would hit in the DL1, but it turns out that the
   load latency will be longer due to a cache miss).  Uops may be
   speculatively rescheduled based on the next anticipated latency
   (e.g., L2 hit latency). */
void core_exec_DPM_t::load_miss_reschedule(void * op, int new_pred_latency)
{
  struct uop_t * uop = (struct uop_t*) op;
  struct core_t * core = uop->core;
  struct core_exec_DPM_t * E = (core_exec_DPM_t*)core->exec;
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert(uop->decode.is_load,(void)0);
  zesto_assert((uop->alloc.LDQ_index >= 0) && (uop->alloc.LDQ_index < knobs->exec.LDQ_size),(void)0);

  /* if we've speculatively woken up our dependents, we need to
     snatch them back out of the execution pipeline and replay them
     later (but if the load already hit in the STQ, then don't need
     to replay) */
  if(E->LDQ[uop->alloc.LDQ_index].speculative_broadcast && !E->LDQ[uop->alloc.LDQ_index].hit_in_STQ)
  {
    /* we speculatively scheduled children assuming we'd hit in the
       previous cache level, but we didn't... so put children back
       to sleep */
    uop->timing.when_otag_ready = TICK_T_MAX;
    struct odep_t * odep = uop->exec.odep_uop;
    while(odep)
    {
      odep->uop->timing.when_itag_ready[odep->op_num] = TICK_T_MAX;
      E->snatch_back(odep->uop);

      odep = odep->next;
    }

    /* now assume a hit in this cache level */
    odep = uop->exec.odep_uop;
    uop->timing.when_otag_ready = sim_cycle + new_pred_latency - knobs->exec.payload_depth - 1;

    while(odep)
    {
      odep->uop->timing.when_itag_ready[odep->op_num] = uop->timing.when_otag_ready;

      /* put back on to readyQ if appropriate */
      int j;
      tick_t when_ready = 0;
      zesto_assert(!odep->uop->exec.in_readyQ,(void)0);

      for(j=0;j<MAX_IDEPS;j++)
        if(when_ready < odep->uop->timing.when_itag_ready[j])
          when_ready = odep->uop->timing.when_itag_ready[j];

      if(when_ready < TICK_T_MAX)
      {
        odep->uop->timing.when_ready = when_ready;
        E->insert_ready_uop(odep->uop);
      }

      odep = odep->next;
    }
  }
}

/* process loads exiting the STQ search pipeline, update caches */
void core_exec_DPM_t::LDST_exec(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int i;

  /* shuffle STQ pipeline forward */
  for(i=0;i<knobs->exec.port_binding[FU_LD].num_FUs;i++)
  {
    int port_num = knobs->exec.port_binding[FU_LD].ports[i];
    int stage = port[port_num].STQ->latency-1;
    int j;
    for(j=stage;j>0;j--)
      port[port_num].STQ->pipe[j] = port[port_num].STQ->pipe[j-1];
    port[port_num].STQ->pipe[0].uop = NULL;
  }

  /* process STQ pipes */
  for(i=0;i<knobs->exec.port_binding[FU_LD].num_FUs;i++)
  {
    int port_num = knobs->exec.port_binding[FU_LD].ports[i];
    int stage = port[port_num].STQ->latency-1;
    int j;

    struct uop_t * uop = port[port_num].STQ->pipe[stage].uop;

    if(uop && (port[port_num].STQ->pipe[stage].action_id == uop->exec.action_id))
    {
      int num_stores = 0;
      zesto_assert((uop->alloc.LDQ_index >= 0) && (uop->alloc.LDQ_index < knobs->exec.LDQ_size),(void)0);

      /* check STQ for match, including senior STQ */
      /*for(j=LDQ[uop->alloc.LDQ_index].store_color;
          STQ[j].sta && (STQ[j].sta->decode.uop_seq < uop->decode.uop_seq) && (num_stores < STQ_senior_num);
          j=(j-1+knobs->exec.STQ_size) % knobs->exec.STQ_size)*/

      j=LDQ[uop->alloc.LDQ_index].store_color;

      int cond1 = STQ[j].sta != NULL;
      seq_t seq1 = (seq_t)-1, seq2 = (seq_t)-1;
      zesto_assert(j >= 0,(void)0);
      zesto_assert(j < knobs->exec.STQ_size,(void)0);
      if(j)
      {
        seq1 = (seq_t)-2;
      }
      if(cond1)
      {
        seq1 = STQ[j].sta->decode.uop_seq;
        seq2 = uop->decode.uop_seq;
      }
      int cond2 = cond1 && (seq1 < seq2);
      int cond3 = (num_stores < STQ_senior_num);

      while(cond1 && cond2 && cond3)
      {
        int st_mem_size = STQ[j].mem_size;
        int ld_mem_size = uop->decode.mem_size;
        md_addr_t st_addr1 = STQ[j].virt_addr; /* addr of first byte */
        md_addr_t st_addr2 = STQ[j].virt_addr + st_mem_size - 1; /* addr of last byte */
        md_addr_t ld_addr1 = LDQ[uop->alloc.LDQ_index].virt_addr;
        md_addr_t ld_addr2 = LDQ[uop->alloc.LDQ_index].virt_addr + ld_mem_size - 1;

        if(STQ[j].addr_valid)
        {
          if((st_addr1 <= ld_addr1) && (st_addr2 >= ld_addr2)) /* match */
          {
            if(uop->timing.when_completed != TICK_T_MAX)
            {
              memdep->update(uop->Mop->fetch.PC);
              core->oracle->pipe_flush(uop->Mop);
              ZESTO_STAT(core->stat.load_nukes++;)
              if(uop->Mop->oracle.spec_mode)
                ZESTO_STAT(core->stat.wp_load_nukes++;)
            }
            else
            {
              if(STQ[j].value_valid)
              {
                int fp_penalty = REG_IS_FPR(uop->decode.odep_name)?knobs->exec.fp_penalty:0;
                uop->exec.ovalue = STQ[j].value;
                uop->exec.ovalue_valid = true;
                uop->timing.when_completed = sim_cycle+fp_penalty;
                uop->timing.when_exec = sim_cycle+fp_penalty;
                last_completed = sim_cycle+fp_penalty; /* for deadlock detection */
                LDQ[uop->alloc.LDQ_index].hit_in_STQ = true;

                if(uop->decode.is_ctrl && (uop->Mop->oracle.NextPC != uop->Mop->fetch.pred_NPC)) /* for RETN */
                {
                  core->oracle->pipe_recover(uop->Mop,uop->Mop->oracle.NextPC);
                  ZESTO_STAT(core->stat.num_jeclear++;)
                  if(uop->Mop->oracle.spec_mode)
                    ZESTO_STAT(core->stat.num_wp_jeclear++;)
                }

                /* we thought this output would be ready later in the future, but
                   it's ready now! */
                if(uop->timing.when_otag_ready > (sim_cycle+fp_penalty))
                  uop->timing.when_otag_ready = sim_cycle+fp_penalty;

                struct odep_t * odep = uop->exec.odep_uop;

                while(odep)
                {
                  /* check scheduling info (tags) */
                  if(odep->uop->timing.when_itag_ready[odep->op_num] > sim_cycle)
                  {
                    int j;
                    tick_t when_ready = 0;

                    odep->uop->timing.when_itag_ready[odep->op_num] = sim_cycle+fp_penalty;

                    for(j=0;j<MAX_IDEPS;j++)
                      if(when_ready < odep->uop->timing.when_itag_ready[j])
                        when_ready = odep->uop->timing.when_itag_ready[j];

                    if(when_ready < TICK_T_MAX)
                    {
                      odep->uop->timing.when_ready = when_ready;
                      if(!odep->uop->exec.in_readyQ)
                        insert_ready_uop(odep->uop);
                    }
                  }

                  /* bypass output value to dependents */
                  odep->uop->exec.ivalue_valid[odep->op_num] = true;
                  if(odep->aflags) /* shouldn't happen for loads? */
                  {
                    warn("load modified flags?");
                    odep->uop->exec.ivalue[odep->op_num].dw = uop->exec.oflags;
                  }
                  else
                    odep->uop->exec.ivalue[odep->op_num] = uop->exec.ovalue;
                  odep->uop->timing.when_ival_ready[odep->op_num] = sim_cycle+fp_penalty;

                  odep = odep->next;
                }
              }
              else /* addr match but STD unknown */
              {
                /* reset the load's children */
                if(LDQ[uop->alloc.LDQ_index].speculative_broadcast)
                {
                  uop->timing.when_otag_ready = TICK_T_MAX;
                  uop->timing.when_completed = TICK_T_MAX;
                  struct odep_t * odep = uop->exec.odep_uop;
                  while(odep)
                  {
                    odep->uop->timing.when_itag_ready[odep->op_num] = TICK_T_MAX;
                    odep->uop->exec.ivalue_valid[odep->op_num] = false;
                    snatch_back(odep->uop);

                    odep = odep->next;
                  }
                  /* clear flag so we don't keep doing this over and over again */
                  LDQ[uop->alloc.LDQ_index].speculative_broadcast = false;
                }
                uop->exec.action_id = core->new_action_id();
                /* When_issued is still set, so LDQ won't reschedule the load.  The load
                   will wait until the STD finishes and wakes it up. */
              }
            }
            break;
          }
          else if((st_addr2 < ld_addr1) || (st_addr1 > ld_addr2)) /* no overlap */
          {
            /* nothing to do */
          }
          else /* partial match */
          {
            /* squash if load already executed */
            if(uop->timing.when_completed != TICK_T_MAX)
            {
              /* flush is slightly different from branch mispred -
                 on branch, we squash everything *after* the branch,
                 whereas with this flush (e.g., used for memory
                 order violations), we squash the Mop itself as well
                 as everything after it */
              memdep->update(uop->Mop->fetch.PC);
              core->oracle->pipe_flush(uop->Mop);
              ZESTO_STAT(core->stat.load_nukes++;)
              if(uop->Mop->oracle.spec_mode)
                ZESTO_STAT(core->stat.wp_load_nukes++;)
            }
            /* reset the load's children - the load's going to have to wait until this store commits */
            else if(LDQ[uop->alloc.LDQ_index].speculative_broadcast)
            {
              uop->timing.when_otag_ready = TICK_T_MAX;
              uop->timing.when_completed = TICK_T_MAX;
              struct odep_t * odep = uop->exec.odep_uop;
              while(odep)
              {
                odep->uop->timing.when_itag_ready[odep->op_num] = TICK_T_MAX;
                odep->uop->exec.ivalue_valid[odep->op_num] = false;
                snatch_back(odep->uop);

                odep = odep->next;
              }
              /* clear flag so we don't keep doing this over and over again */
              LDQ[uop->alloc.LDQ_index].speculative_broadcast = false;
            }
            uop->exec.action_id = core->new_action_id();

            /* different from STD-not-ready case above, in the case of a partial match,
               the load may not get woken up by the store (and even if it does, the
               load still can't execute, so we reset the load's when_issed so that
               LDQ_schedule keeps polling until the partial-matching store retires. */
            LDQ[uop->alloc.LDQ_index].when_issued = TICK_T_MAX;
            LDQ[uop->alloc.LDQ_index].hit_in_STQ = false;
            break;
          }
        }


        num_stores++;

        j=(j-1+knobs->exec.STQ_size) % knobs->exec.STQ_size;

        cond1 = STQ[j].sta != NULL;
        cond2 = cond1 && (STQ[j].sta->decode.uop_seq < uop->decode.uop_seq);
        cond3 = (num_stores < STQ_senior_num);
      }
    }
  }

  cache_process(core->memory.DTLB2);
  cache_process(core->memory.DTLB);
  if((core->current_thread->id == 0) && !(sim_cycle&uncore->LLC_cycle_mask))
    cache_process(uncore->LLC);
  cache_process(core->memory.DL1);
}

/* Schedule load uops to execute from the LDQ.  Load execution occurs in a two-step
   process: first the address gets computed (issuing from the RS), and then the
   cache/STQ search occur (issuing from the LDQ). */
void core_exec_DPM_t::LDQ_schedule(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int i;
  /* walk LDQ, if anyone's ready, issue to DTLB/DL1 */
  for(i=0;i<LDQ_num;i++)
  {
    int index = (LDQ_head + i) % knobs->exec.LDQ_size;
    if(LDQ[index].addr_valid) /* agen has finished */
    {
      if(LDQ[index].when_issued == TICK_T_MAX) /* load hasn't issued to DL1/STQ already */
      {
        if(check_load_issue_conditions(LDQ[index].uop)) /* retval of true means load is predicted to be cleared for issue */
        {
          struct uop_t * uop = LDQ[index].uop;
          if((cache_enqueuable(core->memory.DTLB,core->current_thread->id,PAGE_TABLE_ADDR(core->current_thread->id,uop->oracle.virt_addr))) &&
             (cache_enqueuable(core->memory.DL1,core->current_thread->id,uop->oracle.virt_addr)) &&
             (port[uop->alloc.port_assignment].STQ->pipe[0].uop == NULL))
          {
            uop->exec.when_data_loaded = TICK_T_MAX;
            uop->exec.when_addr_translated = TICK_T_MAX;
            cache_enqueue(core,core->memory.DTLB,NULL,CACHE_READ,core->current_thread->id,uop->Mop->fetch.PC,PAGE_TABLE_ADDR(core->current_thread->id,uop->oracle.virt_addr),uop->exec.action_id,0,NO_MSHR,uop,DTLB_callback,load_miss_reschedule,NULL,get_uop_action_id);
            cache_enqueue(core,core->memory.DL1,NULL,CACHE_READ,core->current_thread->id,uop->Mop->fetch.PC,uop->oracle.virt_addr,uop->exec.action_id,0,NO_MSHR,uop,DL1_callback,load_miss_reschedule,translated_callback,get_uop_action_id);

            port[uop->alloc.port_assignment].STQ->pipe[0].uop = uop;
            port[uop->alloc.port_assignment].STQ->pipe[0].action_id = uop->exec.action_id;
            LDQ[index].when_issued = sim_cycle;
            if(!LDQ[index].speculative_broadcast) /* need to re-wakeup children */
            {
              struct odep_t * odep = LDQ[index].uop->exec.odep_uop;
              if(knobs->exec.payload_depth < core->memory.DL1->latency) /* assume DL1 hit */
                LDQ[index].uop->timing.when_otag_ready = sim_cycle + core->memory.DL1->latency - knobs->exec.payload_depth;
              else
                LDQ[index].uop->timing.when_otag_ready = sim_cycle;
              while(odep)
              {
                /* odep already finished executing.  Normally this wouldn't (can't?) happen,
                   but there are some weird interactions between cache misses, store-to-load
                   forwarding, and load stalling on earlier partial forwarding cases. */
                if(odep->uop->timing.when_exec != TICK_T_MAX) /* inst already finished executing */
                {
                  /* bad mis-scheduling, just flush to recover */
                  core->oracle->pipe_recover(uop->Mop,uop->Mop->oracle.NextPC);
                  ZESTO_STAT(core->stat.num_jeclear++;)
                  if(uop->Mop->oracle.spec_mode)
                    ZESTO_STAT(core->stat.num_wp_jeclear++;)
                }
                else
                {
                  odep->uop->timing.when_itag_ready[odep->op_num] = LDQ[index].uop->timing.when_otag_ready;

                  /* put back on to readyQ if appropriate */
                  int j;
                  tick_t when_ready = 0;

                  for(j=0;j<MAX_IDEPS;j++)
                    if(when_ready < odep->uop->timing.when_itag_ready[j])
                      when_ready = odep->uop->timing.when_itag_ready[j];

                  if(when_ready < TICK_T_MAX)
                  {
                    odep->uop->timing.when_ready = when_ready;
                    /* is it possible that the odep is already on the readyq because if
                       it has more than one input replayed on the same cycle, then the
                       first inst to replay will have already placed the odep into
                       the readyq. */
                    if(!odep->uop->exec.in_readyQ)
                      insert_ready_uop(odep->uop);
                  }
                }

                odep = odep->next;
              }
              LDQ[index].speculative_broadcast = true;
            }
          }
        }
        else
        {
          LDQ[index].uop->timing.when_otag_ready = TICK_T_MAX;
          LDQ[index].uop->timing.when_completed = TICK_T_MAX;

          if(LDQ[index].speculative_broadcast)
          {
            /* we speculatively scheduled children assuming we'd immediately
               go to cache, but we didn't... so put children back to sleep */
            struct odep_t * odep = LDQ[index].uop->exec.odep_uop;
            while(odep)
            {
              odep->uop->timing.when_itag_ready[odep->op_num] = TICK_T_MAX;
              odep->uop->exec.ivalue_valid[odep->op_num] = false;
              snatch_back(odep->uop);

              odep = odep->next;
            }
            /* clear flag so we don't keep doing this over and over again */
            LDQ[index].speculative_broadcast = false;
          }
        }
      }
    }
  }
}

/* Process actual execution (in ALUs) of uops, as well as shuffling of
   uops through the payload pipeline. */
void core_exec_DPM_t::ALU_exec(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int i;

  /* Process Functional Units */
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    int j;
    for(j=0;j<NUM_FU_CLASSES;j++) /* TODO: shorten this loop w/ an explicit list of bound FUs */
    {
      struct ALU_t * FU = port[i].FU[j];
      if(FU && (FU->occupancy > 0))
      {
        /* process last stage of FU pipeline (those uops completing execution) */
        int stage = FU->latency-1;
        struct uop_t * uop = FU->pipe[stage].uop;
        if(uop)
        {
          int squashed = (FU->pipe[stage].action_id != uop->exec.action_id);
          int bypass_available = (port[i].when_bypass_used != sim_cycle);
          int needs_bypass = !(uop->decode.is_sta||uop->decode.is_std||uop->decode.is_load||uop->decode.is_ctrl);

          if(squashed || !needs_bypass || bypass_available)
          {
            FU->occupancy--;
            zesto_assert(FU->occupancy >= 0,(void)0);
            FU->pipe[stage].uop = NULL;
          }

          /* there's a uop completing execution (that hasn't been squashed) */
          if(!squashed && (!needs_bypass || bypass_available))
          {
            if(needs_bypass)
              port[i].when_bypass_used = sim_cycle;

            if(uop->decode.is_load) /* loads need to be processed differently */
            {
              /* update load queue entry */
              zesto_assert((uop->alloc.LDQ_index >= 0) && (uop->alloc.LDQ_index < knobs->exec.LDQ_size),(void)0);
              LDQ[uop->alloc.LDQ_index].virt_addr = uop->oracle.virt_addr;
              LDQ[uop->alloc.LDQ_index].addr_valid = true;
              /* actual scheduling from load queue takes place in LDQ_schedule() */
            }
            else
            {
              int fp_penalty = ((REG_IS_FPR(uop->decode.odep_name) && !(uop->decode.opflags & F_FCOMP)) ||
                               (!REG_IS_FPR(uop->decode.odep_name) && (uop->decode.opflags & F_FCOMP)))?knobs->exec.fp_penalty:0;
              /* TODO: real execute-at-execute of instruction */
              /* XXX for now just copy oracle value */
              uop->exec.ovalue_valid = true;
              uop->exec.ovalue = uop->oracle.ovalue;

              /* alloc, uopQ, and decode all have to search for the
                 recovery point (Mop) because a long flow may have
                 uops that span multiple sections of the pipeline.  */
              if(uop->decode.is_ctrl && (uop->Mop->oracle.NextPC != uop->Mop->fetch.pred_NPC))
              {
                /* XXX: should compute */
                core->oracle->pipe_recover(uop->Mop,uop->Mop->oracle.NextPC);
                ZESTO_STAT(core->stat.num_jeclear++;)
                if(uop->Mop->oracle.spec_mode)
                  ZESTO_STAT(core->stat.num_wp_jeclear++;)
              }
              else if(uop->decode.is_sta)
              {
                zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
                zesto_assert(!STQ[uop->alloc.STQ_index].addr_valid,(void)0);
                STQ[uop->alloc.STQ_index].virt_addr = uop->oracle.virt_addr;
                STQ[uop->alloc.STQ_index].addr_valid = true;
              }
              else if(uop->decode.is_std)
              {
                zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
                zesto_assert(!STQ[uop->alloc.STQ_index].value_valid,(void)0);
                STQ[uop->alloc.STQ_index].value = uop->exec.ovalue;
                STQ[uop->alloc.STQ_index].value_valid = true;
              }

              if((uop->decode.is_sta || uop->decode.is_std) &&
                  STQ[uop->alloc.STQ_index].addr_valid &&
                  STQ[uop->alloc.STQ_index].value_valid)
              {
                /* both parts of store have now made it to the STQ: walk
                   forward in LDQ to see if there are any loads waiting on this
                   this STD.  If so, unblock them. */
                int idx;
                int num_loads = 0;
                int overwrite_index = uop->alloc.STQ_index;

                /* XXX using oracle info here. */
                zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
                md_addr_t st_addr1 = STQ[uop->alloc.STQ_index].sta->oracle.virt_addr;
                md_addr_t st_addr2 = st_addr1 + STQ[uop->alloc.STQ_index].mem_size - 1;

                /* this is a bit mask for each byte of the stored value; if it reaches zero,
                   then we've been completely overwritten and we can stop.
                   least-sig bit corresponds to lowest address byte. */
                int overwrite_mask = (1<< STQ[uop->alloc.STQ_index].mem_size)-1;

                for(idx=STQ[uop->alloc.STQ_index].next_load;
                    LDQ[idx].uop && (LDQ[idx].uop->decode.uop_seq > uop->decode.uop_seq) && (num_loads < LDQ_num);
                    idx=(idx+1) % knobs->exec.LDQ_size)
                {
                  if(LDQ[idx].store_color != uop->alloc.STQ_index) /* some younger stores present */
                  {
                    /* scan store queue for younger loads to see if we've been overwritten */
                    while(overwrite_index != LDQ[idx].store_color)
                    {
                      overwrite_index = (overwrite_index + 1) % knobs->exec.STQ_size;
                      if(overwrite_index == STQ_tail)
                        zesto_fatal("searching for matching store color but hit the end of the STQ",(void)0);

                      md_addr_t new_st_addr1 = STQ[overwrite_index].virt_addr;
                      md_addr_t new_st_addr2 = new_st_addr1 + STQ[overwrite_index].mem_size - 1;

                      /* does this store overwrite any of our bytes? 
                         (1) address has been computed and
                         (2) addr does NOT come completely before or after us */
                      if(STQ[overwrite_index].addr_valid &&
                        !((st_addr2 < new_st_addr1) || (st_addr1 > new_st_addr2)))
                      {
                        /* If the old store does a write of 8 bytes at addres 1000, then
                           its mask is:  0000000011111111 (with the lsb representing the
                           byte at address 1000, and the 8th '1' bit mapping to 1007).
                           If the next store is a 2 byte write to address 1002, then
                           its initial mask is 00..00011, which gets shifted up to:
                           0000000000001100.  We then use this to mask out those two
                           bits to get: 0000000011110011, with the remaining 1's indicating
                           which bytes are still "in play" (i.e. have not been overwritten
                           by a younger store */
                        int new_write_mask = (1<<STQ[overwrite_index].mem_size)-1;
                        int offset = new_st_addr1 - st_addr1;
                        if(offset < 0) /* new_addr is at lower address */
                          new_write_mask >>= (-offset);
                        else
                          new_write_mask <<= offset;
                        overwrite_mask &= ~new_write_mask;

                        if(overwrite_mask == 0)
                          break; /* while */
                      }
                    }

                    if(overwrite_mask == 0)
                      break; /* for */
                  }

                  if(LDQ[idx].addr_valid && /* if addr not valid, load hasn't finished AGEN... it'll pick up the right value after AGEN */
                     (LDQ[idx].when_issued != TICK_T_MAX)) /* similar to addr not valid, the load'll grab the value when it issues from the LDQ */
                  {
                    md_addr_t ld_addr1 = LDQ[idx].virt_addr;
                    md_addr_t ld_addr2 = ld_addr1 + LDQ[idx].mem_size - 1;

                    /* order violation/forwarding case/partial forwarding can
                       occur if there's any overlap in the addresses */
                    if(!((st_addr2 < ld_addr1) || (st_addr1 > ld_addr2)))
                    {
                      /* Similar to the store-overwrite mask above, we take the store
                         mask to see which bytes are still in play (if all bytes overwritten,
                         then the overwriting stores would have been responsible for forwarding
                         to this load/checking for misspeculations).  Taking the same mask
                         as above, if the store wrote to address 1000, and there was that
                         one intervening store, the mask would still be 0000000011110011.
                         Now if we have a 2-byte load from address 1002, it would generate
                         a mask of 00..00011,then shifted to 00..0001100.  When AND'ed with
                         the store mask, we would get all zeros indicating no conflict (this
                         is ok, even though the store overlaps the load's address range,
                         the latter 2-byte store would have forwarded the value to this 2-byte
                         load).  However, if the load was say 4-byte read from 09FE, it would
                         have a mask of 000..001111.  We would then shift the store's mask
                         (since the load is at a lower address) to 00..0001111001100.  ANDing
                         these gives 00...001100, which indicates that the 2 least significant
                         bytes of the store overlapped with the two most-significant bytes of
                         the load, which reveals a match.  Hooray for non-aligned loads and
                         stores of every which size! */
                      int load_read_mask = (1<<LDQ[idx].mem_size) - 1;
                      int offset = ld_addr1 - st_addr1;
                      if(offset < 0) /* load is at lower address */
                        overwrite_mask <<= (-offset);
                      else
                        load_read_mask <<= offset;

                      if(load_read_mask & overwrite_mask)
                      {
                        if(LDQ[idx].uop->timing.when_completed != TICK_T_MAX) /* completed --> memory-ordering violation */
                        {
                          memdep->update(LDQ[idx].uop->Mop->fetch.PC);

                          ZESTO_STAT(core->stat.load_nukes++;)
                          if(LDQ[idx].uop->Mop->oracle.spec_mode)
                            ZESTO_STAT(core->stat.wp_load_nukes++;)

                          core->oracle->pipe_flush(LDQ[idx].uop->Mop);
                        }
                        else /* attempted to issue, but STD not available or still in flight in the memory hierarchy */
                        {
                          /* mark load as schedulable */
                          LDQ[idx].when_issued = TICK_T_MAX; /* this will allow it to get issued from LDQ_schedule */
                          LDQ[idx].hit_in_STQ = false; /* it's possible the store commits before the load gets back to searching the STQ */

                          /* invalidate any in-flight loads from cache hierarchy */
                          LDQ[idx].uop->exec.action_id = core->new_action_id();

                          /* since load attempted to issue, its dependents may have been mis-scheduled */
                          if(LDQ[idx].speculative_broadcast)
                          {
                            LDQ[idx].uop->timing.when_otag_ready = TICK_T_MAX;
                            LDQ[idx].uop->timing.when_completed = TICK_T_MAX;
                            struct odep_t * odep = LDQ[idx].uop->exec.odep_uop;
                            while(odep)
                            {
                              odep->uop->timing.when_itag_ready[odep->op_num] = TICK_T_MAX;
                              odep->uop->exec.ivalue_valid[odep->op_num] = false;
                              snatch_back(odep->uop);

                              odep = odep->next;
                            }
                            /* clear flag so we don't keep doing this over and over again */
                            LDQ[idx].speculative_broadcast = false;
                          }
                        }
                      }
                    }
                  }

                  num_loads++;
                }
              }
              zesto_assert(uop->timing.when_completed == TICK_T_MAX,(void)0);
              uop->timing.when_completed = sim_cycle+fp_penalty;
              last_completed = sim_cycle+fp_penalty; /* for deadlock detection */

              /* bypass output value to dependents */
              struct odep_t * odep = uop->exec.odep_uop;
              while(odep)
              {
                zesto_assert(!odep->uop->exec.ivalue_valid[odep->op_num],(void)0);
                odep->uop->exec.ivalue_valid[odep->op_num] = true;
                if(odep->aflags)
                  odep->uop->exec.ivalue[odep->op_num].dw = uop->exec.oflags;
                else
                  odep->uop->exec.ivalue[odep->op_num] = uop->exec.ovalue;
                odep->uop->timing.when_ival_ready[odep->op_num] = sim_cycle+fp_penalty;

                odep = odep->next;
              }
            }
          }
        }

        /* shuffle the rest forward */
        if(FU->occupancy > 0)
        {
          for( /*nada*/; stage > 0; stage--)
          {
            if((FU->pipe[stage].uop == NULL) && FU->pipe[stage-1].uop)
            {
              FU->pipe[stage] = FU->pipe[stage-1];
              FU->pipe[stage-1].uop = NULL;
            }
          }
        }
      }
    }
  }

  /* Process Payload RAM pipestages */
  for(i=0;i<knobs->exec.num_exec_ports;i++)
  {
    if(port[i].occupancy > 0)
    {
      int stage = knobs->exec.payload_depth-1;
      struct uop_t * uop = port[i].payload_pipe[stage].uop;

      /* uops leaving payload section go to their respective FU's */
      if(uop && (port[i].payload_pipe[stage].action_id == uop->exec.action_id)) /* uop is valid, hasn't been squashed */
      {
        int j;
        int all_ready = true;
        enum md_fu_class FU_class = uop->decode.FU_class;

        /* uop leaves payload regardless of whether it replays */
        port[i].payload_pipe[stage].uop = NULL;
        port[i].occupancy--;
        zesto_assert(port[i].occupancy >= 0,(void)0);

        for(j=0;j<MAX_IDEPS;j++)
          all_ready &= uop->exec.ivalue_valid[j];

        /* have all input values arrived and FU available? */
        if(!all_ready || (port[i].FU[FU_class]->pipe[0].uop != NULL) || (port[i].FU[FU_class]->when_executable > sim_cycle))
        {
          /* if not, replay */
          ZESTO_STAT(core->stat.exec_uops_replayed++;)
          snatch_back(uop);
        }
        else
        {
          zesto_assert((uop->alloc.RS_index >= 0) && (uop->alloc.RS_index < knobs->exec.RS_size),(void)0);
          if(uop->decode.in_fusion)
            uop->decode.fusion_head->exec.uops_in_RS--;

          if((!uop->decode.in_fusion) || (uop->decode.fusion_head->exec.uops_in_RS == 0)) /* only deallocate when entire fused uop finished (or not fused) */
          {
            RS[uop->alloc.RS_index] = NULL;
            RS_num--;
            zesto_assert(RS_num >= 0,(void)0);
            if(uop->decode.in_fusion)
            {
              struct uop_t * fusion_uop = uop->decode.fusion_head;
              while(fusion_uop)
              {
                fusion_uop->alloc.RS_index = -1;
                fusion_uop = fusion_uop->decode.fusion_next;
              }
            }
            else
              uop->alloc.RS_index = -1;
          }

          RS_eff_num--;
          zesto_assert(RS_eff_num >= 0,(void)0);

          /* update port loading table */
          core->alloc->RS_deallocate(uop);

          uop->timing.when_exec = sim_cycle;

          /* this port has the proper FU and the first stage is free. */
          zesto_assert(port[i].FU[FU_class] && (port[i].FU[FU_class]->pipe[0].uop == NULL),(void)0);

          port[i].FU[FU_class]->pipe[0].uop = uop;
          port[i].FU[FU_class]->pipe[0].action_id = uop->exec.action_id;
          port[i].FU[FU_class]->occupancy++;
          port[i].FU[FU_class]->when_executable = sim_cycle + port[i].FU[FU_class]->issue_rate;
        }
      }
      else if(uop && (port[i].payload_pipe[stage].action_id != uop->exec.action_id)) /* uop has been squashed */
      {
        port[i].payload_pipe[stage].uop = NULL;
        port[i].occupancy--;
        zesto_assert(port[i].occupancy >= 0,(void)0);
      }

      /* shuffle the other uops through the payload pipeline */

      for(/*nada*/; stage > 0; stage--)
        port[i].payload_pipe[stage] = port[i].payload_pipe[stage-1];
      port[i].payload_pipe[0].uop = NULL;
    }
  }
}

void core_exec_DPM_t::recover(struct Mop_t * Mop)
{
  /* most flushing/squashing is accomplished through assignments of new action_id's */
}

void core_exec_DPM_t::recover(void)
{
  /* most flushing/squashing is accomplished through assignments of new action_id's */
}

bool core_exec_DPM_t::RS_available(void)
{
  struct core_knobs_t * knobs = core->knobs;
  return RS_num < knobs->exec.RS_size;
}

/* assumes you already called RS_available to check that
   an entry is available */
void core_exec_DPM_t::RS_insert(struct uop_t * uop)
{
  struct core_knobs_t * knobs = core->knobs;
  int RS_index;
  /* find a free RS entry */
  for(RS_index=0;RS_index < knobs->exec.RS_size;RS_index++)
  {
    if(RS[RS_index] == NULL)
      break;
  }
  if(RS_index == knobs->exec.RS_size)
    zesto_fatal("RS and RS_num out of sync",(void)0);

  RS[RS_index] = uop;
  RS_num++;
  RS_eff_num++;
  uop->alloc.RS_index = RS_index;

  if(uop->decode.in_fusion)
    uop->exec.uops_in_RS ++; /* used to track when RS can be deallocated */
  uop->alloc.full_fusion_allocated = false;
}

/* add uops to an existing entry (where all uops in the same
   entry are assumed to be fused) */
void core_exec_DPM_t::RS_fuse_insert(struct uop_t * uop)
{
  /* fusion body shares same RS entry as fusion head */
  uop->alloc.RS_index = uop->decode.fusion_head->alloc.RS_index;
  RS_eff_num++;
  uop->decode.fusion_head->exec.uops_in_RS ++;
  if(uop->decode.fusion_next == NULL)
    uop->decode.fusion_head->alloc.full_fusion_allocated = true;
}

void core_exec_DPM_t::RS_deallocate(struct uop_t * dead_uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert(dead_uop->alloc.RS_index < knobs->exec.RS_size,(void)0);
  if(dead_uop->decode.in_fusion && (dead_uop->timing.when_exec == TICK_T_MAX))
  {
    dead_uop->decode.fusion_head->exec.uops_in_RS --;
    RS_eff_num--;
    zesto_assert(RS_eff_num >= 0,(void)0);
  }
  if(!dead_uop->decode.in_fusion)
  {
    RS_eff_num--;
    zesto_assert(RS_eff_num >= 0,(void)0);
  }

  if(dead_uop->decode.is_fusion_head)
    zesto_assert(dead_uop->exec.uops_in_RS == 0,(void)0);

  if((!dead_uop->decode.in_fusion) || dead_uop->decode.is_fusion_head) /* make head uop responsible for deallocating RS during recovery */
  {
    RS[dead_uop->alloc.RS_index] = NULL;
    RS_num --;
    zesto_assert(RS_num >= 0,(void)0);
  }
  dead_uop->alloc.RS_index = -1;
}

bool core_exec_DPM_t::LDQ_available(void)
{
  struct core_knobs_t * knobs = core->knobs;
  return LDQ_num < knobs->exec.LDQ_size;
}

void core_exec_DPM_t::LDQ_insert(struct uop_t * uop)
{
  struct core_knobs_t * knobs = core->knobs;
  //memset(&LDQ[LDQ_tail],0,sizeof(*LDQ));
  memzero(&LDQ[LDQ_tail],sizeof(*LDQ));
  LDQ[LDQ_tail].uop = uop;
  LDQ[LDQ_tail].mem_size = uop->decode.mem_size;
  LDQ[LDQ_tail].store_color = (STQ_tail - 1 + knobs->exec.STQ_size) % knobs->exec.STQ_size;
  LDQ[LDQ_tail].when_issued = TICK_T_MAX;
  uop->alloc.LDQ_index = LDQ_tail;
  LDQ_num++;
  LDQ_tail = (LDQ_tail+1) % knobs->exec.LDQ_size;
}

/* called by commit */
void core_exec_DPM_t::LDQ_deallocate(struct uop_t * uop)
{
  struct core_knobs_t * knobs = core->knobs;
  LDQ[LDQ_head].uop = NULL;
  LDQ_num --;
  LDQ_head = (LDQ_head+1) % knobs->exec.LDQ_size;
  uop->alloc.LDQ_index = -1;
}

void core_exec_DPM_t::LDQ_squash(struct uop_t * dead_uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert((dead_uop->alloc.LDQ_index >= 0) && (dead_uop->alloc.LDQ_index < knobs->exec.LDQ_size),(void)0);
  zesto_assert(LDQ[dead_uop->alloc.LDQ_index].uop == dead_uop,(void)0);
  //memset(&LDQ[dead_uop->alloc.LDQ_index],0,sizeof(LDQ[0]));
  memzero(&LDQ[dead_uop->alloc.LDQ_index],sizeof(LDQ[0]));
  LDQ_num --;
  LDQ_tail = (LDQ_tail - 1 + knobs->exec.LDQ_size) % knobs->exec.LDQ_size;
  zesto_assert(LDQ_num >= 0,(void)0);
  dead_uop->alloc.LDQ_index = -1;
}

bool core_exec_DPM_t::STQ_available(void)
{
  struct core_knobs_t * knobs = core->knobs;
  return STQ_senior_num < knobs->exec.STQ_size;
}

void core_exec_DPM_t::STQ_insert_sta(struct uop_t * uop)
{
  struct core_knobs_t * knobs = core->knobs;
  //memset(&STQ[STQ_tail],0,sizeof(*STQ));
  memzero(&STQ[STQ_tail],sizeof(*STQ));
  STQ[STQ_tail].sta = uop;
  if(STQ[STQ_tail].sta != NULL)
    uop->decode.is_sta = true;
  STQ[STQ_tail].mem_size = uop->decode.mem_size;
  STQ[STQ_tail].uop_seq = uop->decode.uop_seq;
  STQ[STQ_tail].next_load = LDQ_tail;
  uop->alloc.STQ_index = STQ_tail;
  STQ_num++;
  STQ_senior_num++;
  STQ_tail = (STQ_tail+1) % knobs->exec.STQ_size;
}

void core_exec_DPM_t::STQ_insert_std(struct uop_t * uop)
{
  struct core_knobs_t * knobs = core->knobs;
  /* STQ_tail already incremented from the STA.  Just add this uop to STQ->std */
  int index = (STQ_tail - 1 + knobs->exec.STQ_size) % knobs->exec.STQ_size;
  uop->alloc.STQ_index = index;
  STQ[index].std = uop;
  zesto_assert(STQ[index].sta,(void)0); /* shouldn't have STD w/o a corresponding STA */
  zesto_assert(STQ[index].sta->Mop == uop->Mop,(void)0); /* and we should be from the same macro */
}

void core_exec_DPM_t::STQ_deallocate_sta(void)
{
  STQ[STQ_head].sta = NULL;
}

/* returns true if successful */
bool core_exec_DPM_t::STQ_deallocate_std(struct uop_t * uop)
{
  struct core_knobs_t * knobs = core->knobs;
  /* Store write back occurs here at commit.  NOTE: stores go directly to
     DTLB2 (See "Intel 64 and IA-32 Architectures Optimization Reference
     Manual"). */
  if(!cache_enqueuable(core->memory.DL1,core->current_thread->id,uop->oracle.virt_addr) ||
     !cache_enqueuable(core->memory.DTLB2,core->current_thread->id,PAGE_TABLE_ADDR(core->current_thread->id,uop->oracle.virt_addr)))
    return false;

  /* These are just dummy placeholders, but we need them
     because the original uop pointers will be made invalid
     when the Mop commits and the oracle reclaims the
     original uop-arrays.  The store continues to occupy STQ
     entries until the store actually finishes accessing
     memory, but commit can proceed past this store once the
     request has entered into the cache hierarchy. */
  struct uop_t * dl1_uop = core->get_uop_array(1);
  struct uop_t * dtlb_uop = core->get_uop_array(1);
  dl1_uop->core = core;
  dtlb_uop->core = core;

  dl1_uop->alloc.STQ_index = uop->alloc.STQ_index;
  dtlb_uop->alloc.STQ_index = uop->alloc.STQ_index;

  STQ[STQ_head].action_id = core->new_action_id();
  dl1_uop->exec.action_id = STQ[STQ_head].action_id;
  dtlb_uop->exec.action_id = dl1_uop->exec.action_id;

  cache_enqueue(core,core->memory.DTLB2,NULL,CACHE_READ,core->current_thread->id,uop->Mop->fetch.PC,PAGE_TABLE_ADDR(core->current_thread->id,uop->oracle.virt_addr),dtlb_uop->exec.action_id,0,NO_MSHR,dtlb_uop,store_dtlb_callback,NULL,NULL,get_uop_action_id);
  cache_enqueue(core,core->memory.DL1,NULL,CACHE_WRITE,core->current_thread->id,uop->Mop->fetch.PC,uop->oracle.virt_addr,dl1_uop->exec.action_id,0,NO_MSHR,dl1_uop,store_dl1_callback,NULL,store_translated_callback,get_uop_action_id);

  STQ[STQ_head].std = NULL;
  STQ[STQ_head].translation_complete = false;
  STQ[STQ_head].write_complete = false;
  STQ_num --;
  STQ_head = (STQ_head+1) % knobs->exec.STQ_size;

  return true;
}

void core_exec_DPM_t::STQ_deallocate_senior(void)
{
  struct core_knobs_t * knobs = core->knobs;
  if(STQ[STQ_senior_head].write_complete &&
     STQ[STQ_senior_head].translation_complete)
  {
    STQ[STQ_senior_head].write_complete = false;
    STQ[STQ_senior_head].translation_complete = false;
    STQ_senior_head = (STQ_senior_head + 1) % knobs->exec.STQ_size;
    STQ_senior_num--;
    zesto_assert(STQ_senior_num >= 0,(void)0);
  }
}

void core_exec_DPM_t::STQ_squash_sta(struct uop_t * dead_uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert((dead_uop->alloc.STQ_index >= 0) && (dead_uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
  zesto_assert(STQ[dead_uop->alloc.STQ_index].std == NULL,(void)0);
  zesto_assert(STQ[dead_uop->alloc.STQ_index].sta == dead_uop,(void)0);
  //memset(&STQ[dead_uop->alloc.STQ_index],0,sizeof(STQ[0]));
  memzero(&STQ[dead_uop->alloc.STQ_index],sizeof(STQ[0]));
  STQ_num --;
  STQ_senior_num --;
  STQ_tail = (STQ_tail - 1 + knobs->exec.STQ_size) % knobs->exec.STQ_size;
  zesto_assert(STQ_num >= 0,(void)0);
  zesto_assert(STQ_senior_num >= 0,(void)0);
  dead_uop->alloc.STQ_index = -1;
}

void core_exec_DPM_t::STQ_squash_std(struct uop_t * dead_uop)
{
  struct core_knobs_t * knobs = core->knobs;
  zesto_assert((dead_uop->alloc.STQ_index >= 0) && (dead_uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
  zesto_assert(STQ[dead_uop->alloc.STQ_index].std == dead_uop,(void)0);
  STQ[dead_uop->alloc.STQ_index].std = NULL;
  dead_uop->alloc.STQ_index = -1;
}

void core_exec_DPM_t::STQ_squash_senior(void)
{
  struct core_knobs_t * knobs = core->knobs;
  while(STQ_senior_num > 0)
  {
    STQ[STQ_senior_head].write_complete = false;
    STQ[STQ_senior_head].translation_complete = false;
    STQ[STQ_senior_head].action_id = core->new_action_id();

    if(STQ_senior_head == STQ_head)
    {
      STQ_head = (STQ_head + 1) % knobs->exec.STQ_size;
      STQ_num--;
    }
    STQ_senior_head = (STQ_senior_head + 1) % knobs->exec.STQ_size;
    STQ_senior_num--;
  }
}

void core_exec_DPM_t::recover_check_assertions(void)
{
  zesto_assert(STQ_senior_num == 0,(void)0);
  zesto_assert(STQ_num == 0,(void)0);
  zesto_assert(LDQ_num == 0,(void)0);
}

/* Stores don't write back to cache/memory until commit.  When D$
   and DTLB accesses complete, these functions get called which
   update the status of the corresponding STQ entries.  The STQ
   entry cannot be deallocated until the store has completed. */
void core_exec_DPM_t::store_dl1_callback(void * op)
{
  struct uop_t * uop = (struct uop_t *)op;
  struct core_t * core = uop->core;
  struct core_exec_DPM_t * E = (core_exec_DPM_t*)core->exec;
  struct core_knobs_t * knobs = core->knobs;

  zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
  if(uop->exec.action_id == E->STQ[uop->alloc.STQ_index].action_id)
    E->STQ[uop->alloc.STQ_index].write_complete = true;
  core->return_uop_array(uop);
}

void core_exec_DPM_t::store_dtlb_callback(void * op)
{
  struct uop_t * uop = (struct uop_t *)op;
  struct core_t * core = uop->core;
  struct core_exec_DPM_t * E = (core_exec_DPM_t*)core->exec;
  struct core_knobs_t * knobs = core->knobs;

  zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),(void)0);
  if(uop->exec.action_id == E->STQ[uop->alloc.STQ_index].action_id)
    E->STQ[uop->alloc.STQ_index].translation_complete = true;
  core->return_uop_array(uop);
}

bool core_exec_DPM_t::store_translated_callback(void * op, seq_t action_id /* ignored */)
{
  struct uop_t * uop = (struct uop_t *)op;
  struct core_t * core = uop->core;
  struct core_knobs_t * knobs = core->knobs;
  struct core_exec_DPM_t * E = (core_exec_DPM_t*)core->exec;

  if(uop->alloc.STQ_index == -1)
    return true;
  zesto_assert((uop->alloc.STQ_index >= 0) && (uop->alloc.STQ_index < knobs->exec.STQ_size),true);
  return E->STQ[uop->alloc.STQ_index].translation_complete;
}

#endif
