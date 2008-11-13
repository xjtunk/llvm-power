/* zesto-opts.c */
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

#include "thread.h"
#include "loader.h"

#include "zesto-opts.h"
#include "zesto-core.h"
#include "zesto-oracle.h"
#include "zesto-fetch.h"
#include "zesto-decode.h"
#include "zesto-alloc.h"
#include "zesto-exec.h"
#include "zesto-cache.h"
#include "zesto-commit.h"
#include "zesto-dram.h"
#include "zesto-uncore.h"


/* variables for fast-forwarding prior to detailed simulation */
long long fastfwd = 0;
long long trace_limit = 0; /* maximum number of instructions per trace - used for looping */

/* maximum number of inst's/uop's to execute */
long long max_insts = 0;
long long max_uops = 0;

static bool ignored_flag = 0;
extern struct core_knobs_t knobs;

counter_t total_commit_insn = 0;
counter_t total_commit_uops = 0;
counter_t total_commit_eff_uops = 0;

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
      "sim-zesto: This simulator implements an execute-at-fetch timing\n"
      "simulator for x86 only.  Exec-at-exec is planned for the future.\n"
      );

  /* ignored flag used to terminate list options */
  opt_reg_flag(odb, "-","ignored flag",
      &ignored_flag, /*default*/ false, /*print*/false,/*format*/NULL);

  opt_reg_long_long(odb, "-fastfwd", "number of inst's to skip before timing simulation",
      &fastfwd, /* default */0, /* print */true, /* format */NULL);

  opt_reg_long_long(odb, "-tracelimit", "maximum number of instructions per trace",
      &trace_limit, /* default */0, /* print */true, /* format */NULL);

  opt_reg_int(odb, "-cores", "number of cores (if > 1, must provide this many eio traces)",
      &num_threads, /* default */1, /* print */true, /* format */NULL);

  /* instruction limit */
  opt_reg_long_long(odb, "-max:inst", "maximum number of inst's to execute",
      &max_insts, /* default */0,
      /* print */true, /* format */NULL);
  opt_reg_long_long(odb, "-max:uops", "maximum number of uop's to execute",
      &max_uops, /* default */0,
      /* print */true, /* format */NULL);

  opt_reg_string(odb, "-model","pipeline model type",
      &knobs.model, /*default*/ "DPM", /*print*/true,/*format*/NULL);

  fetch_reg_options(odb,&knobs);
  decode_reg_options(odb,&knobs);
  alloc_reg_options(odb,&knobs);
  exec_reg_options(odb,&knobs);
  commit_reg_options(odb,&knobs);

  uncore_reg_options(odb);
  dram_reg_options(odb);
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  if(max_uops && max_uops < max_insts)
    warn("-max:uops is less than -max:inst, so -max:inst is useless");

  if((num_threads < 1) || (num_threads > MAX_CORES))
    fatal("-cores must be between 1 and %d (inclusive)",MAX_CORES);

  simulated_processes_remaining = num_threads;

  uncore_create();
  dram_create();
}

/* register per-arch statistics */
void
cpu_reg_stats(struct core_t * core, struct stat_sdb_t *sdb)
{
  struct thread_t * arch = core->current_thread;

  core->oracle->reg_stats(sdb);
  core->fetch->reg_stats(sdb);
  core->decode->reg_stats(sdb);
  core->alloc->reg_stats(sdb);
  core->exec->reg_stats(sdb);
  core->commit->reg_stats(sdb);
  ld_reg_stats(core->current_thread,sdb);

  /* only print this out once at the very end */
  if(core->current_thread->id == (num_threads-1))
  {
    uncore_reg_stats(sdb);
    mem_reg_stats(arch->mem, sdb);
  }
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct thread_t ** archs, struct stat_sdb_t *sdb)
{
  int i;
  char buf[1024];
  char buf2[1024];

  /* per core stats */
  for(i=0;i<num_threads;i++)
    cpu_reg_stats(cores[i],sdb);

  stat_reg_note(sdb,"\n#### SIMULATOR PERFORMANCE STATS ####");
  stat_reg_qword(sdb, true, "sim_cycle", "total simulation cycles", (qword_t*)&sim_cycle, 0, NULL);
  stat_reg_int(sdb, true, "sim_elapsed_time", "total simulation time in seconds", &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, true, "sim_cycle_rate", "simulation speed (in Mcycles/sec)", "sim_cycle / (sim_elapsed_time * 1000000.0)", NULL);
  /* Make formula to add num_insn from all archs */
  strcpy(buf2,"");
  for(i=0;i<num_threads;i++)
  {
    if(i==0)
      sprintf(buf,"c%d.commit_insn",i);
    else
      sprintf(buf," + c%d.commit_insn",i);

    strcat(buf2,buf);
  }
  stat_reg_formula(sdb, true, "all_insn", "total insts simulated for all cores", buf2, "%12.0f");
  stat_reg_formula(sdb, true, "sim_inst_rate", "simulation speed (in MIPS)", "all_insn / (sim_elapsed_time * 1000000.0)", NULL);

  /* Make formula to add num_uops from all archs */
  strcpy(buf2,"");
  for(i=0;i<num_threads;i++)
  {
    if(i==0)
      sprintf(buf,"c%d.commit_uops",i);
    else
      sprintf(buf," + c%d.commit_uops",i);

    strcat(buf2,buf);
  }
  stat_reg_formula(sdb, true, "all_uops", "total uops simulated for all cores", buf2, "%12.0f");
  stat_reg_formula(sdb, true, "sim_uop_rate", "simulation speed (in MuPS)", "all_uops / (sim_elapsed_time * 1000000.0)", NULL);

  /* Make formula to add num_eff_uops from all archs */
  strcpy(buf2,"");
  for(i=0;i<num_threads;i++)
  {
    if(i==0)
      sprintf(buf,"c%d.commit_eff_uops",i);
    else
      sprintf(buf," + c%d.commit_eff_uops",i);

    strcat(buf2,buf);
  }
  stat_reg_formula(sdb, true, "all_eff_uops", "total effective uops simulated for all cores", buf2, "%12.0f");
  stat_reg_formula(sdb, true, "sim_eff_uop_rate", "simulation speed (in MeuPS)", "all_eff_uops / (sim_elapsed_time * 1000000.0)", NULL);

  /* Make formula to add commit_IPC from all cores */
  strcpy(buf2,"");
  for(i=0;i<num_threads;i++)
  {
    if(i==0)
      sprintf(buf,"c%d.commit_IPC",i);
    else
      sprintf(buf," + c%d.commit_IPC",i);

    strcat(buf2,buf);
  }
  stat_reg_formula(sdb, true, "all_IPC", "total IPC for all cores", buf2, NULL);

  strcpy(buf2,"");
  for(i=0;i<num_threads;i++)
  {
    if(i==0)
      sprintf(buf,"c%d.commit_uPC",i);
    else
      sprintf(buf," + c%d.commit_uPC",i);

    strcat(buf2,buf);
  }
  stat_reg_formula(sdb, true, "all_uPC", "total uPC for all cores", buf2, NULL);

  strcpy(buf2,"");
  for(i=0;i<num_threads;i++)
  {
    if(i==0)
      sprintf(buf,"c%d.commit_euPC",i);
    else
      sprintf(buf," + c%d.commit_euPC",i);

    strcat(buf2,buf);
  }
  stat_reg_formula(sdb, true, "all_euPC", "total euPC for all cores", buf2, NULL);

  if(num_threads > 1)
  {
    stat_reg_counter(sdb, true, "total_insn", "total instructions simulated for all cores, including instructions from inactive (looping) cores", &total_commit_insn, total_commit_insn, NULL);
    stat_reg_counter(sdb, true, "total_uops", "total uops simulated for all cores, including uops from inactive (looping) cores", &total_commit_uops, total_commit_uops, NULL);
    stat_reg_counter(sdb, true, "total_eff_uops", "total effective uops simulated for all cores, including effective uops from inactive (looping) cores", &total_commit_eff_uops, total_commit_eff_uops, NULL);
    stat_reg_formula(sdb, true, "total_inst_rate", "simulation speed (in MIPS)", "total_insn / (sim_elapsed_time * 1000000.0)", NULL);
    stat_reg_formula(sdb, true, "total_uop_rate", "simulation speed (in MuPS)", "total_uops / (sim_elapsed_time * 1000000.0)", NULL);
    stat_reg_formula(sdb, true, "total_eff_uop_rate", "simulation speed (in MeuPS)", "total_eff_uops / (sim_elapsed_time * 1000000.0)", NULL);
    stat_reg_formula(sdb, true, "loop_inst_overhead", "overhead rate for additional looping instructions", "total_insn / all_insn", NULL);
    stat_reg_formula(sdb, true, "loop_uop_overhead", "overhead rate for additional looping uops", "total_uops / all_uops", NULL);
    stat_reg_formula(sdb, true, "loop_eff_uop_overhead", "overhead rate for additional looping effective uops", "total_eff_uops / all_eff_uops", NULL);
  }

}


