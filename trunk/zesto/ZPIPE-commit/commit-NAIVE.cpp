/* commit-NAIVE.cpp - Simple(r) Timing Model */
/*
 * Copyright (C) 2007 by Gabriel H. Loh and the Georgia Tech
 * Research Corporation Atlanta, GA  30332-0415
 * All Rights Reserved.
 * 
 * THIS IS A LEGAL DOCUMENT BY DOWNLOADING ZESTO, YOU ARE AGREEING
 * TO THESE TERMS AND CONDITIONS.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * NOTE: Portions of this release are directly derived from the
 * SimpleScalar Toolset (property of SimpleScalar LLC), and as
 * such, those portions are bound by the corresponding legal terms
 * and conditions.  All source files derived directly or in part
 * from the SimpleScalar Toolset bear the original user agreement.
 * 
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 
 * 1. Redistributions of source code must retain the above
 * copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 * 
 * 3. Neither the name of the Georgia Tech Research Coporation nor
 * the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior
 * written permission.
 * 
 * 4. Zesto is distributed freely for commercial and non-commercial
 * use.  Note, however, that the portions derived from the
 * SimpleScalar Toolset are bound by the terms and agreements set
 * forth by SimpleScalar, LLC.  In particular:
 * 
 *   "Nonprofit and noncommercial use is encouraged. SimpleScalar
 *   may be downloaded, compiled, executed, copied, and modified
 *   solely for nonprofit, educational, noncommercial research, and
 *   noncommercial scholarship purposes provided that this notice
 *   in its entirety accompanies all copies.  Copies of the
 *   modified software can be delivered to persons who use it
 *   solely for nonprofit, educational, noncommercial research, and
 *   noncommercial scholarship purposes provided that this notice
 *   in its entirety accompanies all copies."
 * 
 * User is responsible for reading and adhering to the terms set
 * forth by SimpleScalar, LLC where appropriate.
 * 
 * 5. No nonprofit user may place any restrictions on the use of
 * this software, including as modified by the user, by any other
 * authorized user.
 * 
 * 6. Noncommercial and nonprofit users may distribute copies of
 * Zesto in compiled or executable form as set forth in Section 2,
 * provided that either: (A) it is accompanied by the corresponding
 * machine-readable source code, or (B) it is accompanied by a
 * written offer, with no time limit, to give anyone a
 * machine-readable copy of the corresponding source code in return
 * for reimbursement of the cost of distribution. This written
 * offer must permit verbatim duplication by anyone, or (C) it is
 * distributed by someone who received only the executable form,
 * and is accompanied by a copy of the written offer of source
 * code.
 * 
 * 7. Zesto was developed by Gabriel H. Loh, Ph.D.  US Mail: 266
 * Ferst Drive, Georgia Institute of Technology, Atlanta, GA
 * 30332-0765
 * 
 */

#ifdef ZESTO_PARSE_ARGS
  if(!strcasecmp(commit_opt_string,"NAIVE"))
    return new core_commit_NAIVE_t(core);
#else

class core_commit_NAIVE_t:public core_commit_t
{
  enum commit_stall_t {CSTALL_NONE,      /* no stall */
                       CSTALL_NOT_READY, /* oldest inst not done (no uops finished) */
                       CSTALL_PARTIAL,   /* oldest inst not done (but some uops finished) */
                       CSTALL_EMPTY,     /* ROB is empty, nothing to commit */
                       CSTALL_num
                     };

  public:

  core_commit_NAIVE_t(struct core_t * core);
  virtual void reg_stats(struct stat_sdb_t *sdb);
  virtual void update_occupancy(void);

  virtual void step(void);
  virtual void recover(struct Mop_t * Mop);
  virtual void recover(void);

  virtual bool ROB_available(void);
  virtual bool ROB_empty(void);
  virtual void ROB_insert(struct uop_t * uop);
  virtual void ROB_fuse_insert(struct uop_t * uop);

  protected:

  struct uop_t ** ROB;
  int ROB_head;
  int ROB_tail;
  int ROB_num;

  static const char *commit_stall_str[CSTALL_num];

  /* additional temps to track timing of REP insts */
  tick_t when_rep_fetch_started;
  tick_t when_rep_fetched;
  tick_t when_rep_decode_started;
  tick_t when_rep_commit_started;
};

/* number of buckets in uop-flow-length histogram */
#define FLOW_HISTO_SIZE 9

/* VARIABLES/TYPES */

const char *core_commit_NAIVE_t::commit_stall_str[CSTALL_num] = {
  "no stall                   ",
  "oldest inst not done       ",
  "oldest inst partially done ",
  "ROB is empty               ",
};

/*******************/
/* SETUP FUNCTIONS */
/*******************/

core_commit_NAIVE_t::core_commit_NAIVE_t(struct core_t * arg_core):
  ROB_head(0), ROB_tail(0), ROB_num(0),
  when_rep_fetch_started(0), when_rep_fetched(0),
  when_rep_decode_started(0), when_rep_commit_started(0)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;
  ROB = (struct uop_t**) calloc(knobs->commit.ROB_size,sizeof(*ROB));
  if(!ROB)
    fatal("couldn't calloc ROB");
}

void
core_commit_NAIVE_t::reg_stats(struct stat_sdb_t *sdb)
{
  char buf[1024];
  char buf2[1024];
  struct thread_t * arch = core->current_thread;

  stat_reg_note(sdb,"\n#### COMMIT STATS ####");

  sprintf(buf,"c%d.commit_insn",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions committed", &core->stat.commit_insn, core->stat.commit_insn, NULL);
  sprintf(buf,"c%d.commit_uops",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of uops committed", &core->stat.commit_uops, core->stat.commit_uops, NULL);
  sprintf(buf,"c%d.commit_IPC",arch->id);
  sprintf(buf2,"c%d.commit_insn/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "IPC at commit", buf2, NULL);
  sprintf(buf,"c%d.commit_uPC",arch->id);
  sprintf(buf2,"c%d.commit_uops/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "uPC at commit", buf2, NULL);
  sprintf(buf,"c%d.avg_commit_flowlen",arch->id);
  sprintf(buf2,"c%d.commit_uops/c%d.commit_insn",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "uops per instruction at commit", buf2, NULL);

  sprintf(buf,"c%d.commit_dead_lock_flushes",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of pipe-flushes due to dead-locked pipeline", &core->stat.commit_deadlock_flushes, core->stat.commit_deadlock_flushes, NULL);
  sprintf(buf,"c%d.ROB_occupancy",arch->id);
  stat_reg_counter(sdb, false, buf, "total ROB occupancy", &core->stat.ROB_occupancy, core->stat.ROB_occupancy, NULL);
  sprintf(buf,"c%d.ROB_empty",arch->id);
  stat_reg_counter(sdb, false, buf, "total cycles ROB was empty", &core->stat.ROB_empty_cycles, core->stat.ROB_empty_cycles, NULL);
  sprintf(buf,"c%d.ROB_full",arch->id);
  stat_reg_counter(sdb, false, buf, "total cycles ROB was full", &core->stat.ROB_full_cycles, core->stat.ROB_full_cycles, NULL);
  sprintf(buf,"c%d.ROB_avg",arch->id);
  sprintf(buf2,"c%d.ROB_occupancy/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "average ROB occupancy", buf2, NULL);
  sprintf(buf,"c%d.ROB_frac_empty",arch->id);
  sprintf(buf2,"c%d.ROB_empty/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles ROB was empty", buf2, NULL);
  sprintf(buf,"c%d.ROB_frac_full",arch->id);
  sprintf(buf2,"c%d.ROB_full/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "fraction of cycles ROB was full", buf2, NULL);

  sprintf(buf,"c%d.commit_stall",core->current_thread->id);
  core->stat.commit_stall = stat_reg_dist(sdb, buf,
                                           "breakdown of stalls at commit",
                                           /* initial value */0,
                                           /* array size */CSTALL_num,
                                           /* bucket size */1,
                                           /* print format */(PF_COUNT|PF_PDF),
                                           /* format */NULL,
                                           /* index map */commit_stall_str,
                                           /* print fn */NULL);

  stat_reg_note(sdb,"#### TIMING STATS ####");
  sprintf(buf,"c%d.sim_cycle",arch->id);
  stat_reg_qword(sdb, true, buf, "total number of cycles when last instruction (or uop) committed", (qword_t*) &core->stat.final_sim_cycle, core->stat.final_sim_cycle, NULL);
  /* instruction distribution stats */
  stat_reg_note(sdb,"\n#### INSTRUCTION STATS (no wrong-path) ####");
  sprintf(buf,"c%d.num_insn",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions committed", &core->stat.commit_insn, core->stat.commit_insn, NULL);
  sprintf(buf,"c%d.num_refs",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of loads and stores committed", &core->stat.commit_refs, core->stat.commit_refs, NULL);
  sprintf(buf,"c%d.num_loads",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of loads committed", &core->stat.commit_loads, core->stat.commit_loads, NULL);
  sprintf(buf2,"c%d.num_refs - c%d.num_loads",arch->id,arch->id);
  sprintf(buf,"c%d.num_stores",arch->id);
  stat_reg_formula(sdb, true, buf, "total number of stores committed", buf2, "%12.0f");
}

void core_commit_NAIVE_t::update_occupancy(void)
{
}


/*************************/
/* MAIN COMMIT FUNCTIONS */
/*************************/

/* In-order instruction commit.  Individual uops cannot commit
   until it is guaranteed that the entire Mop's worth of uops will
   commit. */
void core_commit_NAIVE_t::step(void)
{
}

/* Walk ROB from youngest uop until we find the requested Mop.
   (NOTE: We stop at any uop belonging to the Mop.  We assume
   that recovery only occurs on Mop boundaries.)
   Release resources (PREGs, RS/ROB/LSQ entries, etc. as we go).
   If Mop == NULL, we're blowing away the entire pipeline. */
void
core_commit_NAIVE_t::recover(struct Mop_t * Mop)
{
}

void
core_commit_NAIVE_t::recover(void)
{
}

bool core_commit_NAIVE_t::ROB_available(void)
{
}

bool core_commit_NAIVE_t::ROB_empty(void)
{
}

void core_commit_NAIVE_t::ROB_insert(struct uop_t * uop)
{
}

void core_commit_NAIVE_t::ROB_fuse_insert(struct uop_t * uop)
{
}

#endif
