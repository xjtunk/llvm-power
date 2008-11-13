/* decode-STM.cpp - Simple(r) Timing Model */
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
  if(!strcasecmp(decode_opt_string,"NAIVE"))
    return new core_decode_NAIVE_t(core);
#else

class core_decode_NAIVE_t:public core_decode_t
{
  enum decode_stall_t {DSTALL_NONE,   /* no stall */
                       DSTALL_FULL,   /* first decode stage is full */
                       DSTALL_EMPTY,  /* no insts to decode */
                       DSTALL_TARGET, /* target correction flushed remaining insts (only occurs when target_stage == crack_stage-1) */
                       DSTALL_num
                      };

  public:

  /* constructor, stats registration */
  core_decode_NAIVE_t(struct core_t * core);
  virtual void reg_stats(struct stat_sdb_t *sdb);
  virtual void update_occupancy(void);

  virtual void step(void);
  virtual void recover(void);
  virtual void recover(struct Mop_t * Mop);

  /* interface functions for alloc stage */
  virtual bool uop_available(void);
  virtual struct uop_t * uop_peek(void);
  virtual void uop_consume(void);

  protected:

  struct Mop_t *** pipe; /* the actual decode pipe */
  int * occupancy;

  struct Mop_t * current_Mop;
  int current_uop_index;
  struct uop_t * current_uop;
  struct uop_t * last_uop;
  int uop_latency;
  long long uop_finish_time;
  bool uop_requested;
  struct thread_t * current_thread;
  
  static const char * decode_stall_str[DSTALL_num];

  enum decode_stall_t check_target(struct Mop_t * Mop);
  bool check_flush(int stage, int idx);
};

const char * core_decode_NAIVE_t::decode_stall_str[DSTALL_num] = {
  "no stall                   ",
  "next decode stage is full  ",
  "no insts to decode         ",
  "target correction          ",
};

core_decode_NAIVE_t::core_decode_NAIVE_t(struct core_t * arg_core)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;

  if(knobs->decode.depth <= 0)
    fatal("decode pipe depth must be > 0");

  if((knobs->decode.width <= 0) || (knobs->decode.width > MAX_DECODE_WIDTH))
    fatal("decode pipe width must be > 0 and < %d (change MAX_DECODE_WIDTH if you want more)",MAX_DECODE_WIDTH);

  if(knobs->decode.target_stage <= 0 || knobs->decode.target_stage >= knobs->decode.depth)
    fatal("decode target resteer stage (%d) must be > 0, and less than decode pipe depth (currently set to %d)",knobs->decode.target_stage,knobs->decode.depth);

  /* if the pipe is N wide, we assume there are N decoders */
  pipe = (struct Mop_t***) calloc(knobs->decode.depth,sizeof(*pipe));
  if(!pipe)
    fatal("couldn't calloc decode pipe");

  for(int i=0;i<knobs->decode.depth;i++)
  {
    pipe[i] = (struct Mop_t**) calloc(knobs->decode.width,sizeof(**pipe));
    if(!pipe[i])
      fatal("couldn't calloc decode pipe stage");
  }

  occupancy = (int*) calloc(knobs->decode.depth,sizeof(*occupancy));
  if(!occupancy)
    fatal("couldn't calloc decode pipe occupancy array");

  knobs->decode.max_uops = (int*) calloc(knobs->decode.width,sizeof(*knobs->decode.max_uops));
  if(!knobs->decode.max_uops)
    fatal("couldn't calloc decode.max_uops");
  if(knobs->decode.width != knobs->decode.num_decoder_specs)
    fatal("number of decoder specifications must be equal to decode pipeline width");
  for(int i=0;i<knobs->decode.width;i++)
    knobs->decode.max_uops[i] = knobs->decode.decoders[i];

  if(knobs->decode.fusion_none)
    knobs->decode.fusion_mode = 0x00000000;
  else if(knobs->decode.fusion_all)
  {
    if(knobs->decode.fusion_all || knobs->decode.fusion_load_op || knobs->decode.fusion_sta_std || knobs->decode.fusion_partial)
      warn("uop fusion not supported in Simple Timing Model");
    knobs->decode.fusion_all = false;
    knobs->decode.fusion_load_op = false;
    knobs->decode.fusion_sta_std = false;
    knobs->decode.fusion_partial = false;
  }
  
  current_Mop = NULL;
  current_uop_index = 0;
  current_uop = NULL;
  last_uop = NULL;
  uop_latency = 0;
  uop_requested = false;
  current_thread = core->current_thread;
}

void
core_decode_NAIVE_t::reg_stats(struct stat_sdb_t *sdb)
{
  char buf[1024];
  char buf2[1024];
  struct thread_t * arch = core->current_thread;

  stat_reg_note(sdb,"\n#### DECODE STATS ####");
  sprintf(buf,"c%d.target_resteers",arch->id);
  stat_reg_counter(sdb, true, buf, "decode-time target resteers", &core->stat.target_resteers, core->stat.target_resteers, NULL);
  sprintf(buf,"c%d.decode_insn",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions decodeed", &core->stat.decode_insn, core->stat.decode_insn, NULL);
  sprintf(buf,"c%d.decode_uops",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of uops decodeed", &core->stat.decode_uops, core->stat.decode_uops, NULL);
  sprintf(buf,"c%d.decode_IPC",arch->id);
  sprintf(buf2,"c%d.decode_insn/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "IPC at decode", buf2, NULL);
  sprintf(buf,"c%d.decode_uPC",arch->id);
  sprintf(buf2,"c%d.decode_uops/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "uPC at decode", buf2, NULL);

  sprintf(buf,"c%d.decode_stall",core->current_thread->id);
  core->stat.decode_stall = stat_reg_dist(sdb, buf,
                                           "breakdown of stalls at decode",
                                           /* initial value */0,
                                           /* array size */DSTALL_num,
                                           /* bucket size */1,
                                           /* print format */(PF_COUNT|PF_PDF),
                                           /* format */NULL,
                                           /* index map */decode_stall_str,
                                           /* print fn */NULL);
  
}

void core_decode_NAIVE_t::update_occupancy(void)
{
}


/*************************/
/* MAIN DECODE FUNCTIONS */
/*************************/

/* Helper functions to check for BAC-related resteers */

/* Check an individual Mop to see if it's next target is correct/reasonable, and
   recover if necessary. */
enum core_decode_NAIVE_t::decode_stall_t
core_decode_NAIVE_t::check_target(struct Mop_t * Mop)
{
}

/* Perform branch-address calculation check for a given decode-pipe stage
   and Mop position. */
bool core_decode_NAIVE_t::check_flush(int stage, int idx)
{
}


/* shuffle Mops down the decode pipe, read Mops from fetch */
void core_decode_NAIVE_t::step(void)
{
  //fprintf(stdout, "step: %lld \n", sim_cycle);
  if (!current_Mop)
  {
    if (core->fetch->Mop_available())
      current_Mop = core->fetch->Mop_peek();
    else return;
  }

  current_uop = &current_Mop->uop[current_uop_index];
  if (!uop_requested)
  {
    uop_requested = true;
    uop_latency = core->knobs->exec.latency[current_uop->decode.FU_class];
    int dtlb_latency = 0, dl1_latency = 0;
    //fprintf(stdout, "alulatency=%d ", uop_latency);
    if(MD_OP_FLAGS(current_Mop->decode.op) & F_MEM)  //figure out the memory latency
    {
      //fprintf(stdout, "memOP ");
      dtlb_latency += core->memory.DTLB->latency;
      //fprintf(stdout, "dtlb=%d ", dtlb_latency);
      if(!cache_is_hit(core->memory.DTLB,CACHE_READ,PAGE_TABLE_ADDR(current_thread->id,current_uop->oracle.virt_addr)))
      {
	dtlb_latency += core->memory.DTLB2->latency;
	//fprintf(stdout, "dtlb2=%d ", dtlb_latency);
	struct cache_line_t * p = cache_get_evictee(core->memory.DTLB,PAGE_TABLE_ADDR(current_thread->id,current_uop->oracle.virt_addr));
	p->dirty = p->valid = FALSE;
	cache_insert_block(core->memory.DTLB,CACHE_READ,PAGE_TABLE_ADDR(current_thread->id,current_uop->oracle.virt_addr));
	if(!cache_is_hit(core->memory.DTLB2,CACHE_READ,PAGE_TABLE_ADDR(current_thread->id,current_uop->oracle.virt_addr)))
	{
	  dtlb_latency += dram->access(CACHE_READ, PAGE_TABLE_ADDR(current_thread->id, current_uop->oracle.virt_addr), core->memory.DTLB2->linesize);
	  //fprintf(stdout, "dramdtlb=%d ", dtlb_latency);
	  struct cache_line_t * p = cache_get_evictee(core->memory.DTLB2,PAGE_TABLE_ADDR(current_thread->id,current_uop->oracle.virt_addr));
	  p->dirty = p->valid = FALSE;
	  cache_insert_block(core->memory.DTLB2,CACHE_READ,PAGE_TABLE_ADDR(current_thread->id,current_uop->oracle.virt_addr));
	}
      }
      
      enum cache_command cmd = CACHE_READ;
      md_paddr_t paddr = v2p_translate(current_thread->id,current_uop->oracle.virt_addr);
      if(MD_OP_FLAGS(current_Mop->decode.op) & F_STORE)
	cmd = CACHE_WRITE;
      dl1_latency = core->memory.DL1->latency;
      if (core->memory.DL1->latency > dtlb_latency) uop_latency += dl1_latency; else uop_latency += dtlb_latency; //get MAX
      
      //fprintf(stdout, "EVEdl1=%d ", uop_latency);
      if(!cache_is_hit(core->memory.DL1,cmd,paddr))
      {
	uop_latency += uncore->LLC->latency;
	//fprintf(stdout, "LLC=%d ", uop_latency);
	struct cache_line_t * p = cache_get_evictee(core->memory.DL1,paddr);
	p->dirty = p->valid = FALSE;
	cache_insert_block(core->memory.DL1,cmd,paddr);
	if(!cache_is_hit(uncore->LLC,cmd,paddr))
	{
	  uop_latency += dram->access(cmd, paddr, uncore->LLC->linesize);
	  //fprintf(stdout, "dram=%d ", uop_latency);
	  struct cache_line_t * p = cache_get_evictee(uncore->LLC,paddr);
	  p->dirty = p->valid = FALSE;
	  cache_insert_block(uncore->LLC,cmd,paddr);
	}
      }
    }

    uop_latency--; //for a one-cycle instruction, we actually need this number to be 0, so at the next cycle, we can proceed to another
    uop_finish_time = sim_cycle + uop_latency;
    //fprintf(stdout, "uopltc=%d time=%lld\n", uop_latency, uop_finish_time);
  }

  
  {
    if (sim_cycle >= uop_finish_time)  //one uop finished
    {
      core->oracle->commit_uop(current_uop);
      ZESTO_STAT(core->stat.commit_uops++;);
      current_uop_index += current_uop->decode.has_imm ? 3 : 1;
      uop_requested = false;
      uop_finish_time = 0;
      if (current_uop_index >= current_Mop->decode.flow_length) //all uops in the Mop finished, commit the Mop
      {
	//fprintf(stdout, "commit Mop\n");
	core->fetch->Mop_consume();

	if(current_Mop->uop[current_Mop->decode.last_uop_index].decode.EOM)
	  ZESTO_STAT(core->stat.commit_insn++;);
	core->oracle->commit(current_Mop);
	current_Mop = NULL;
	current_uop_index = 0;
	
	if (max_insts && core->stat.commit_insn > max_insts) //all instructions finished, end the simulation
	{
	  core->stat.final_sim_cycle = sim_cycle;
	  if(max_insts && max_uops)
	    fprintf(stderr,"# Committed instruction/uop ");
	  else if(max_insts)
	    fprintf(stderr,"# Committed instruction ");
	  else
	    fprintf(stderr,"# Committed uop ");
	  fprintf(stderr,"limit reached for core %d.\n",core->current_thread->id);

	  simulated_processes_remaining--;
	  core->current_thread->active = false;
	  core->fetch->bpred->freeze_stats();
	  cache_freeze_stats(core);
	  /* start this core over */
	  if(simulated_processes_remaining <= 0)
	    longjmp(sim_exit_buf, /* exitcode + fudge */0 + 1);
	}
      }
    }
  }
}

void
core_decode_NAIVE_t::recover(struct Mop_t * Mop)
{
}

/* same as above, but blow away the entire decode pipeline */
void
core_decode_NAIVE_t::recover(void)
{
}





bool core_decode_NAIVE_t::uop_available(void)
{
}

struct uop_t * core_decode_NAIVE_t::uop_peek(void)
{
}

void core_decode_NAIVE_t::uop_consume(void)
{

}

#endif
