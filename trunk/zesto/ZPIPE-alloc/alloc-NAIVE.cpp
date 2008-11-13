/* alloc-NAIVE.cpp - Simple(r) Timing Model */
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
  if(!strcasecmp(alloc_opt_string,"NAIVE"))
    return new core_alloc_NAIVE_t(core);
#else

class core_alloc_NAIVE_t:public core_alloc_t
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

  core_alloc_NAIVE_t(struct core_t * core);
  virtual void reg_stats(struct stat_sdb_t * sdb);

  virtual void step(void);
  virtual void recover(void);
  virtual void recover(struct Mop_t * Mop);

  virtual void RS_deallocate(struct uop_t * uop);
  virtual void start_drain(void); /* prevent allocation from proceeding to exec */

  protected:

  /* for load-balancing port binding */
  int * port_loading;

  static const char *alloc_stall_str[ASTALL_num];
};

const char *core_alloc_NAIVE_t::alloc_stall_str[ASTALL_num] = {
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

core_alloc_NAIVE_t::core_alloc_NAIVE_t(struct core_t * arg_core)
{
  struct core_knobs_t * knobs = arg_core->knobs;
  core = arg_core;

  port_loading = (int*) calloc(knobs->exec.num_exec_ports,sizeof(*port_loading));
  if(!port_loading)
    fatal("couldn't calloc allocation port-loading scoreboard");
}

void
core_alloc_NAIVE_t::reg_stats(struct stat_sdb_t *sdb)
{
  char buf[1024];
  char buf2[1024];
  struct thread_t * arch = core->current_thread;

  stat_reg_note(sdb,"#### ALLOC STATS ####");
  sprintf(buf,"c%d.alloc_insn",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of instructions alloced", &core->stat.alloc_insn, core->stat.alloc_insn, NULL);
  sprintf(buf,"c%d.alloc_uops",arch->id);
  stat_reg_counter(sdb, true, buf, "total number of uops alloced", &core->stat.alloc_uops, core->stat.alloc_uops, NULL);
  sprintf(buf,"c%d.alloc_IPC",arch->id);
  sprintf(buf2,"c%d.alloc_insn/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "IPC at alloc", buf2, NULL);
  sprintf(buf,"c%d.alloc_uPC",arch->id);
  sprintf(buf2,"c%d.alloc_uops/c%d.sim_cycle",arch->id,arch->id);
  stat_reg_formula(sdb, true, buf, "uPC at alloc", buf2, NULL);

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

void core_alloc_NAIVE_t::step(void)
{
}

/* clean up on pipeline flush (blow everything away) */
void
core_alloc_NAIVE_t::recover(void)
{
  /* nothing to do */
}
void
core_alloc_NAIVE_t::recover(struct Mop_t * Mop)
{
  /* nothing to do */
}

void core_alloc_NAIVE_t::RS_deallocate(struct uop_t * uop)
{
  zesto_assert(port_loading[uop->alloc.port_assignment] > 0,(void)0);
  port_loading[uop->alloc.port_assignment]--;
}

void core_alloc_NAIVE_t::start_drain(void)
{
  drain_in_progress = true;
}


#endif
