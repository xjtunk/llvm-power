/* alloc-DPM.cpp - Detailed Pipeline Model */
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
  if(!strcasecmp(alloc_opt_string,"DPM"))
    return new core_alloc_DPM_t(core);
#else

class core_alloc_DPM_t:public core_alloc_t
{
  enum alloc_stall_t {ASTALL_NONE,   /* no stall */
                      ASTALL_EMPTY,
                      ASTALL_ROB,
                      ASTALL_LDQ,
                      ASTALL_STQ,
                      ASTALL_RS,
                      ASTALL_DRAIN,
                      ASTALL_num
                     };

  public:

  core_alloc_DPM_t(struct core_t * core);
  virtual void reg_stats(struct stat_sdb_t * sdb);

  virtual void step(void);
  virtual void recover(void);
  virtual void recover(struct Mop_t * Mop);

  virtual void RS_deallocate(struct uop_t * uop);
  virtual void start_drain(void); /* prevent allocation from proceeding to exec */

  protected:

  struct uop_t *** pipe;
  int * occupancy;
  /* for load-balancing port binding */
  int * port_loading;

  static const char *alloc_stall_str[ASTALL_num];
};

const char *core_alloc_DPM_t::alloc_stall_str[ASTALL_num] = {
  "no stall                   ",
  "no uops to allocate        ",
  "ROB is full                ",
  "LDQ is full                ",
  "STQ is full                ",
  "RS is full                 ",
  "OOO core draining          "
};

/*******************/
/* SETUP FUNCTIONS */
/*******************/

core_alloc_DPM_t::core_alloc_DPM_t(struct core_t * arg_core)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;
  int i;

  if(knobs->alloc.depth <= 0)
    fatal("allocation pipeline depth must be > 0");
  if(knobs->alloc.width <= 0)
    fatal("allocation pipeline width must be > 0");

  pipe = (struct uop_t***) calloc(knobs->alloc.depth,sizeof(*pipe));
  if(!pipe)
    fatal("couldn't calloc alloc pipe");

  for(i=0;i<knobs->alloc.depth;i++)
  {
    pipe[i] = (struct uop_t**) calloc(knobs->alloc.width,sizeof(**pipe));
    if(!pipe[i])
      fatal("couldn't calloc alloc pipe stage");
  }

  occupancy = (int*) calloc(knobs->alloc.depth,sizeof(*occupancy));
  if(!occupancy)
    fatal("couldn't calloc alloc occupancy array");

  port_loading = (int*) calloc(knobs->exec.num_exec_ports,sizeof(*port_loading));
  if(!port_loading)
    fatal("couldn't calloc allocation port-loading scoreboard");
}

void
core_alloc_DPM_t::reg_stats(struct stat_sdb_t *sdb)
{
  char buf[1024];
  char buf2[1024];
  struct thread_t * arch = core->current_thread;

  stat_reg_note(sdb,"#### ALLOC STATS ####");
  sprintf(buf,"c%d.alloc_insn",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions alloced", &core->stat.alloc_insn, core->stat.alloc_insn, NULL);
  sprintf(buf,"c%d.alloc_uops",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of uops alloced", &core->stat.alloc_uops, core->stat.alloc_uops, NULL);
  sprintf(buf,"c%d.alloc_eff_uops",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of effective uops alloced", &core->stat.alloc_eff_uops, core->stat.alloc_eff_uops, NULL);
  sprintf(buf,"c%d.alloc_IPC",arch->id);
  sprintf(buf2,"c%d.alloc_insn/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "IPC at alloc", buf2, NULL);
  sprintf(buf,"c%d.alloc_uPC",arch->id);
  sprintf(buf2,"c%d.alloc_uops/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "uPC at alloc", buf2, NULL);
  sprintf(buf,"c%d.alloc_euPC",arch->id);
  sprintf(buf2,"c%d.alloc_eff_uops/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "effective uPC at alloc", buf2, NULL);

  sprintf(buf,"c%d.alloc_stall",core->current_thread->id);
  core->stat.alloc_stall = stat_reg_dist(sdb, buf,
                                          "breakdown of stalls at alloc",
                                          /* initial value */0,
                                          /* array size */ASTALL_num,
                                          /* bucket size */1,
                                          /* print format */(PF_COUNT|PF_PDF),
                                          /* format */NULL,
                                          /* index map */alloc_stall_str,
                                          /* print fn */NULL);
}


/************************/
/* MAIN ALLOC FUNCTIONS */
/************************/

void core_alloc_DPM_t::step(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int stage, i;
  enum alloc_stall_t stall_reason = ASTALL_NONE;

  /*========================================================================*/
  /*== Dispatch insts if ROB, RS, and LQ/SQ entries available (as needed) ==*/
  stage = knobs->alloc.depth-1;
  if(occupancy[stage]) /* are there uops in the last stage of the alloc pipe? */
  {
    for(i=0; i < knobs->alloc.width; i++) /* if so, scan all slots (width) of this stage */
    {
      struct uop_t * uop = pipe[stage][i];
      int abort_alloc = false;

      /* if using drain flush: */
      /* is the back-end still draining from a misprediction recovery? */
      if(knobs->alloc.drain_flush && drain_in_progress)
      {
        if(!core->commit->ROB_empty())
        {
          stall_reason = ASTALL_DRAIN;
          break;
        }
        else
          drain_in_progress = false;
      }

      if(uop)
      {
        while(uop) /* this while loop is to handle multiple uops fused together into the same slot */
        {
          if(uop->timing.when_allocated == TICK_T_MAX)
          {
            /* is the ROB full? */
            if(!core->commit->ROB_available())
            {
              stall_reason = ASTALL_ROB;
              abort_alloc = true;
              break;
            }

            /* for loads, is the LDQ full? */
            if(uop->decode.is_load && !core->exec->LDQ_available())
            {
              stall_reason = ASTALL_LDQ;
              abort_alloc = true;
              break;
            }
            /* for stores, allocate STQ entry on STA.  NOTE: This is different from 
               Bob Colwell's description in Shen&Lipasti Chap 7 where he describes
               allocation on STD.  We emit STA uops first since the oracle needs to
               use the STA result to feed the following STD uop. */
            if(uop->decode.is_sta && !core->exec->STQ_available())
            {
              stall_reason = ASTALL_STQ;
              abort_alloc = true;
              break;
            }

            /* is the RS full? -- don't need to alloc for NOP's */
            if(!core->exec->RS_available() && !uop->decode.is_nop)
            {
              stall_reason = ASTALL_RS;
              abort_alloc = true;
              break;
            }

            /* ALL ALLOC STALL CONDITIONS PASSED */

            /* place in ROB */
            if((!uop->decode.in_fusion) || uop->decode.is_fusion_head)
              core->commit->ROB_insert(uop);
            else /* fusion body doesn't occupy additional ROB entries */
              core->commit->ROB_fuse_insert(uop);

            /* place in LDQ/STQ if needed */
            if(uop->decode.is_load)
              core->exec->LDQ_insert(uop);
            else if(uop->decode.is_sta)
              core->exec->STQ_insert_sta(uop);
            else if(uop->decode.is_std)
              core->exec->STQ_insert_std(uop);

            /* all store uops had better be marked is_std */
            zesto_assert((!(uop->decode.opflags & F_STORE)) || uop->decode.is_std,(void)0);
            zesto_assert((!(uop->decode.opflags & F_LOAD)) || uop->decode.is_load,(void)0);

            /* port bindings */
            if(!uop->decode.is_nop && !uop->Mop->decode.is_trap)
            {
              /* port-binding is trivial when there's only one valid port */
              if(knobs->exec.port_binding[uop->decode.FU_class].num_FUs == 1)
              {
                uop->alloc.port_assignment = knobs->exec.port_binding[uop->decode.FU_class].ports[0];
              }
              else /* else assign uop to least loaded port */
              {
                int min_load = INT_MAX;
                int index = -1;
                for(int j=0;j<knobs->exec.port_binding[uop->decode.FU_class].num_FUs;j++)
                {
                  int port = knobs->exec.port_binding[uop->decode.FU_class].ports[j];
                  if(port_loading[port] < min_load)
                  {
                    min_load = port_loading[port];
                    index = port;
                  }
                }
                uop->alloc.port_assignment = index;
              }
              port_loading[uop->alloc.port_assignment]++;

              /* only allocate for non-fused or fusion-head */
              if((!uop->decode.in_fusion) || uop->decode.is_fusion_head)
                core->exec->RS_insert(uop);
              else
                core->exec->RS_fuse_insert(uop);

              /* Get input mappings - this is a proxy for explicit register numbers, which
                 you can always get from idep_uop->alloc.ROB_index */
              for(int j=0;j<MAX_IDEPS;j++)
              {
                /* This use of oracle info is valid: at this point the processor would be
                   looking up this information in the RAT, but this saves us having to
                   explicitly store/track the RAT state. */
                uop->exec.idep_uop[j] = uop->oracle.idep_uop[j];

                /* Add self onto parent's output list.  This output list doesn't
                   have a real microarchitectural counter part, but it makes the
                   simulation faster by not having to perform a whole mess of
                   associative searches each time any sort of broadcast is needed.
                   The parent's odep list only points to uops which have dispatched
                   into the OOO core (i.e. has left the alloc pipe). */
                if(uop->exec.idep_uop[j])
                {
                  struct odep_t * odep = core->get_odep_link();
                  odep->next = uop->exec.idep_uop[j]->exec.odep_uop;
                  uop->exec.idep_uop[j]->exec.odep_uop = odep;
                  odep->uop = uop;
                  odep->aflags = (uop->decode.idep_name[j] == DCREG(MD_REG_AFLAGS));
                  odep->op_num = j;
                }
              }

              /* check "scoreboard" for operand readiness (we're not actually
                 explicitly implementing a scoreboard); if value is ready, read
                 it into data-capture window or payload RAM. */
              tick_t when_ready = 0;
              for(int j=0;j<MAX_IDEPS;j++) /* for possible input argument */
              {
                if(uop->exec.idep_uop[j]) /* if the parent uop exists (i.e., still in the processor) */
                {
                  uop->timing.when_itag_ready[j] = uop->exec.idep_uop[j]->timing.when_otag_ready;
                  if(uop->exec.idep_uop[j]->exec.ovalue_valid)
                  {
                    uop->timing.when_ival_ready[j] = uop->exec.idep_uop[j]->timing.when_completed;
                    uop->exec.ivalue_valid[j] = true;
                    if(uop->decode.idep_name[j] == DCREG(MD_REG_AFLAGS))
                      uop->exec.ivalue[j].dw = uop->exec.idep_uop[j]->exec.oflags;
                    else
                      uop->exec.ivalue[j] = uop->exec.idep_uop[j]->exec.ovalue;
                  }
                }
                else /* read from ARF */
                {
                  uop->timing.when_itag_ready[j] = sim_cycle;
                  uop->timing.when_ival_ready[j] = sim_cycle;
                  uop->exec.ivalue_valid[j] = true; /* applies to invalid (DNA) inputs as well */
                  if(uop->decode.idep_name[j] != DNA)
                    uop->exec.ivalue[j] = uop->oracle.ivalue[j]; /* oracle value == architected value */
                }
                if(when_ready < uop->timing.when_itag_ready[j])
                  when_ready = uop->timing.when_itag_ready[j];
              }
              uop->timing.when_ready = when_ready;
              if(when_ready < TICK_T_MAX) /* add to readyQ if appropriate */
                core->exec->insert_ready_uop(uop);


            }
            else /* is_nop || is_trap */
            {
              /* NOP's don't go through exec pipeline; they go straight to the
                 ROB and are immediately marked as completed (they still take
                 up space in the ROB though). */
              /* Since traps/interrupts aren't really properly modeled in SimpleScalar, we just let
                 it go through without doing anything. */
              uop->timing.when_ready = sim_cycle;
              uop->timing.when_issued = sim_cycle;
              uop->timing.when_completed = sim_cycle;
            }

            uop->timing.when_allocated = sim_cycle;

          }

          if(uop->decode.in_fusion)
            uop = uop->decode.fusion_next;
          else
            uop = NULL;
        }

        if(abort_alloc)
          break;

        if((!pipe[stage][i]->decode.in_fusion) || !uop) /* either not fused, or complete fused uops alloc'd */
        {
          uop = pipe[stage][i]; /* may be NULL if we just finished a fused set */

          /* update stats */
          if(uop->decode.EOM)
            ZESTO_STAT(core->stat.alloc_insn++;)

          ZESTO_STAT(core->stat.alloc_uops++;)
          if(uop->decode.in_fusion)
            ZESTO_STAT(core->stat.alloc_eff_uops += uop->decode.fusion_size;)
          else
            ZESTO_STAT(core->stat.alloc_eff_uops++;)

          /* remove from alloc pipe */
          pipe[stage][i] = NULL;
          occupancy[stage]--;
          zesto_assert(occupancy[stage] >= 0,(void)0);
        }
      }
    }
  }
  else
    stall_reason = ASTALL_EMPTY;


  /*=============================================*/
  /*== Shuffle uops down the rename/alloc pipe ==*/

  /* TODO: PRF check and stall */
  
  /* walk pipe backwards */
  for(stage=knobs->alloc.depth-1; stage > 0; stage--)
  {
    if(0 == occupancy[stage]) /* implementing non-serpentine pipe (no compressing) - can't advance until stage is empty */
    {
      /* move everyone from previous stage forward */
      for(i=0;i<knobs->alloc.width;i++)
      {
        pipe[stage][i] = pipe[stage-1][i];
        pipe[stage-1][i] = NULL;
        if(pipe[stage][i])
        {
          occupancy[stage]++;
          occupancy[stage-1]--;
          zesto_assert(occupancy[stage] <= knobs->alloc.width,(void)0);
          zesto_assert(occupancy[stage-1] >= 0,(void)0);
        }
      }
    }
  }

  /*==============================================*/
  /*== fill first alloc stage from decode stage ==*/
  if(0 == occupancy[0])
  {
    /* while the uopQ sitll has uops in it, allocate up to alloc.width uops per cycle */
    for(i=0;(i<knobs->alloc.width) && core->decode->uop_available();i++)
    {
      pipe[0][i] = core->decode->uop_peek(); core->decode->uop_consume();
      occupancy[0]++;
      zesto_assert(occupancy[0] <= knobs->alloc.width,(void)0);
    }
  }

  ZESTO_STAT(stat_add_sample(core->stat.alloc_stall, (int)stall_reason);)
}

/* start from most recently fetched, blow away everything until
   we find the Mop */
void
core_alloc_DPM_t::recover(struct Mop_t * Mop)
{
  struct core_knobs_t * knobs = core->knobs;
  int stage,i;
  for(stage=0;stage<knobs->alloc.depth;stage++)
  {
    /* slot N-1 is most speculative, start from there */
    if(occupancy[stage])
      for(i=knobs->alloc.width-1;i>=0;i--)
      {
        if(pipe[stage][i])
        {
          if(pipe[stage][i]->Mop == Mop)
            return;
          pipe[stage][i] = NULL;
          occupancy[stage]--;
          zesto_assert(occupancy[stage] >= 0,(void)0);
        }
      }
  }
}

/* clean up on pipeline flush (blow everything away) */
void
core_alloc_DPM_t::recover(void)
{
  struct core_knobs_t * knobs = core->knobs;
  int stage,i;
  for(stage=0;stage<knobs->alloc.depth;stage++)
  {
    /* slot N-1 is most speculative, start from there */
    if(occupancy[stage])
      for(i=knobs->alloc.width-1;i>=0;i--)
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

void core_alloc_DPM_t::RS_deallocate(struct uop_t * uop)
{
  zesto_assert(port_loading[uop->alloc.port_assignment] > 0,(void)0);
  port_loading[uop->alloc.port_assignment]--;
}

void core_alloc_DPM_t::start_drain(void)
{
  drain_in_progress = true;
}


#endif
