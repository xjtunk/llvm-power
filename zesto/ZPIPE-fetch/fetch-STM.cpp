/* fetch-STM.cpp - Simple(r) Timing Model */
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
  if(!strcasecmp(fetch_opt_string,"STM"))
    return new core_fetch_STM_t(core);
#else

class core_fetch_STM_t:public core_fetch_t
{
  enum fetch_stall_t {FSTALL_byteQ_FULL, /* byteQ is full */
                      FSTALL_TBR,      /* predicted taken */
                      FSTALL_EOL,      /* hit end of cache line */
                      FSTALL_BOGUS,    /* encountered invalid inst on wrong-path */
                      FSTALL_SYSCALL,  /* syscall waiting for pipe to clear */
                      FSTALL_ZPAGE,    /* fetch request from zeroth page of memory */
                      FSTALL_num
                     };

  public:

  /* constructor, stats registration */
  core_fetch_STM_t(struct core_t * core);
  virtual void reg_stats(struct stat_sdb_t *sdb);
  virtual void update_occupancy(void);

  /* simulate one cycle */
  virtual void step(void);

  /* decode interface */
  virtual bool Mop_available(void);
  virtual struct Mop_t * Mop_peek(void);
  virtual void Mop_consume(void);

  /* enqueue a jeclear event into the jeclear_pipe */
  virtual void jeclear_enqueue(struct Mop_t * Mop, md_addr_t New_PC);
  /* recover the front-end after the recover request actually
     reaches the front end. */
  virtual void recover(md_addr_t new_PC); 

  protected:

  int byteQ_num;

  /* The byteQ contains the raw bytes (well, the addresses thereof).
     This is similar to the old IFQ in that the contents of the I$
     are delivered here.  Each entry contains raw bytes, and so
     this may correspond to many, or even less than one, instruction
     per entry. */
  struct byteQ_entry_t {
    md_addr_t addr;
    tick_t when_fetch_requested;
    tick_t when_fetched;
    tick_t when_translation_requested;
    tick_t when_translated;
    int num_Mop;
    int MopQ_first_index;
    seq_t action_id;
  } * byteQ;
  int byteQ_linemask; /* for masking out line offset */
  int byteQ_head;
  int byteQ_tail;

  static const char *fetch_stall_str[FSTALL_num];

  void byteQ_request(md_addr_t lineaddr);
  bool byteQ_is_full(void);
  bool byteQ_already_requested(md_addr_t addr);

  static void IL1_callback(void * op);
  static void ITLB_callback(void * op);
  static bool translated_callback(void * op, seq_t action_id);
  static seq_t get_byteQ_action_id(void * op);

};

const char *core_fetch_STM_t::fetch_stall_str[FSTALL_num] = {
  "byteQ full             ",
  "taken branch           ",
  "end of cache line      ",
  "wrong-path invalid inst",
  "trap waiting on drain  ",
  "request for page zero  "
};


core_fetch_STM_t::core_fetch_STM_t(struct core_t * arg_core):
  byteQ_num(0)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;

  /* must come after exec_init()! */
  char name[256];
  int sets, assoc, linesize, latency, banks, bank_width, MSHR_entries;
  char rp;
  int i;

  /* bpred */
  bpred = new bpred_t(
      knobs->fetch.num_bpred_components,
      knobs->fetch.bpred_opt_str,
      knobs->fetch.fusion_opt_str,
      knobs->fetch.dirjmpbtb_opt_str,
      knobs->fetch.indirjmpbtb_opt_str,
      knobs->fetch.ras_opt_str
    );

  if(knobs->fetch.jeclear_delay != 0)
  {
    warn("STM fetch model does not support non-zero latency jeclear delay... setting to zero");
    knobs->fetch.jeclear_delay = 0;
  }

  /* IL1 */
  if(sscanf(knobs->memory.IL1_opt_str,"%[^:]:%d:%d:%d:%d:%d:%d:%c:%d",
      name,&sets,&assoc,&linesize,&banks,&bank_width,&latency, &rp, &MSHR_entries) != 9)
    fatal("invalid IL1 options: <name:sets:assoc:linesize:banks:bank_width:latency:repl-policy:num-MSHR>\n\t(%s)",knobs->memory.IL1_opt_str);

  /* the write-related options don't matter since the IL1 will(should) never see any stores */
  core->memory.IL1 = cache_create(core,name,CACHE_READONLY,sets,assoc,linesize,rp,'w','t','n',banks,bank_width,latency,4,MSHR_entries,1,uncore->LLC,uncore->LLC_bus);

  core->memory.IL1->PFF_size = knobs->memory.IL1_PFFsize;
  core->memory.IL1->PFF = (cache_t::PFF_t *) calloc(knobs->memory.IL1_PFFsize,sizeof(*core->memory.IL1->PFF));
  if(!core->memory.IL1->PFF)
    fatal("failed to calloc %s's prefetch FIFO",core->memory.IL1->name);
  //prefetch_buffer_create(core->memory.IL1,knobs->memory.IL1_PF_buffer_size);
  //prefetch_filter_create(core->memory.IL1,knobs->memory.IL1_PF_filter_size,knobs->memory.IL1_PF_filter_reset);
  core->memory.IL1->prefetch_threshold = knobs->memory.IL1_PFthresh;
  core->memory.IL1->prefetch_max = knobs->memory.IL1_PFmax;
  core->memory.IL1->PF_low_watermark = knobs->memory.IL1_low_watermark;
  core->memory.IL1->PF_high_watermark = knobs->memory.IL1_high_watermark;
  core->memory.IL1->PF_sample_interval = knobs->memory.IL1_WMinterval;

  core->memory.IL1->prefetcher = (struct prefetch_t **) calloc(knobs->memory.IL1_num_PF,sizeof(*core->memory.IL1->prefetcher));
  core->memory.IL1->num_prefetchers = knobs->memory.IL1_num_PF;
  if(!core->memory.IL1->prefetcher)
    fatal("couldn't calloc %s's prefetcher array",core->memory.IL1->name);
  for(i=0;i<knobs->memory.IL1_num_PF;i++)
    core->memory.IL1->prefetcher[i] = prefetch_create(knobs->memory.IL1PF_opt_str[i],core->memory.IL1);
  if(core->memory.IL1->prefetcher[0] == NULL)
    core->memory.IL1->num_prefetchers = knobs->memory.IL1_num_PF = 0;

  /* ITLB */
  if(sscanf(knobs->memory.ITLB_opt_str,"%[^:]:%d:%d:%d:%d:%c:%d",
      name,&sets,&assoc,&banks,&latency, &rp, &MSHR_entries) != 7)
    fatal("invalid ITLB options: <name:sets:assoc:banks:latency:repl-policy:num-MSHR>");

  core->memory.ITLB = cache_create(core,name,CACHE_READONLY,sets,assoc,1,rp,'w','t','n',banks,1,latency,4,MSHR_entries,1,uncore->LLC,uncore->LLC_bus);

  byteQ = (byteQ_entry_t*) calloc(knobs->fetch.byteQ_size,sizeof(*byteQ));
  byteQ_head = byteQ_tail = 0;
  if(!byteQ)
    fatal("couldn't calloc byteQ");

  byteQ_linemask = ~(md_addr_t)(knobs->fetch.byteQ_linesize - 1);

}

void
core_fetch_STM_t::reg_stats(struct stat_sdb_t *sdb)
{
  char buf[1024];
  char buf2[1024];
  struct thread_t * arch = core->current_thread;

  stat_reg_note(sdb,"\n#### BPRED STATS ####");
  bpred->reg_stats(sdb,core);

  stat_reg_note(sdb,"\n#### INST CACHE STATS ####");
  cache_reg_stats(sdb, core, core->memory.IL1);
  cache_reg_stats(sdb, core, core->memory.ITLB);

  /* TODO: move stats into core_fetch_STM_t */
  stat_reg_note(sdb,"\n#### FETCH STATS ####");
  sprintf(buf,"c%d.fetch_insn",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions fetched", &core->stat.fetch_insn, core->stat.fetch_insn, NULL);
  sprintf(buf,"c%d.fetch_uops",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of uops fetched", &core->stat.fetch_uops, core->stat.fetch_uops, NULL);
  sprintf(buf,"c%d.fetch_IPC",arch->id);
  sprintf(buf2,"c%d.fetch_insn/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "IPC at fetch", buf2, NULL);
  sprintf(buf,"c%d.fetch_uPC",arch->id);
  sprintf(buf2,"c%d.fetch_uops/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "uPC at fetch", buf2, NULL);

  sprintf(buf,"c%d.fetch_stall",core->current_thread->id);
  core->stat.fetch_stall = stat_reg_dist(sdb, buf,
                                          "breakdown of stalls in fetch",
                                          /* initial value */0,
                                          /* array size */FSTALL_num,
                                          /* bucket size */1,
                                          /* print format */(PF_COUNT|PF_PDF),
                                          /* format */NULL,
                                          /* index map */fetch_stall_str,
                                          /* print fn */NULL);

  sprintf(buf,"c%d.byteQ_occupancy",arch->id);
  stat_reg_counter(sdb, false, buf, "total byteQ occupancy (in lines/entries)", &core->stat.byteQ_occupancy, core->stat.byteQ_occupancy, NULL);
  sprintf(buf,"c%d.byteQ_avg",arch->id);
  sprintf(buf2,"c%d.byteQ_occupancy/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "average byteQ occupancy (in insts)", buf2, NULL);
}

void core_fetch_STM_t::update_occupancy(void)
{
  core->stat.byteQ_occupancy += byteQ_num;
}


/******************************/
/* byteQ/I$ RELATED FUNCTIONS */
/******************************/

bool core_fetch_STM_t::byteQ_is_full(void)
{
  struct core_knobs_t * knobs = core->knobs;
  return (byteQ_num >= knobs->fetch.byteQ_size);
}

/* is this line already the most recently requested? */
bool core_fetch_STM_t::byteQ_already_requested(md_addr_t addr)
{
  struct core_knobs_t * knobs = core->knobs;
  int index = (byteQ_tail-1+knobs->fetch.byteQ_size) % knobs->fetch.byteQ_size;
  if(byteQ_num && (byteQ[index].addr == (addr & byteQ_linemask)))
    return true;
  else
    return false;
}

/* Initiate a fetch request */
void core_fetch_STM_t::byteQ_request(md_addr_t lineaddr)
{
  struct core_knobs_t * knobs = core->knobs;
  /* this function assumes you already called byteQ_already_requested so that
     you don't double-request the same line */
  byteQ[byteQ_tail].when_fetch_requested = TICK_T_MAX;
  byteQ[byteQ_tail].when_fetched = TICK_T_MAX;
  byteQ[byteQ_tail].when_translation_requested = TICK_T_MAX;
  byteQ[byteQ_tail].when_translated = TICK_T_MAX;
  byteQ[byteQ_tail].addr = lineaddr;
  byteQ[byteQ_tail].action_id = core->new_action_id();
  byteQ_tail = (byteQ_tail+1)%knobs->fetch.byteQ_size;
  byteQ_num++;

  return;
}

/* Callback functions used by the cache code to indicate when the IL1/ITLB lookups
   have been completed. */
void core_fetch_STM_t::IL1_callback(void * op)
{
  byteQ_entry_t * byteQ = (byteQ_entry_t*) op;
  byteQ->when_fetched = sim_cycle;
}

void core_fetch_STM_t::ITLB_callback(void * op)
{
  byteQ_entry_t * byteQ = (byteQ_entry_t*) op;
  byteQ->when_translated = sim_cycle;
}

bool core_fetch_STM_t::translated_callback(void * op, seq_t action_id)
{
  byteQ_entry_t * byteQ = (byteQ_entry_t*) op;
  if(byteQ->action_id == action_id)
    return byteQ->when_translated <= sim_cycle;
  else
    return true;
}

seq_t core_fetch_STM_t::get_byteQ_action_id(void * op)
{
  byteQ_entry_t * byteQ = (byteQ_entry_t*) op;
  return byteQ->action_id;
}

/************************/
/* MAIN FETCH FUNCTIONS */
/************************/

void core_fetch_STM_t::step(void)
{
  struct core_knobs_t * knobs = core->knobs;
  md_addr_t current_line = PC & byteQ_linemask;
  struct Mop_t * Mop = NULL;
  enum fetch_stall_t stall_reason = FSTALL_EOL;
  int i;

  /* This gets processed here, so that demand misses from the DL1 get higher
     priority for accessing the L2 */
  cache_process(core->memory.ITLB);
  cache_process(core->memory.IL1);

  /* XXX: I don't think we really should need this code here, since
     whenever a Mop gets consumed, this check should be happening
     any way (see the Mop_consume function)... but the simulator
     seems to get stuck with a line where all Mops have been read
     yet has not been reclaimed. */
  if(byteQ_num && (byteQ[byteQ_head].when_fetched != TICK_T_MAX) 
                  && (byteQ[byteQ_head].when_translated != TICK_T_MAX)
                  && (byteQ[byteQ_head].num_Mop <= 0))
  {
    /* consumed all insts from this fetch line */
    byteQ[byteQ_head].MopQ_first_index = -1;
    byteQ[byteQ_head].when_fetch_requested = TICK_T_MAX;
    byteQ[byteQ_head].when_fetched = TICK_T_MAX;
    byteQ[byteQ_head].when_translation_requested = TICK_T_MAX;
    byteQ[byteQ_head].when_translated = TICK_T_MAX;
    byteQ_num--;
    byteQ_head = (byteQ_head+1)%knobs->fetch.byteQ_size;
  }


  while( /* still fetching from the same byteQ entry */
         (PC & byteQ_linemask) == current_line
       )
  {
    Mop = core->oracle->exec(PC);
    if(Mop && ((PC >> PAGE_SHIFT) == 0))
    {
      zesto_assert(core->oracle->spec_mode,(void)0);
      stall_reason = FSTALL_ZPAGE;
      break;
    }

    if(!Mop) /* awaiting pipe to clear for system call/trap, or encountered wrong-path bogus inst */
    {
      if(bogus)
        stall_reason = FSTALL_BOGUS;
      else
        stall_reason = FSTALL_SYSCALL;
      break;
    }

    /* We explicitly check for both the address of the first byte and the last
       byte, since x86 instructions have no alignment restrictions and therefore
       may end up getting split across more than one cache line.  If so, this
       can generate more than one IL1/ITLB lookup. */
    if(byteQ_already_requested(Mop->fetch.PC))
    {
      Mop->fetch.first_byte_requested = true;
      if(Mop->timing.when_fetch_started == TICK_T_MAX)
        Mop->timing.when_fetch_started = sim_cycle;
    }
    if(!Mop->fetch.first_byte_requested)
    {
      if(byteQ_is_full()) /* stall if byteQ is full */
      {
        stall_reason = FSTALL_byteQ_FULL;
        break;
      }

      if(Mop->timing.when_fetch_started == TICK_T_MAX)
        Mop->timing.when_fetch_started = sim_cycle;
      Mop->fetch.first_byte_requested = true;
      byteQ_request(Mop->fetch.PC & byteQ_linemask);
    }

    /* STM model doesn't deal with fetches across cache lines */
    Mop->fetch.last_byte_requested = true;

    zesto_assert(Mop->fetch.first_byte_requested,(void)0);

    /* All bytes for this Mop have been requested.  Record it in the byteQ entry
       and let the oracle know we're done with it so can proceed to the next
       one */

    int byteQ_index = (byteQ_tail-1+knobs->fetch.byteQ_size)%knobs->fetch.byteQ_size;
    if(byteQ[byteQ_index].num_Mop == 0)
      byteQ[byteQ_index].MopQ_first_index = core->oracle->get_index(Mop);
    byteQ[byteQ_index].num_Mop++;

    core->oracle->consume(Mop);

    /* figure out where to fetch from next */
    if(Mop->decode.is_ctrl || Mop->fetch.inst.rep)
    {
      Mop->fetch.bpred_update = bpred->get_state_cache();

      Mop->fetch.pred_NPC = bpred->lookup(Mop->fetch.bpred_update,
          Mop->decode.opflags, Mop->fetch.PC,Mop->fetch.PC+Mop->fetch.inst.len,Mop->decode.targetPC,
          Mop->oracle.NextPC,(Mop->oracle.NextPC != (Mop->fetch.PC+Mop->fetch.inst.len)));


      bpred->spec_update(Mop->fetch.bpred_update,Mop->decode.opflags,
          Mop->fetch.PC,Mop->decode.targetPC,Mop->oracle.NextPC,Mop->fetch.bpred_update->our_pred);
    }
    else
      Mop->fetch.pred_NPC = Mop->fetch.PC + Mop->fetch.inst.len;

    if(Mop->fetch.pred_NPC != Mop->oracle.NextPC)
    {
      Mop->oracle.recover_inst = true;
      Mop->uop[Mop->decode.last_uop_index].oracle.recover_inst = true;
    }

    /* advance the fetch PC to the next instruction */
    PC = Mop->fetch.pred_NPC;

    if(  (Mop->fetch.pred_NPC != (Mop->fetch.PC + Mop->fetch.inst.len))
      && (Mop->fetch.pred_NPC != Mop->fetch.PC)) /* REPs don't count as taken branches w.r.t. fetching */
    {
      stall_reason = FSTALL_TBR;
      break;
    }
    else if((Mop->fetch.PC & byteQ_linemask) != current_line)
    {
      stall_reason = FSTALL_EOL;
      break;
    }

  } /* while */

  ZESTO_STAT(stat_add_sample(core->stat.fetch_stall, (int)stall_reason);)

  /* check byteQ and send requests to I$/ITLB */
  for(i=0;i<byteQ_num;i++)
  {
    int index = (byteQ_head+i)%knobs->fetch.byteQ_size;
    if(byteQ[index].when_fetch_requested == TICK_T_MAX)
    {
      if(cache_enqueuable(core->memory.IL1,core->current_thread->id,byteQ[index].addr))
      {
        cache_enqueue(core,core->memory.IL1,NULL,CACHE_READ,core->current_thread->id,byteQ[index].addr,byteQ[index].addr,byteQ[index].action_id,0,NO_MSHR,&byteQ[index],IL1_callback,NULL,translated_callback,get_byteQ_action_id);
        byteQ[index].when_fetch_requested = sim_cycle;
        break;
      }
    }
  }

  for(i=0;i<byteQ_num;i++)
  {
    int index = (byteQ_head+i)%knobs->fetch.byteQ_size;
    if(byteQ[index].when_translation_requested == TICK_T_MAX)
    {
      if(cache_enqueuable(core->memory.ITLB,core->current_thread->id,PAGE_TABLE_ADDR(core->current_thread->id,byteQ[index].addr)))
      {
        cache_enqueue(core,core->memory.ITLB,NULL,CACHE_READ,0,core->current_thread->id,PAGE_TABLE_ADDR(core->current_thread->id,byteQ[index].addr),byteQ[index].action_id,0,NO_MSHR,&byteQ[index],ITLB_callback,NULL,NULL,get_byteQ_action_id);
        byteQ[index].when_translation_requested = sim_cycle;
        break;
      }
    }
  }
}



bool core_fetch_STM_t::Mop_available(void)
{
  return byteQ_num && byteQ[byteQ_head].when_fetched != TICK_T_MAX
                   && byteQ[byteQ_head].when_translated != TICK_T_MAX
                   && byteQ[byteQ_head].num_Mop;
}

struct Mop_t * core_fetch_STM_t::Mop_peek(void)
{
  int MopQ_index = byteQ[byteQ_head].MopQ_first_index;
  zesto_assert(MopQ_index != -1,NULL);
  return core->oracle->get_Mop(MopQ_index);
}

/* check byteQ for fetched/translated instructions */
void core_fetch_STM_t::Mop_consume(void)
{
  struct core_knobs_t * knobs = core->knobs;
  
  /* assumes you alerady called Mop_available to check that
     a ready Mop exists. */
  int MopQ_index = byteQ[byteQ_head].MopQ_first_index;
  struct Mop_t * Mop = core->oracle->get_Mop(MopQ_index);

  Mop->timing.when_fetched = sim_cycle;
  if(Mop->uop[Mop->decode.last_uop_index].decode.EOM)
    ZESTO_STAT(core->stat.fetch_insn++;)

  ZESTO_STAT(core->stat.fetch_uops += Mop->stat.num_uops;)

  byteQ[byteQ_head].num_Mop--;
  byteQ[byteQ_head].MopQ_first_index = core->oracle->next_index(MopQ_index);

  if(byteQ[byteQ_head].num_Mop <= 0)
  {
    /* consumed all insts from this fetch line */
    byteQ[byteQ_head].MopQ_first_index = -1;
    byteQ[byteQ_head].when_fetch_requested = TICK_T_MAX;
    byteQ[byteQ_head].when_fetched = TICK_T_MAX;
    byteQ[byteQ_head].when_translation_requested = TICK_T_MAX;
    byteQ[byteQ_head].when_translated = TICK_T_MAX;
    byteQ_num--;
    byteQ_head = (byteQ_head+1)%knobs->fetch.byteQ_size;
  }
}

void core_fetch_STM_t::jeclear_enqueue(struct Mop_t * Mop, md_addr_t New_PC)
{
  /* does nothing; should not get called for STM */
  fatal("core_fetch_SMT_t::jeclear_enqueue should never get called");
}

void
core_fetch_STM_t::recover(md_addr_t new_PC)
{
  struct core_knobs_t * knobs = core->knobs;
  /* XXX: use non-oracle nextPC as there may be multiple re-fetches */
  int i;
  PC = new_PC;
  bogus = false;

  /* clear out the byteQ */
  for(i=0;i<knobs->fetch.byteQ_size;i++)
  {
    byteQ[i].addr = 0;
    byteQ[i].when_fetch_requested = TICK_T_MAX;
    byteQ[i].when_fetched = TICK_T_MAX;
    byteQ[i].when_translation_requested = TICK_T_MAX;
    byteQ[i].when_translated = TICK_T_MAX;
    byteQ[i].num_Mop = 0;
    byteQ[i].MopQ_first_index = -1;
    byteQ[i].action_id = core->new_action_id();
  }
  byteQ_num = 0;
  byteQ_head = 0;
  byteQ_tail = 0;
}

#endif
