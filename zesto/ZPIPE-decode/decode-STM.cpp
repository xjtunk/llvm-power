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
  if(!strcasecmp(decode_opt_string,"STM"))
    return new core_decode_STM_t(core);
#else

class core_decode_STM_t:public core_decode_t
{
  enum decode_stall_t {DSTALL_NONE,   /* no stall */
                       DSTALL_FULL,   /* first decode stage is full */
                       DSTALL_EMPTY,  /* no insts to decode */
                       DSTALL_TARGET, /* target correction flushed remaining insts (only occurs when target_stage == crack_stage-1) */
                       DSTALL_num
                      };

  public:

  /* constructor, stats registration */
  core_decode_STM_t(struct core_t * core);
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

  static const char * decode_stall_str[DSTALL_num];

  enum decode_stall_t check_target(struct Mop_t * Mop);
  bool check_flush(int stage, int idx);
};

const char * core_decode_STM_t::decode_stall_str[DSTALL_num] = {
  "no stall                   ",
  "next decode stage is full  ",
  "no insts to decode         ",
  "target correction          ",
};

core_decode_STM_t::core_decode_STM_t(struct core_t * arg_core)
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
}

void
core_decode_STM_t::reg_stats(struct stat_sdb_t *sdb)
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

void core_decode_STM_t::update_occupancy(void)
{
}


/*************************/
/* MAIN DECODE FUNCTIONS */
/*************************/

/* Helper functions to check for BAC-related resteers */

/* Check an individual Mop to see if it's next target is correct/reasonable, and
   recover if necessary. */
enum core_decode_STM_t::decode_stall_t
core_decode_STM_t::check_target(struct Mop_t * Mop)
{
  if(Mop->decode.is_ctrl)
  {
    if((Mop->fetch.pred_NPC != (Mop->fetch.PC + Mop->fetch.inst.len)) /* branch is predicted taken */
        || (Mop->decode.opflags | F_UNCOND))
    {
      if(Mop->fetch.pred_NPC != Mop->decode.targetPC) /* wrong target */
      {
        if(Mop->fetch.bpred_update)
          core->fetch->bpred->recover(Mop->fetch.bpred_update,/*taken*/1);
        ZESTO_STAT(core->stat.target_resteers++;)
        core->oracle->recover(Mop);
        recover(Mop);
        core->fetch->recover(Mop->decode.targetPC);
        Mop->fetch.pred_NPC = Mop->decode.targetPC;
        return DSTALL_TARGET;
      }
    }
  }
  return DSTALL_NONE;
}

/* Perform branch-address calculation check for a given decode-pipe stage
   and Mop position. */
bool core_decode_STM_t::check_flush(int stage, int idx)
{
  struct core_knobs_t * knobs = core->knobs;
  /* stage-1 because we assume the following checks are acted upon only after
     the instruction has spent a cycle in the corresponding pipeline stage. */
  struct Mop_t * Mop = pipe[stage-1][idx];
  enum decode_stall_t stall_reason;

  if(Mop)
  {
    if((stage-1) == knobs->decode.target_stage)
    {
      if((stall_reason = check_target(Mop)))
      {
        pipe[stage][idx] = pipe[stage-1][idx];
        pipe[stage-1][idx] = NULL;
        if(pipe[stage][idx]) {
          occupancy[stage]++;
          occupancy[stage-1]--;
          zesto_assert(occupancy[stage] <= knobs->decode.width,false);
          zesto_assert(occupancy[stage-1] >= 0,false);
        }
        ZESTO_STAT(stat_add_sample(core->stat.decode_stall, (int)stall_reason);)
        return true;
      }
    }
  }

  return false;
}


/* shuffle Mops down the decode pipe, read Mops from fetch */
void core_decode_STM_t::step(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int stage, i;
  enum decode_stall_t stall_reason = DSTALL_NONE;

  /* walk pipe backwards up to and but not including the first stage*/
  for(stage=knobs->decode.depth-1; stage > 0; stage--)
  {
    if(0 == occupancy[stage]) /* implementing non-serpentine pipe (no compressing) - can't advance until stage is empty */
    {
      zesto_assert(occupancy[stage] == 0,(void)0);

      /* move everyone from previous stage forward */
      if(occupancy[stage-1])
        for(i=0;i<knobs->decode.width;i++)
        {
          if(check_flush(stage,i))
            return;

          pipe[stage][i] = pipe[stage-1][i];
          pipe[stage-1][i] = NULL;
          if(pipe[stage][i]) {
            occupancy[stage]++;
            occupancy[stage-1]--;
            zesto_assert(occupancy[stage] <= knobs->decode.width,(void)0);
            zesto_assert(occupancy[stage-1] >= 0,(void)0);
          }
        }
    }
  }

  /* process the first stage, which reads Mops from fetch */
  if(!core->fetch->Mop_available())
  {
    stall_reason = DSTALL_EMPTY;
  }
  else if(occupancy[0] == 0) /* non-serpentine pipe; only decode if first stage is empty */
  {
    int Mops_decoded = 0;
    for(i=0;(i<knobs->decode.width) && core->fetch->Mop_available();i++)
    {
      if(pipe[0][i] == NULL) /* decoder available */
      {
        struct Mop_t * fetch_Mop = core->fetch->Mop_peek();

        /* consume the Mop from fetch */
        pipe[0][i] = fetch_Mop;
        occupancy[0]++;
        zesto_assert(occupancy[0] <= knobs->decode.width,(void)0);
        core->fetch->Mop_consume();
        Mops_decoded++;
      }
    }
    if(Mops_decoded == 0)
      stall_reason = DSTALL_FULL;
  }

  ZESTO_STAT(stat_add_sample(core->stat.decode_stall, (int)stall_reason);)
}

void
core_decode_STM_t::recover(struct Mop_t * Mop)
{
  struct core_knobs_t * knobs = core->knobs;
  /* walk pipe from youngest uop blowing everything away,
     stop if we encounter the recover-Mop */
  for(int stage=0;stage<knobs->decode.depth;stage++)
  {
    if(occupancy[stage])
      for(int i=knobs->decode.width-1;i>=0;i--)
      {
        if(pipe[stage][i])
        {
          if(pipe[stage][i] == Mop)
            return;
          else
          {
            pipe[stage][i] = NULL;
            occupancy[stage]--;
            zesto_assert(occupancy[stage] >= 0,(void)0);
          }
        }
      }
  }
}

/* same as above, but blow away the entire decode pipeline */
void
core_decode_STM_t::recover(void)
{
  struct core_knobs_t * knobs = core->knobs;
  for(int stage=0;stage<knobs->decode.depth;stage++)
  {
    if(occupancy[stage])
      for(int i=knobs->decode.width-1;i>=0;i--)
      {
        if(pipe[stage][i])
        {
          pipe[stage][i] = NULL;
          occupancy[stage]--;
          zesto_assert(occupancy[stage] >= 0,(void)0);
        }
      }
  }
}





bool core_decode_STM_t::uop_available(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int stage = knobs->decode.depth-1;
  return occupancy[stage] != 0; /* if last stage is not empty, a uop is available */
}

struct uop_t * core_decode_STM_t::uop_peek(void)
{
  struct core_knobs_t * knobs = core->knobs;
  struct uop_t * uop = NULL;
  int stage = knobs->decode.depth-1;


  /* assumes uop_available has already been called */
  for(int i=0;i<knobs->decode.width;i++)
  {
    if(pipe[stage][i])
    {
      struct Mop_t * Mop = pipe[stage][i];      /* Mop in current decoder */
      uop = &Mop->uop[Mop->decode.last_stage_index]; /* first non-queued uop */
      
      zesto_assert((!(uop->decode.opflags & F_STORE)) || uop->decode.is_std,NULL);
      zesto_assert((!(uop->decode.opflags & F_LOAD)) || uop->decode.is_load,NULL);

      uop->decode.Mop_seq = Mop->oracle.seq;
      uop->decode.uop_seq = (Mop->oracle.seq << UOP_SEQ_SHIFT) + uop->flow_index;

      zesto_assert(uop != NULL,NULL);
      return uop;
    }
  }

  fatal("should have been able to provide a uop, but none were found in decode");

}

void core_decode_STM_t::uop_consume(void)
{
  struct core_knobs_t * knobs = core->knobs;
  struct uop_t * uop = NULL;
  int stage = knobs->decode.depth-1;

  /* assumes uop_available has already been called */
  for(int i=0;i<knobs->decode.width;i++)
  {
    if(pipe[stage][i])
    {
      struct Mop_t * Mop = pipe[stage][i];      /* Mop in current decoder */
      uop = &Mop->uop[Mop->decode.last_stage_index]; /* first non-queued uop */
      Mop->decode.last_stage_index += uop->decode.has_imm ? 3 : 1;  /* increment the uop pointer to next uop */
      
      ZESTO_STAT(core->stat.decode_uops++;)
      uop->timing.when_decoded = sim_cycle;
      if(uop->decode.BOM)
        uop->Mop->timing.when_decode_started = sim_cycle;
      if(uop->decode.EOM)
        uop->Mop->timing.when_decode_finished = sim_cycle;

      if(Mop->decode.last_stage_index >= Mop->decode.flow_length)
      {
        if(Mop->uop[Mop->decode.last_uop_index].decode.EOM)
          ZESTO_STAT(core->stat.decode_insn++;)
        /* all uops dispatched, remove from decoder */
        pipe[stage][i] = NULL;
        occupancy[stage]--;
        zesto_assert(occupancy[stage] >= 0,(void)0);
      }

      zesto_assert(uop != NULL,(void)0);
      return;
    }
  }

  fatal("should have been able to provide a uop, but none were found in decode");

}

#endif
