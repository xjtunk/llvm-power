/* zesto-cache.cpp */
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

#include <ctype.h>
#include "thread.h"
#include "stats.h"
#include "zesto-core.h"
#include "zesto-opts.h"
#include "zesto-cache.h"
#include "zesto-prefetch.h"
#include "zesto-dram.h"
#include "zesto-uncore.h"

/* This is used for latency prediction when a load completely misses in cache
   and has to go off to main memory */
#define BIG_LATENCY 99999

#define CACHE_STAT(x) {if(!cp->frozen) {x}}

#define GET_BANK(x) (((x)>>cp->bank_shift) & cp->bank_mask)
#define GET_MSHR_BANK(x) (((x)>>cp->bank_shift) & cp->MSHR_mask)

struct cache_t * cache_create(
    struct core_t * core,
    char * name,
    int read_only,
    int sets,
    int assoc,
    int linesize,
    char rp, /* replacement policy */
    char ap, /* allocation policy */
    char wp, /* write policy */
    char wc, /* write combining */
    int banks,
    int bank_width, /* in bytes; i.e., the interleaving granularity */
    int latency, /* in cycles */
    int WBB_size, /* writeback-buffer size, in cache lines (for evictions) */
    int MSHR_size, /* MSHR size (per bank), in requests */
    int MSHR_banks, /* number of MSHR banks */
    struct cache_t * next_level_cache, /* e.g., for the DL1, this should point to the L2 */
    struct bus_t * next_bus) /* e.g., for the DL1, this should point to the bus between DL1 and L2 */
{

  int i;
  struct cache_t * cp;
  cp = (struct cache_t*) calloc(1,sizeof(*cp));
  if(!cp)
    fatal("failed to calloc cache %s",name);

  enum repl_policy_t replacement_policy;
  enum alloc_policy_t allocate_policy;
  enum write_policy_t write_policy;
  int write_combining;

  cp->core = core;

  switch(toupper(rp)) {
    case 'L': replacement_policy = REPLACE_LRU; break;
    case 'M': replacement_policy = REPLACE_MRU; break;
    case 'R': replacement_policy = REPLACE_RANDOM; break;
    case 'N': replacement_policy = REPLACE_NMRU; break;
    case 'P': replacement_policy = REPLACE_PLRU; break;
    case 'C': replacement_policy = REPLACE_CLOCK; break;
    default: fatal("unrecognized cache replacement policy");
  }
  switch(toupper(ap)) {
    case 'W': allocate_policy = WRITE_ALLOC; break;
    case 'N': allocate_policy = NO_WRITE_ALLOC; break;
    default: fatal("unrecognized cache allocation policy (W=write-alloc,N=no-write-alloc)");
  }
  switch(toupper(wp)) {
    case 'T': write_policy = WRITE_THROUGH; break;
    case 'B': write_policy = WRITE_BACK; break;
    default: fatal("unrecognized cache write policy (T=write-Through,B=write-Back)");
  }
  switch(toupper(wc)) {
    case 'C': write_combining = true; break;
    case 'N': write_combining = false; break;
    default: fatal("unrecognized cache write combining option (C=write-Combining,N=No-write-combining)");
  }

  cp->name = strdup(name);
  if(!cp->name)
    fatal("failed to strdup %s's name",name);
  cp->read_only = (enum read_only_t) read_only;
  cp->sets = sets;
  cp->assoc = assoc;
  cp->log2_assoc = (int)(ceil(log(assoc)/log(2.0)));
  cp->linesize = linesize;
  cp->replacement_policy = replacement_policy;
  cp->allocate_policy = allocate_policy;
  cp->write_policy = write_policy;
  cp->banks = banks;
  cp->bank_width = bank_width;
  cp->bank_mask = banks-1;
  cp->latency = latency;
  cp->write_combining = write_combining;
  cp->WBB_size = WBB_size;
  cp->MSHR_size = MSHR_size;
  cp->MSHR_banks = MSHR_banks;
  cp->MSHR_mask = MSHR_banks-1;
  cp->next_level = next_level_cache;
  cp->next_bus = next_bus;

  if((cp->replacement_policy == REPLACE_PLRU) && (assoc & (assoc-1)))
    fatal("Tree-based PLRU only works when associativity is a power of two");
  if((cp->replacement_policy == REPLACE_CLOCK) && (assoc > 64))
    fatal("Clock-based PLRU only works when associativity is <= 64");

  /* bits to shift to get to the tag */
  cp->addr_shift = (int)ceil(log(linesize)/log(2.0));
  /* bits to shfit to get the bank number */
  cp->bank_shift = (int)ceil(log(bank_width)/log(2.0));

  if((sets & (sets-1)) != 0)
    fatal("%s cache sets must be power of two");
  if((linesize & (linesize-1)) != 0)
    fatal("%s linesize must be power of two");
  if((banks & (banks-1)) != 0)
    fatal("%s banks must be power of two");

  cp->blocks = (struct cache_line_t**) calloc(sets,sizeof(*cp->blocks));
  for(i=0;i<sets;i++)
  {
    int j;
    /* allocate as one big block! */
    struct cache_line_t * line = (struct cache_line_t*) calloc(assoc,sizeof(*line));
    memset(line,0,assoc*sizeof(*line)); // for some weird reason valgrind was reporting that "line" was being used uninitialized, despite being calloc'd above?
    if(!line)
      fatal("failed to calloc cache line for %s",name);

    for(j=0;j<assoc;j++)
    {
      line[j].way = j;
      line[j].next = &line[j+1];
    }
    line[assoc-1].next = NULL;
    cp->blocks[i] = line;
  }

  cp->heap_size = 1 << ((int) rint(ceil(log(latency+1)/log(2.0))));

  cp->pipe = (struct cache_action_t**) calloc(banks,sizeof(*cp->pipe));
  cp->pipe_num = (int*) calloc(banks,sizeof(*cp->pipe_num));
  cp->fill_pipe = (struct cache_fill_t**) calloc(banks,sizeof(*cp->fill_pipe));
  cp->fill_num = (int*) calloc(banks,sizeof(*cp->fill_num));
  if(!cp->pipe || !cp->fill_pipe || !cp->pipe_num || !cp->fill_num)
    fatal("failed to calloc cp->pipe for %s",name);
  for(i=0;i<banks;i++)
  {
    cp->pipe[i] = (struct cache_action_t*) calloc(cp->heap_size,sizeof(**cp->pipe));
    if(!cp->pipe[i])
      fatal("failed to calloc cp->pipe[%d] for %s",i,name);
    cp->fill_pipe[i] = (struct cache_fill_t*) calloc(cp->heap_size,sizeof(**cp->fill_pipe));
    if(!cp->fill_pipe[i])
      fatal("failed to calloc cp->fill_pipe[%d] for %s",i,name);
  }

  cp->MSHR = (struct cache_action_t**) calloc(MSHR_banks,sizeof(*cp->MSHR));
  if(!cp->MSHR)
    fatal("failed to calloc cp->MSHR for %s",name);
  for(i=0;i<MSHR_banks;i++)
  {
    cp->MSHR[i] = (struct cache_action_t*) calloc(MSHR_size,sizeof(**cp->MSHR));
    if(!cp->MSHR[i])
      fatal("failed to calloc cp->MSHR[%d] for %s",i,name);
  }

  cp->MSHR_num = (int*) calloc(MSHR_banks,sizeof(*cp->MSHR_num));
  if(!cp->MSHR_num)
    fatal("failed to calloc cp->MSHR_num");
  cp->MSHR_num_pf = (int*) calloc(MSHR_banks,sizeof(*cp->MSHR_num_pf));
  if(!cp->MSHR_num_pf)
    fatal("failed to calloc cp->MSHR_num_pf");
  cp->MSHR_fill_num = (int*) calloc(MSHR_banks,sizeof(*cp->MSHR_fill_num));
  if(!cp->MSHR_fill_num)
    fatal("failed to calloc cp->MSHR_fill_num");
  cp->MSHR_unprocessed_num = (int*) calloc(MSHR_banks,sizeof(*cp->MSHR_unprocessed_num));
  if(!cp->MSHR_unprocessed_num)
    fatal("failed to calloc cp->MSHR_unprocessed_num");

  cp->WBB_num = (int*) calloc(MSHR_banks,sizeof(*cp->WBB_num));
  if(!cp->WBB_num)
    fatal("failed to calloc cp->WBB_num");
  cp->WBB_head = (int*) calloc(MSHR_banks,sizeof(*cp->WBB_head));
  if(!cp->WBB_head)
    fatal("failed to calloc cp->WBB_head");
  cp->WBB_tail = (int*) calloc(MSHR_banks,sizeof(*cp->WBB_tail));
  if(!cp->WBB_tail)
    fatal("failed to calloc cp->WBB_tail");

  cp->WBB = (struct cache_line_t**) calloc(MSHR_banks,sizeof(*cp->WBB));
  if(!cp->WBB)
    fatal("failed to calloc cp->WBB");
  for(i=0;i<MSHR_banks;i++)
  {
    cp->WBB[i] = (struct cache_line_t*) calloc(WBB_size,sizeof(**cp->WBB));
    if(!cp->WBB[i])
      fatal("failed to calloc cp->WBB[%d] for %s",i,name);
  }

  if(!strcasecmp(name,"LLC"))
  {
    cp->stat.core_lookups = (counter_t*) calloc(num_threads,sizeof(*cp->stat.core_lookups));
    cp->stat.core_misses = (counter_t*) calloc(num_threads,sizeof(*cp->stat.core_misses));
    if(!cp->stat.core_lookups || !cp->stat.core_misses)
      fatal("failed to calloc cp->stat.core_{lookups|misses} for %s",name);
  }

  return cp;
}

void cache_reg_stats(struct stat_sdb_t * sdb, struct core_t * core, struct cache_t * cp)
{
  char buf[256];
  char buf2[256];
  char buf3[256];
  char core_str[256];
  int i;

  if(!core)
    fatal("must provide a core for cache_reg_stats; for LLC, use LLC_reg_stats");

  int id = core->current_thread->id;
  sprintf(core_str,"c%d.",id);

  /* TODO: re-factor this */
  if(cp->read_only == CACHE_READWRITE)
  {
    sprintf(buf,"%s%s.load_lookups",core_str,cp->name);
    sprintf(buf2,"number of load lookups in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.load_lookups, 0, NULL);
    sprintf(buf,"%s%s.load_misses",core_str,cp->name);
    sprintf(buf2,"number of load misses in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.load_misses, 0, NULL);
    sprintf(buf,"%s%s.load_miss_rate",core_str,cp->name);
    sprintf(buf2,"load miss rate in %s",cp->name);
    sprintf(buf3,"%s%s.load_misses/%s%s.load_lookups",core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"%s%s.store_lookups",core_str,cp->name);
    sprintf(buf2,"number of store lookups in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.store_lookups, 0, NULL);
    sprintf(buf,"%s%s.store_misses",core_str,cp->name);
    sprintf(buf2,"number of store misses in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.store_misses, 0, NULL);
    sprintf(buf,"%s%s.store_miss_rate",core_str,cp->name);
    sprintf(buf2,"store miss rate in %s",cp->name);
    sprintf(buf3,"%s%s.store_misses/%s%s.store_lookups",core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"%s%s.pf_lookups",core_str,cp->name);
    sprintf(buf2,"number of prefetch lookups in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_lookups, 0, NULL);
    sprintf(buf,"%s%s.pf_misses",core_str,cp->name);
    sprintf(buf2,"number of prefetch misses in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_misses, 0, NULL);
    sprintf(buf,"%s%s.pf_miss_rate",core_str,cp->name);
    sprintf(buf2,"prefetch miss rate in %s",cp->name);
    sprintf(buf3,"%s%s.pf_misses/%s%s.pf_lookups",core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"%s%s.pf_insertions",core_str,cp->name);
    sprintf(buf2,"number of prefetched blocks inserted into %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_insertions, 0, NULL);
    sprintf(buf,"%s%s.pf_useful_insertions",core_str,cp->name);
    sprintf(buf2,"number of prefetched blocks actually used in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_useful_insertions, 0, NULL);
    sprintf(buf,"%s%s.pf_useful_rate",core_str,cp->name);
    sprintf(buf2,"rate of useful prefetches in %s",cp->name);
    sprintf(buf3,"%s%s.pf_useful_insertions/%s%s.pf_insertions",core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"%s%s.total_lookups",core_str,cp->name);
    sprintf(buf2,"total number of lookups in %s",cp->name);
    sprintf(buf3,"%s%s.load_lookups + %s%s.store_lookups + %s%s.pf_lookups",core_str,cp->name,core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.0f");
    sprintf(buf,"%s%s.total_misses",core_str,cp->name);
    sprintf(buf2,"total number of misses in %s",cp->name);
    sprintf(buf3,"%s%s.load_misses + %s%s.store_misses + %s%s.pf_misses",core_str,cp->name,core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.0f");
    sprintf(buf,"%s%s.total_miss_rate",core_str,cp->name);
    sprintf(buf2,"total miss rate in %s",cp->name);
    sprintf(buf3,"%s%s.total_misses / %s%s.total_lookups",core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"%s%s.MPKI",core_str,cp->name);
    sprintf(buf2,"total miss rate in MPKI (no prefetches) for %s",cp->name);
    sprintf(buf3,"((%s%s.load_misses + %s%s.store_misses) / c%d.commit_insn) * 1000.0",core_str,cp->name,core_str,cp->name,id);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"%s%s.MPKu",core_str,cp->name);
    sprintf(buf2,"total miss rate in MPKu (no prefetches) for %s",cp->name);
    sprintf(buf3,"((%s%s.load_misses + %s%s.store_misses) / c%d.commit_uops) * 1000.0",core_str,cp->name,core_str,cp->name,id);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"%s%s.MPKeu",core_str,cp->name);
    sprintf(buf2,"total miss rate in MPKeu (no prefetches) for %s",cp->name);
    sprintf(buf3,"((%s%s.load_misses + %s%s.store_misses) / c%d.commit_eff_uops) * 1000.0",core_str,cp->name,core_str,cp->name,id);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"%s%s.MPKC",core_str,cp->name);
    sprintf(buf2,"total miss rate in MPKC (no prefetches) for %s",cp->name);
    sprintf(buf3,"((%s%s.load_misses + %s%s.store_misses) / c%d.sim_cycle) * 1000.0",core_str,cp->name,core_str,cp->name,id);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"%s%s.fill_pipe_hits",core_str,cp->name);
    sprintf(buf2,"total number load hits in %s's fill buffers",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.fill_pipe_hits, 0, NULL);
    sprintf(buf,"%s%s.WBB_accesses",core_str,cp->name);
    sprintf(buf2,"total number of writebacks in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.WBB_accesses, 0, NULL);
    sprintf(buf,"%s%s.WBB_combined",core_str,cp->name);
    sprintf(buf2,"total writebacks eliminated due to write-combining in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.WBB_combines, 0, NULL);
    sprintf(buf,"%s%s.WBB_frac_combined",core_str,cp->name);
    sprintf(buf2,"fraction of writebacks eliminated due to write combining in %s",cp->name);
    sprintf(buf3,"%s%s.WBB_combined / %s%s.WBB_accesses",core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"%s%s.WBB_hits",core_str,cp->name);
    sprintf(buf2,"total number load hits in %s's WBB",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.WBB_hits, 0, NULL);
    sprintf(buf,"%s%s.WBB_total_occupancy",core_str,cp->name);
    sprintf(buf2,"cumulative WBB occupancy in %s",cp->name);
    stat_reg_counter(sdb, false, buf, buf2, &cp->stat.WBB_occupancy, 0, NULL);
    sprintf(buf,"%s%s.WBB_avg_occupancy",core_str,cp->name);
    sprintf(buf2,"average WBB entries in use in %s",cp->name);
    sprintf(buf3,"(%s%s.WBB_total_occupancy / %ssim_cycle)",core_str,cp->name,core_str);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"%s%s.WBB_full_cycles",core_str,cp->name);
    sprintf(buf2,"cycles WBB was full in %s",cp->name);
    stat_reg_counter(sdb, false, buf, buf2, &cp->stat.WBB_full_cycles, 0, NULL);
    sprintf(buf,"%s%s.WBB_full",core_str,cp->name);
    sprintf(buf2,"fraction of time WBBs are full in %s",cp->name);
    sprintf(buf3,"(%s%s.WBB_full_cycles / c%d.sim_cycle)",core_str,cp->name,id);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
  }
  else
  {
    sprintf(buf,"%s%s.lookups",core_str,cp->name);
    sprintf(buf2,"number of lookups in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.load_lookups, 0, NULL);
    sprintf(buf,"%s%s.misses",core_str,cp->name);
    sprintf(buf2,"number of misses in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.load_misses, 0, NULL);
    sprintf(buf,"%s%s.pf_lookups",core_str,cp->name);
    sprintf(buf2,"number of prefetch lookups in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_lookups, 0, NULL);
    sprintf(buf,"%s%s.pf_misses",core_str,cp->name);
    sprintf(buf2,"number of prefetch misses in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_misses, 0, NULL);
    sprintf(buf,"%s%s.pf_miss_rate",core_str,cp->name);
    sprintf(buf2,"prefetch miss rate in %s",cp->name);
    sprintf(buf3,"%s%s.pf_misses/%s%s.pf_lookups",core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"%s%s.pf_insertions",core_str,cp->name);
    sprintf(buf2,"number of prefetched blocks inserted into %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_insertions, 0, NULL);
    sprintf(buf,"%s%s.pf_useful_insertions",core_str,cp->name);
    sprintf(buf2,"number of prefetched blocks actually used in %s",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_useful_insertions, 0, NULL);
    sprintf(buf,"%s%s.pf_useful_rate",core_str,cp->name);
    sprintf(buf2,"rate of useful prefetches in %s",cp->name);
    sprintf(buf3,"%s%s.pf_useful_insertions/%s%s.pf_insertions",core_str,cp->name,core_str,cp->name);

    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"%s%s.miss_rate",core_str,cp->name);
    sprintf(buf2,"miss rate in %s (no prefetches)",cp->name);
    sprintf(buf3,"%s%s.misses/%s%s.lookups",core_str,cp->name,core_str,cp->name);
    sprintf(buf,"%s%s.total_miss_rate",core_str,cp->name);
    sprintf(buf2,"miss rate in %s",cp->name);
    sprintf(buf3,"(%s%s.misses+%s%s.pf_misses)/(%s%s.lookups+%s%s.pf_lookups)",core_str,cp->name,core_str,cp->name,core_str,cp->name,core_str,cp->name);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"%s%s.MPKI",core_str,cp->name);
    sprintf(buf2,"miss rate in MPKI (no prefetches) for %s",cp->name);
    sprintf(buf3,"(%s%s.misses / c%d.commit_insn) * 1000.0",core_str,cp->name,id);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"%s%s.MPKu",core_str,cp->name);
    sprintf(buf2,"miss rate in MPKu (no prefetches) for %s",cp->name);
    sprintf(buf3,"(%s%s.misses / c%d.commit_uops) * 1000.0",core_str,cp->name,id);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"%s%s.MPKeu",core_str,cp->name);
    sprintf(buf2,"miss rate in MPKeu (no prefetches) for %s",cp->name);
    sprintf(buf3,"(%s%s.misses / c%d.commit_eff_uops) * 1000.0",core_str,cp->name,id);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"%s%s.fill_pipe_hits",core_str,cp->name);
    sprintf(buf2,"total number load hits in %s's fill buffers",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.fill_pipe_hits, 0, NULL);
    sprintf(buf,"%s%s.MPKC",core_str,cp->name);
    sprintf(buf2,"miss rate in MPKC (no prefetches) for %s",cp->name);
    sprintf(buf3,"(%s%s.misses / c%d.sim_cycle) * 1000.0",core_str,cp->name,id);
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
  }
  sprintf(buf,"%s%s.MSHR_total_occupancy",core_str,cp->name);
  sprintf(buf2,"cumulative MSHR occupancy in %s",cp->name);
  stat_reg_counter(sdb, false, buf, buf2, &cp->stat.MSHR_occupancy, 0, NULL);
  sprintf(buf,"%s%s.MSHR_avg_occupancy",core_str,cp->name);
  sprintf(buf2,"average MSHR entries in use in %s",cp->name);
  sprintf(buf3,"(%s%s.MSHR_total_occupancy / c%d.sim_cycle)",core_str,cp->name,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
  sprintf(buf,"%s%s.MSHR_full_cycles",core_str,cp->name);
  sprintf(buf2,"cycles MSHR was full in %s",cp->name);
  stat_reg_counter(sdb, false, buf, buf2, &cp->stat.MSHR_full_cycles, 0, NULL);
  sprintf(buf,"%s%s.MSHR_full",core_str,cp->name);
  sprintf(buf2,"fraction of time MSHRs are full in %s",cp->name);
  sprintf(buf3,"(%s%s.MSHR_full_cycles / c%d.sim_cycle)",core_str,cp->name,id);
  stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

  for(i=0;i<cp->num_prefetchers;i++)
    cp->prefetcher[i]->reg_stats(sdb,core);
}

void LLC_reg_stats(struct stat_sdb_t * sdb, struct cache_t * cp)
{
  char buf[256];
  char buf2[256];
  char buf3[256];
  int i;

  /* TODO: re-factor this */
  if(cp->read_only == CACHE_READWRITE)
  {
    sprintf(buf,"LLC.load_lookups");
    sprintf(buf2,"number of load lookups in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.load_lookups, 0, NULL);
    sprintf(buf,"LLC.load_misses");
    sprintf(buf2,"number of load misses in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.load_misses, 0, NULL);
    sprintf(buf,"LLC.load_miss_rate");
    sprintf(buf2,"load miss rate in LLC");
    sprintf(buf3,"LLC.load_misses/LLC.load_lookups");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"LLC.store_lookups");
    sprintf(buf2,"number of store lookups in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.store_lookups, 0, NULL);
    sprintf(buf,"LLC.store_misses");
    sprintf(buf2,"number of store misses in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.store_misses, 0, NULL);
    sprintf(buf,"LLC.store_miss_rate");
    sprintf(buf2,"store miss rate in LLC");
    sprintf(buf3,"LLC.store_misses/LLC.store_lookups");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"LLC.pf_lookups");
    sprintf(buf2,"number of prefetch lookups in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_lookups, 0, NULL);
    sprintf(buf,"LLC.pf_misses");
    sprintf(buf2,"number of prefetch misses in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_misses, 0, NULL);
    sprintf(buf,"LLC.pf_miss_rate");
    sprintf(buf2,"prefetch miss rate in LLC");
    sprintf(buf3,"LLC.pf_misses/LLC.pf_lookups");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"LLC.pf_insertions");
    sprintf(buf2,"number of prefetched blocks inserted into LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_insertions, 0, NULL);
    sprintf(buf,"LLC.pf_useful_insertions");
    sprintf(buf2,"number of prefetched blocks actually used in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_useful_insertions, 0, NULL);
    sprintf(buf,"LLC.pf_useful_rate");
    sprintf(buf2,"rate of useful prefetches in LLC");
    sprintf(buf3,"LLC.pf_useful_insertions/LLC.pf_insertions");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"LLC.total_lookups");
    sprintf(buf2,"total number of lookups in LLC");
    sprintf(buf3,"LLC.load_lookups + LLC.store_lookups + LLC.pf_lookups");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.0f");
    sprintf(buf,"LLC.total_misses");
    sprintf(buf2,"total number of misses in LLC");
    sprintf(buf3,"LLC.load_misses + LLC.store_misses + LLC.pf_misses");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.0f");
    sprintf(buf,"LLC.total_miss_rate");
    sprintf(buf2,"total miss rate in LLC");
    sprintf(buf3,"LLC.total_misses / LLC.total_lookups");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    if(num_threads > 1)
    {
      sprintf(buf,"LLC.MPKC");
      sprintf(buf2,"MPKC for the LLC (no prefetches)");
      sprintf(buf3,"((LLC.load_misses+LLC.store_misses) / sim_cycle) * 1000.0");
      stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
      sprintf(buf,"LLC.total_MPKC");
      sprintf(buf2,"MPKC for the LLC");
      sprintf(buf3,"((LLC.load_misses+LLC.store_misses+LLC.pf_misses) / sim_cycle) * 1000.0");
      stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    }

    sprintf(buf,"LLC.fill_pipe_hits");
    sprintf(buf2,"total number load hits in %s's fill buffers",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.fill_pipe_hits, 0, NULL);
    sprintf(buf,"LLC.WBB_accesses");
    sprintf(buf2,"total number of writebacks in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.WBB_accesses, 0, NULL);
    sprintf(buf,"LLC.WBB_combined");
    sprintf(buf2,"total writebacks eliminated due to write-combining in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.WBB_combines, 0, NULL);
    sprintf(buf,"LLC.WBB_frac_combined");
    sprintf(buf2,"fraction of writebacks eliminated due to write combining in LLC");
    sprintf(buf3,"LLC.WBB_combined / LLC.WBB_accesses");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"LLC.WBB_hits");
    sprintf(buf2,"total number load hits in %s's WBB",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.WBB_hits, 0, NULL);
    sprintf(buf,"LLC.WBB_total_occupancy");
    sprintf(buf2,"cumulative WBB occupancy in LLC");
    stat_reg_counter(sdb, false, buf, buf2, &cp->stat.WBB_occupancy, 0, NULL);
    sprintf(buf,"LLC.WBB_avg_occupancy");
    sprintf(buf2,"average WBB entries in use in LLC");
    sprintf(buf3,"(LLC.WBB_total_occupancy / sim_cycle)");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"LLC.WBB_full_cycles");
    sprintf(buf2,"cycles WBB was full in LLC");
    stat_reg_counter(sdb, false, buf, buf2, &cp->stat.WBB_full_cycles, 0, NULL);
    sprintf(buf,"LLC.WBB_full");
    sprintf(buf2,"fraction of time WBBs are full in LLC");
    sprintf(buf3,"(LLC.WBB_full_cycles / sim_cycle)");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
  }
  else
  {
    sprintf(buf,"LLC.lookups");
    sprintf(buf2,"number of lookups in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.load_lookups, 0, NULL);
    sprintf(buf,"LLC.misses");
    sprintf(buf2,"number of misses in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.load_misses, 0, NULL);
    sprintf(buf,"LLC.pf_lookups");
    sprintf(buf2,"number of prefetch lookups in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_lookups, 0, NULL);
    sprintf(buf,"LLC.pf_misses");
    sprintf(buf2,"number of prefetch misses in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_misses, 0, NULL);
    sprintf(buf,"LLC.pf_miss_rate");
    sprintf(buf2,"prefetch miss rate in LLC");
    sprintf(buf3,"LLC.pf_misses/LLC.pf_lookups");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

    sprintf(buf,"LLC.pf_insertions");
    sprintf(buf2,"number of prefetched blocks inserted into LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_insertions, 0, NULL);
    sprintf(buf,"LLC.pf_useful_insertions");
    sprintf(buf2,"number of prefetched blocks actually used in LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.prefetch_useful_insertions, 0, NULL);
    sprintf(buf,"LLC.pf_useful_rate");
    sprintf(buf2,"rate of useful prefetches in LLC");
    sprintf(buf3,"LLC.pf_useful_insertions/LLC.pf_insertions");

    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"LLC.miss_rate");
    sprintf(buf2,"miss rate in %s (no prefetches)",cp->name);
    sprintf(buf3,"LLC.misses/LLC.lookups");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"LLC.total_miss_rate");
    sprintf(buf2,"miss rate in LLC");
    sprintf(buf3,"(LLC.misses+LLC.pf_misses)/(LLC.lookups+LLC.pf_lookups)");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    if(num_threads > 1)
    {
      sprintf(buf,"LLC.MPKC");
      sprintf(buf2,"MPKC for the LLC (no prefetches)");
      sprintf(buf3,"(LLC.misses / sim_cycle) * 1000.0");
      stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
      sprintf(buf,"LLC.total_MPKC");
      sprintf(buf2,"MPKC for the LLC");
      sprintf(buf3,"((LLC.misses+LLC.pf_misses) / sim_cycle) * 1000.0");
      stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    }
    sprintf(buf,"LLC.fill_pipe_hits");
    sprintf(buf2,"total number load hits in %s's fill buffers",cp->name);
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.fill_pipe_hits, 0, NULL);
  }
  sprintf(buf,"LLC.MSHR_total_occupancy");
  sprintf(buf2,"cumulative MSHR occupancy in LLC");
  stat_reg_counter(sdb, false, buf, buf2, &cp->stat.MSHR_occupancy, 0, NULL);
  sprintf(buf,"LLC.MSHR_avg_occupancy");
  sprintf(buf2,"average MSHR entries in use in LLC");
  sprintf(buf3,"(LLC.MSHR_total_occupancy / sim_cycle)");
  stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
  sprintf(buf,"LLC.MSHR_full_cycles");
  sprintf(buf2,"cycles MSHR was full in LLC");
  stat_reg_counter(sdb, false, buf, buf2, &cp->stat.MSHR_full_cycles, 0, NULL);
  sprintf(buf,"LLC.MSHR_full");
  sprintf(buf2,"fraction of time MSHRs are full in LLC");
  sprintf(buf3,"(LLC.MSHR_full_cycles / sim_cycle)");
  stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");

  if(num_threads == 1)
  {
    sprintf(buf,"LLC.lookups");
    sprintf(buf2,"number of lookups in the LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.core_lookups[0], 0, NULL);
    sprintf(buf,"LLC.misses");
    sprintf(buf2,"number of misses in the LLC");
    stat_reg_counter(sdb, true, buf, buf2, &cp->stat.core_misses[0], 0, NULL);
    sprintf(buf,"LLC.MPKI");
    sprintf(buf2,"MPKI for the LLC");
    sprintf(buf3,"(LLC.misses / c0.commit_insn) * 1000.0");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    sprintf(buf,"LLC.MPKC");
    sprintf(buf2,"MPKC for the LLC");
    sprintf(buf3,"(LLC.misses / c0.sim_cycle) * 1000.0");
    stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
  }
  else
  {
    for(i=0;i<num_threads;i++)
    {
      sprintf(buf,"LLC.c%d.lookups",i);
      sprintf(buf2,"number of lookups by core %d in shared %s cache",i,cp->name);
      stat_reg_counter(sdb, true, buf, buf2, &cp->stat.core_lookups[i], 0, NULL);
      sprintf(buf,"LLC.c%d.misses",i);
      sprintf(buf2,"number of misses by core %d in shared %s cache",i,cp->name);
      stat_reg_counter(sdb, true, buf, buf2, &cp->stat.core_misses[i], 0, NULL);
      sprintf(buf,"LLC.c%d.MPKI",i);
      sprintf(buf2,"MPKI by core %d in shared %s cache",i,cp->name);
      sprintf(buf3,"(LLC.c%d.misses / c%d.commit_insn) * 1000.0",i,i);
      stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
      sprintf(buf,"LLC.c%d.MPKC",i);
      sprintf(buf2,"MPKC by core %d in shared %s cache",i,cp->name);
      sprintf(buf3,"(LLC.c%d.misses / c%d.sim_cycle) * 1000.0",i,i);
      stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
    }
  }

  for(i=0;i<cp->num_prefetchers;i++)
    cp->prefetcher[i]->reg_stats(sdb,NULL);
}

/* Called after fast-forwarding/warmup to clear out the stats */
void cache_reset_stats(struct cache_t * cp)
{
  counter_t * core_lookups = cp->stat.core_lookups;
  counter_t * core_misses = cp->stat.core_misses;
  memset(&cp->stat,0,sizeof(cp->stat));
  if(core_lookups)
  {
    cp->stat.core_lookups = core_lookups;
    cp->stat.core_misses = core_misses;
    memset(cp->stat.core_lookups,0,num_threads*sizeof(*cp->stat.core_lookups));
    memset(cp->stat.core_misses,0,num_threads*sizeof(*cp->stat.core_misses));
  }
}

/* Create an optional prefetch buffer so that prefetches get inserted here first
   before being promoted to the main cache. */
void prefetch_buffer_create(struct cache_t * cp, int num_entries)
{
  int i;
  cp->PF_buffer_size = num_entries;
  for(i=0;i<num_entries;i++)
  {
    struct prefetch_buffer_t * p = (struct prefetch_buffer_t*) calloc(1,sizeof(*p));
    if(!p)
      fatal("couldn't calloc prefetch buffer entry");
    p->addr = (md_paddr_t)-1;
    p->next = cp->PF_buffer;
    cp->PF_buffer = p;
  }
}

/* Create an optional prefetch filter that predicts whether a prefetch will be
   useful.  If it's predicted to not be, then don't bother inserting the
   prefetch into the cache.  TODO: this should be updated so that the prefetch
   doesn't even happen in the first place; the current implementation reduces
   cache pollution, but doesn't cut down on unnecessary bus utilization. */
void prefetch_filter_create(struct cache_t * cp, int num_entries, int reset_interval)
{
  int i;
  if(num_entries == 0)
    return;
  cp->PF_filter = (struct prefetch_filter_t*) calloc(1,sizeof(*cp->PF_filter));
  if(!cp->PF_filter)
    fatal("couldn't calloc prefetch filter");
  cp->PF_filter->num_entries = num_entries;
  cp->PF_filter->mask = num_entries-1;
  cp->PF_filter->reset_interval = reset_interval;

  cp->PF_filter->table = (char*) calloc(num_entries,sizeof(*cp->PF_filter->table));
  if(!cp->PF_filter->table)
    fatal("couldn't calloc prefetch filter table");
  for(i=0;i<num_entries;i++)
    cp->PF_filter->table[i] = 3;
}

/* Returns true if the prefetch should get inserted into the cache */
int prefetch_filter_lookup(struct prefetch_filter_t * p, md_paddr_t addr)
{
  if(sim_cycle >= (p->last_reset + p->reset_interval))
  {
    int i;
    for(i=0;i<p->num_entries;i++)
      p->table[i] = 3;
    p->last_reset = sim_cycle - (sim_cycle % p->reset_interval);
    return true;
  }

  int index = addr & p->mask;
  return (p->table[index] >= 2);
}

/* Update the filter's predictor based on whether a prefetch was useful or not. */
void prefetch_filter_update(struct prefetch_filter_t * p, md_paddr_t addr, int useful)
{
  int index = addr & p->mask;
  if(sim_cycle > (p->last_reset + p->reset_interval))
  {
    int i;
    for(i=0;i<p->num_entries;i++)
      p->table[i] = 3;
    p->last_reset = sim_cycle - (sim_cycle % p->reset_interval);
  }
  if(useful)
  {
    if(p->table[index] < 3)
      p->table[index]++;
  }
  else
  {
    if(p->table[index] > 0)
      p->table[index]--;
  }
}

/* create a generic bus that can be used to connect one or more caches */
struct bus_t * bus_create(char * name, int width, int ratio)
{
  struct bus_t * bus = (struct bus_t*) calloc(1,sizeof(*bus));
  if(!bus)
    fatal("failed to calloc bus %s",name);
  bus->name = strdup(name);
  bus->width = width;
  bus->ratio = ratio;
  return bus;
}

void bus_reg_stats(struct stat_sdb_t * sdb, struct core_t * core, struct bus_t * bus)
{
  char core_str[256];
  if(core)
    sprintf(core_str,"c%d.",core->current_thread->id);
  else
    core_str[0] = '\0'; /* empty string */

  char buf[256];
  char buf2[256];
  char buf3[256];

  sprintf(buf,"%s%s.accesses",core_str,bus->name);
  sprintf(buf2,"number of accesses to bus %s",bus->name);
  stat_reg_counter(sdb, true, buf, buf2, &bus->stat.accesses, 0, NULL);
  sprintf(buf,"%s%s.utilization",core_str,bus->name);
  sprintf(buf2,"cumulative cycles of utilization of bus %s",bus->name);
  stat_reg_counter(sdb, true, buf, buf2, &bus->stat.utilization, 0, NULL);
  sprintf(buf,"%s%s.avg_burst",core_str,bus->name);
  sprintf(buf2,"avg cycles utilized per transfer of bus %s",bus->name);
  sprintf(buf3,"%s%s.utilization/%s%s.accesses",core_str,bus->name,core_str,bus->name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
  sprintf(buf,"%s%s.duty_cycle",core_str,bus->name);
  sprintf(buf2,"fraction of time bus %s was in use",bus->name);
  sprintf(buf3,"%s%s.utilization/sim_cycle",core_str,bus->name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
  sprintf(buf,"%s%s.pf_utilization",core_str,bus->name);
  sprintf(buf2,"cumulative cycles of utilization of bus %s for prefetches",bus->name);
  stat_reg_counter(sdb, true, buf, buf2, &bus->stat.prefetch_utilization, 0, NULL);
  sprintf(buf,"%s%s.pf_duty_cycle",core_str,bus->name);
  sprintf(buf2,"fraction of time bus %s was in use for prefetches",bus->name);
  sprintf(buf3,"%s%s.pf_utilization/sim_cycle",core_str,bus->name);
  stat_reg_formula(sdb, true, buf, buf2, buf3, "%12.4f");
}

/* Returns true is the bus is available */
int bus_free(struct bus_t * bus)
{
  /* assume bus clock is locked to cpu clock (synchronous): only
     allow operations when cycle MOD bus-multiplier is zero */
  if(sim_cycle % bus->ratio)
    return false;
  else
    return (bus->when_available <= sim_cycle);
}

/* Make use of the bus, thereby making it NOT free for some number of cycles hence */
void bus_use(struct bus_t * bus, int transfer_size, int prefetch)
{
  int latency = (((transfer_size-1) / bus->width)+1) * bus->ratio; /* round up */
  bus->when_available = sim_cycle + latency;
  bus->stat.accesses++;
  bus->stat.utilization += latency;
  if(prefetch)
    bus->stat.prefetch_utilization += latency;
}


/* Check to see if a given address can be found in the cache.  This is only a "peek" function
   in that it does not update any hit/miss stats, although it does update replacement state. */
struct cache_line_t * cache_is_hit(struct cache_t * cp, enum cache_command cmd, md_paddr_t addr)
{
  md_paddr_t block_addr = addr >> cp->addr_shift;
  int index = block_addr & (cp->sets-1);
  struct cache_line_t * p = cp->blocks[index];
  struct cache_line_t * prev = NULL;

  while(p) /* search all of the ways */
  {
    if(p->valid && (p->tag == block_addr)) /* hit */
    {
      switch(cp->replacement_policy)
      {
        case REPLACE_PLRU: /* tree-based pseudo-LRU */
        {
          int bitmask = cp->blocks[index][0].meta;
          int way = p->way;
          for(int i=0;i<cp->log2_assoc;i++)
          {
            int pos = (way >> (cp->log2_assoc-i)) + (1<<i);
            
            if((way>>(cp->log2_assoc-i-1)) & 1)
              bitmask |= (1<<pos);
            else
              bitmask &= ~(1<<pos);
          }
          cp->blocks[index][0].meta = bitmask;
        }
          /* NO BREAK IN THE CASE STATEMENT HERE: Do the LRU ordering, too.
             This doesn't affect the behavior of the replacement policy, but
             for highly-associative caches it'll speed up any subsequent
             searches (assuming temporal locality) */

        case REPLACE_RANDOM: /* similar: random doesn't need it, but this can speed up searches */
        case REPLACE_LRU:
        case REPLACE_NMRU:
          {
            if(prev) /* insert back at front of list */
            {
              prev->next = p->next;
              p->next = cp->blocks[index];
              cp->blocks[index] = p;
            }
            break;
          }
        case REPLACE_MRU:
          {
            if(p->next) /* only move node if not already at end of list */
            {
              /* remove node */
              if(prev)
                prev->next = p->next;
              else
                cp->blocks[index] = p->next;

              /* go to end of list */
              prev = p->next;
              while(prev->next)
                prev = prev->next;
              
              /* stick ourselves there */
              prev->next = p;
              p->next = NULL;
            }
            break;
          }
        case REPLACE_CLOCK:
        {
          cp->blocks[index][0].meta |= 1ULL<<p->way; /* set referenced bit */
          break;
        }
        default:
          fatal("policy not yet implemented");
      }

      if(cmd == CACHE_WRITE)
      {
        cache_assert(cp->read_only != CACHE_READONLY,NULL);
        if(cp->write_policy == WRITE_BACK) /* write-thru doesn't have dirty lines */
          p->dirty = true;
      }

      return p;
    }
    prev = p;
    p = p->next;
  }

  /* miss */
  return NULL;
}

/* Overwrite the victim line with a new block.  This function only gets called after
   the previous evictee (as determined by the replacement policy) has already been
   removed, which means there is at least one invalid line (which we use for the
   newly inserted line). */
void cache_insert_block(struct cache_t * cp, enum cache_command cmd, md_paddr_t addr)
{
  /* assumes block not already present */
  md_paddr_t block_addr = addr >> cp->addr_shift;
  int index = block_addr & (cp->sets-1);
  struct cache_line_t * p = cp->blocks[index];
  struct cache_line_t *prev = NULL;

  /* there had better be an invalid line now - cache_get_evictee should
     have already returned a line to be invalidated */
  while(p)
  {
    if(!p->valid)
      break;
    prev = p;
    p = p->next;
  }

  cache_assert(p,(void)0);
  p->tag = block_addr;
  p->valid = true;
  if(cmd == CACHE_WRITE)
    p->dirty = true;
  else
    p->dirty = false;
  if(cmd == CACHE_PREFETCH)
  {
    p->prefetched = true;
    CACHE_STAT(cp->stat.prefetch_insertions++;)
  }

  switch(cp->replacement_policy)
  {
    case REPLACE_PLRU: /* tree-based pseudo-LRU */
    {
      int i;
      int bitmask = cp->blocks[index][0].meta;
      int way = p->way;
      for(i=0;i<cp->log2_assoc;i++)
      {
        int pos = (way >> (cp->log2_assoc-i)) + (1<<i);
        
        if((way>>(cp->log2_assoc-i-1)) & 1)
          bitmask |= (1<<pos);
        else
          bitmask &= ~(1<<pos);
      }
      cp->blocks[index][0].meta = bitmask;
    }
    /* same comment about case statement fall-through as in cache_is_hit() */
    case REPLACE_RANDOM:
    case REPLACE_LRU:
    case REPLACE_NMRU:
      if(prev) /* put to front of list */
      {
        prev->next = p->next;
        p->next = cp->blocks[index];
        cp->blocks[index] = p;
      }
      break;
    case REPLACE_MRU:
      if(p->next) /* only move node if not already at end of list */
      {
        /* remove node */
        if(prev)
          prev->next = p->next;
        else
          cp->blocks[index] = p->next;

        /* go to end of list */
        prev = p->next;
        while(prev->next)
          prev = prev->next;
        
        /* stick ourselves there */
        prev->next = p;
        p->next = NULL;
      }
      break;
    case REPLACE_CLOCK:
    {
      /* do not set referenced bit */
      break;
    }
    default:
      fatal("policy not yet implemented");
  }
}

/* Returns true if there's at least one free entry in the write-back buffer */
inline int WBB_available(struct cache_t * cp, md_paddr_t paddr)
{
  int bank = GET_MSHR_BANK(paddr);
  return cp->WBB_num[bank] < cp->WBB_size;
}

/* Insert a writeback request into the WBB; assumes you already called WBB_available
   to make sure there's room. */
void WBB_insert(struct cache_t * cp, struct cache_line_t * cache_block)
{
  /* don't need to cache invalid blocks */
  if(!cache_block->valid)
    return;

  CACHE_STAT(cp->stat.WBB_accesses++;)
  int bank = GET_MSHR_BANK(cache_block->tag << cp->addr_shift);

  if(cp->write_combining)
  {
    int i;
    for(i=0;i<cp->WBB_num[bank];i++)
    {
      int index = (cp->WBB_head[bank] + i) % cp->WBB_size;
      if(cp->WBB[bank][index].valid && (cp->WBB[bank][index].tag == cache_block->tag))
      {
        /* if can combine with another write that's already
           waiting in the WBB, then don't need to add new request */
        CACHE_STAT(cp->stat.WBB_combines++;)
        return;
      }
    }
  }

  struct cache_line_t * p = &cp->WBB[bank][cp->WBB_tail[bank]];

  p->tag = cache_block->tag;
  p->valid = cache_block->valid;
  p->dirty = cache_block->dirty;

  cp->WBB_num[bank]++;
  cp->WBB_tail[bank] = (cp->WBB_tail[bank] + 1) % cp->WBB_size;
}


/* Returns true if at least one MSHR entry is free/available. */
inline int MSHR_available(struct cache_t * cp, md_paddr_t paddr)
{
  int bank = GET_MSHR_BANK(paddr);
  return cp->MSHR_num[bank] < cp->MSHR_size;
}

/* Returns a pointer to a free MSHR entry; assumes you already called
   MSHR_available to make sure there's room */
struct cache_action_t * MSHR_allocate(struct cache_t * cp, md_paddr_t paddr)
{
  int i;
  int bank = GET_MSHR_BANK(paddr);

  for(i=0;i<cp->MSHR_size;i++)
    if(cp->MSHR[bank][i].cb == NULL)
    {
      cp->MSHR_num[bank]++;
      return &cp->MSHR[bank][i];
    }

  fatal("request for MSHR failed");
}

/* caller of get_evictee is responsible for writing back (if needed) and
   invalidating the entry prior to inserting a new block */
struct cache_line_t * cache_get_evictee(struct cache_t * cp, md_paddr_t addr)
{
  int block_addr = addr >> cp->addr_shift;
  int index = block_addr & (cp->sets-1);
  struct cache_line_t * p = cp->blocks[index];

  switch(cp->replacement_policy)
  {
    /* this works for both LRU and MRU, since MRU just sorts its recency list backwards */
    case REPLACE_LRU:
    case REPLACE_MRU:
    {
      while(p)
      {
        if(!p->next || !p->valid) /* take any invalid line, else take the last one (LRU) */
          return p;

        p = p->next;
      }
      break;
    }
    case REPLACE_RANDOM:
    {
      /* use an invalid block if possible */
      while(p)
      {
        if(!p->valid) /* take any invalid line */
          return p;

        p = p->next;
      }

      if(!p) /* no invalid line, pick at random */
      {
        int pos = random() % cp->assoc;
        int i;
        p = cp->blocks[index];

        for(i=0;i<pos;i++)
          p = p->next;

        return p;
      }

      break;
    }
    case REPLACE_NMRU:
    {
      /* use an invalid block if possible */
      while(p)
      {
        if(!p->valid) /* take any invalid line */
          return p;
        p = p->next;
      }

      if((cp->assoc > 1) && !p) /* no invalid line, pick at random from non-MRU */
      {
        int pos = random() % (cp->assoc-1);
        int i;
        p = cp->blocks[index];
        p = p->next; /* skip MRU */

        for(i=0;i<pos;i++)
          p = p->next;
      }
      return p;
    }
    case REPLACE_PLRU:
    {
      int bitmask = cp->blocks[index][0].meta;
      int i;
      int node = 1;

      while(p)
      {
        if(!p->valid) /* take any invalid line */
          return p;
        p = p->next;
      }

      for(i=0;i<cp->log2_assoc;i++)
      {
        int bit = (bitmask >> node) & 1;

        node = (node<<1) + !bit;
      }

      int way = node & ~(1<<cp->log2_assoc);

      p = cp->blocks[index];
      for(i=0;i<cp->assoc;i++)
      {
        if(p->way==way)
          break;
        p = p->next;
      }

      return p;
    }
    case REPLACE_CLOCK:
    {
      int just_in_case = 0;

      while(1)
      {
        qword_t way = cp->blocks[index][1].meta;
        struct cache_line_t * p = &cp->blocks[index][way];

        /* increment clock */
        cp->blocks[index][1].meta = (way+1) % cp->assoc;

        if(!p->valid) /* take any invalid line */
        {
          cp->blocks[index][0].meta &= ~(1ULL<<p->way); /* make sure referenced bit is clear */
          return p;
        }
        else if(!((cp->blocks[index][0].meta >> p->way) & 1)) /* not referenced */
        {
          return p;
        }
        else
        {
          cp->blocks[index][0].meta &= ~(1ULL<<p->way); /* clear referenced bit */
        }

        just_in_case++;
        if(just_in_case > (2*cp->assoc))
          fatal("Clock-PLRU has gone around twice without finding an evictee for %s",cp->name);
      }
    }
    default:
      fatal("policy not yet implemented");
  }

  fatal("unreachable code");
}

static void dummy_callback(void * p)
{
  /* this is just a place holder for cast-out writebacks
     in a write-back cache */
}

/* Called when a cache miss gets serviced.  May be recursively called for
   other (higher) cache hierarchy levels. */
void fill_arrived(struct cache_t * cp, int MSHR_bank, int MSHR_index)
{
  struct cache_action_t * MSHR = &cp->MSHR[MSHR_bank][MSHR_index];
  cache_assert(MSHR_index != NO_MSHR,(void)0);
  cache_assert(MSHR->cb,(void)0);

  MSHR->when_returned = sim_cycle;
  cp->MSHR_fill_num[MSHR_bank]++;

  if(MSHR->prev_cp)
    fill_arrived(MSHR->prev_cp,MSHR->MSHR_bank,MSHR->MSHR_index);
}

/* input state: assumes that the new node has just been inserted at
   insert_position and all remaining nodes obey the heap property */
void cache_heap_balance(struct cache_action_t * pipe, int insert_position)
{
  int pos = insert_position;
  //struct cache_action_t tmp;
  while(pos > 1)
  {
    int parent = pos >> 1;
    if(pipe[parent].pipe_exit_time > pipe[pos].pipe_exit_time)
    {
      /*
      tmp = pipe[parent];
      pipe[parent] = pipe[pos];
      pipe[pos] = tmp;
      */
      memswap(&pipe[parent],&pipe[pos],sizeof(*pipe));
      pos = parent;
    }
    else
      return;
  }
}

/* input state: pipe_num is the number of elements in the heap
   prior to removing the root node. */
void cache_heap_remove(struct cache_action_t * pipe, int pipe_num)
{
  if(pipe_num == 1) /* only one node to remove */
  {
    pipe[1].cb = NULL;
    pipe[1].pipe_exit_time = TICK_T_MAX;
    return;
  }

  pipe[1] = pipe[pipe_num]; /* move last node to root */
  //struct cache_action_t tmp;

  /* delete previous last node */
  pipe[pipe_num].cb = NULL;
  pipe[pipe_num].pipe_exit_time = TICK_T_MAX;

  /* push node down until heap property re-established */
  int pos = 1;
  while(1)
  {
    int Lindex = pos<<1;     // index of left child
    int Rindex = (pos<<1)+1; // index of right child
    tick_t myValue = pipe[pos].pipe_exit_time;

    tick_t Lvalue = TICK_T_MAX;
    tick_t Rvalue = TICK_T_MAX;
    if(Lindex < pipe_num) /* valid index */
      Lvalue = pipe[Lindex].pipe_exit_time;
    if(Rindex < pipe_num) /* valid index */
      Rvalue = pipe[Rindex].pipe_exit_time;

    if(((myValue > Lvalue) && (Lvalue != TICK_T_MAX)) ||
       ((myValue > Rvalue) && (Rvalue != TICK_T_MAX)))
    {
      if(Rvalue == TICK_T_MAX) /* swap pos with L */
      {
        /* tmp = pipe[Lindex];
        pipe[Lindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Lindex],&pipe[pos],sizeof(*pipe));
        return;
      }
      else if((myValue > Lvalue) && (Lvalue > Rvalue))
      {
        /* tmp = pipe[Lindex];
        pipe[Lindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Lindex],&pipe[pos],sizeof(*pipe));
        pos = Lindex;
      }
      else /* swap pos with R */
      {
        /* tmp = pipe[Rindex];
        pipe[Rindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Rindex],&pipe[pos],sizeof(*pipe));
        pos = Rindex;
      }
    }
    else
      return;
  }
}

void print_heap(struct cache_t * cp)
{
  int i;
  if(cp == uncore->LLC)
    return;
  for(i=0;i<cp->banks;i++)
  {
    fprintf(stderr,"%s[%d] <%d>: {",cp->name,i,cp->pipe_num[i]);
    for(int j=1;j<=cp->pipe_num[i];j++)
      fprintf(stderr," %lld",cp->pipe[i][j].pipe_exit_time);
    fprintf(stderr," }\n");
  }
}

/* Can a cache access get enqueued into the cache?  Returns true if it
   can.  Cases when it can't include, for example, more than one request
   per cycle goes to the same bank, or if a bank has been locked up. */
int cache_enqueuable(struct cache_t * cp, int id, md_paddr_t addr)
{
  md_paddr_t paddr;
  if(id == DO_NOT_TRANSLATE)
    paddr = addr;
  else
    paddr = v2p_translate(id,addr);
  int bank = GET_BANK(paddr);
  if(cp->pipe_num[bank] < cp->latency)
    return true;
  else
    return false;
#if 0
  if(cp->pipe[bank][0].cb == NULL)
    return true;
  else
    return false;
#endif
}

/* Enqueue a cache access request to the cache.  Assumes you already
   called cache_enqueuable. */
void cache_enqueue(struct core_t * core,
                   struct cache_t * cp,
                   struct cache_t * prev_cp,
                   enum cache_command cmd,
                   int id,
                   md_addr_t PC,
                   md_paddr_t addr,
                   seq_t action_id,
                   int MSHR_bank,
                   int MSHR_index,
                   void * op,
                   void (*cb)(void *),
                   void (*miss_cb)(void *, int),
                   bool (*translated_cb)(void *,seq_t),
                   seq_t (*get_action_id)(void *)
                  )
{
  md_paddr_t paddr;
  if(id == DO_NOT_TRANSLATE)
    paddr = addr;
  else
    paddr = v2p_translate(id,addr); /* for the IL1/DL1, we need a virtual-to-physical translation */
  int bank = GET_BANK(paddr);


  /* heap initial insertion position */
  int insert_position = cp->pipe_num[bank]+1;
  cache_assert(cp->pipe[bank][insert_position].cb == NULL,(void)0);
  cache_assert(cb != NULL,(void)0);

  cp->pipe[bank][insert_position].core = core;
  cp->pipe[bank][insert_position].prev_cp = prev_cp;
  cp->pipe[bank][insert_position].cmd = cmd;
  cp->pipe[bank][insert_position].PC = PC;
  cp->pipe[bank][insert_position].paddr = paddr;
  cp->pipe[bank][insert_position].op = op;
  cp->pipe[bank][insert_position].action_id = action_id;
  cp->pipe[bank][insert_position].MSHR_bank = MSHR_bank;
  cp->pipe[bank][insert_position].MSHR_index = MSHR_index;
  cp->pipe[bank][insert_position].cb = cb;
  cp->pipe[bank][insert_position].miss_cb = miss_cb;
  cp->pipe[bank][insert_position].translated_cb = translated_cb;
  cp->pipe[bank][insert_position].get_action_id = get_action_id;
  cp->pipe[bank][insert_position].when_started = sim_cycle;
  cp->pipe[bank][insert_position].when_returned = TICK_T_MAX;
  cp->pipe[bank][insert_position].pipe_exit_time = sim_cycle+cp->latency;

  assert(insert_position < cp->heap_size);
  cache_heap_balance(cp->pipe[bank],insert_position);

  cp->pipe_num[bank]++;


/* TODO: knob this */
//#define PREFETCH_ALL /* define this to prefetch on all accesses; else only prefetch on miss */
#ifdef PREFETCH_ALL
  if(PC && (cmd == CACHE_READ))
  {
    int i;
    for(i=0;i<cp->num_prefetchers;i++)
    {
      md_paddr_t pf_addr = cp->prefetcher[i]->lookup(PC,paddr);

      if(pf_addr & ~(PAGE_SIZE-1)) /* don't prefetch from zeroth page */
      {
        int j;

        /* search PFF to see if pf_addr already requested */
        int already_requested = false;
        for(j=0;j<cp->PFF_num;j++)
        {
          int index = (j+cp->PFF_head) % cp->PFF_size;
          if(cp->PFF[index].addr == pf_addr)
          {
            already_requested = true;
            break;
          }
        }
        if(already_requested)
          continue; /* for(i=0;...) */

        /* if FIFO full, overwrite oldest */
        if(cp->PFF_num == cp->PFF_size)
        {
          cp->PFF_head = (cp->PFF_head + 1) % cp->PFF_size;
          cp->PFF_num--;

        }

        cp->PFF[cp->PFF_tail].PC = PC;
        cp->PFF[cp->PFF_tail].addr = pf_addr;
        cp->PFF_tail = (cp->PFF_tail + 1) % cp->PFF_size;
        cp->PFF_num++;
      }
    }
  }
#endif
}

/* input state: assumes that the new node has just been inserted at
   insert_position and all remaining nodes obey the heap property */
void fill_heap_balance(struct cache_fill_t * pipe, int insert_position)
{
  int pos = insert_position;
  //struct cache_fill_t tmp;
  while(pos > 1)
  {
    int parent = pos >> 1;
    if(pipe[parent].pipe_exit_time > pipe[pos].pipe_exit_time)
    {
      /* tmp = pipe[parent];
      pipe[parent] = pipe[pos];
      pipe[pos] = tmp; */
      memswap(&pipe[parent],&pipe[pos],sizeof(*pipe));
      pos = parent;
    }
    else
      return;
  }
}

/* input state: pipe_num is the number of elements in the heap
   prior to removing the root node. */
void fill_heap_remove(struct cache_fill_t * pipe, int pipe_num)
{
  if(pipe_num == 1) /* only one node to remove */
  {
    pipe[1].valid = false;
    pipe[1].pipe_exit_time = TICK_T_MAX;
    return;
  }

  pipe[1] = pipe[pipe_num]; /* move last node to root */
  //struct cache_fill_t tmp;

  /* delete previous last node */
  pipe[pipe_num].valid = false;
  pipe[pipe_num].pipe_exit_time = TICK_T_MAX;

  /* push node down until heap property re-established */
  int pos = 1;
  while(1)
  {
    int Lindex = pos<<1;     // index of left child
    int Rindex = (pos<<1)+1; // index of right child
    tick_t myValue = pipe[pos].pipe_exit_time;

    tick_t Lvalue = TICK_T_MAX;
    tick_t Rvalue = TICK_T_MAX;
    if(Lindex < pipe_num) /* valid index */
      Lvalue = pipe[Lindex].pipe_exit_time;
    if(Rindex < pipe_num) /* valid index */
      Rvalue = pipe[Rindex].pipe_exit_time;

    if(((myValue > Lvalue) && (Lvalue != TICK_T_MAX)) ||
       ((myValue > Rvalue) && (Rvalue != TICK_T_MAX)))
    {
      if(Rvalue == TICK_T_MAX) /* swap pos with L */
      {
        /* tmp = pipe[Lindex];
        pipe[Lindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Lindex],&pipe[pos],sizeof(*pipe));
        return;
      }
      else if((myValue > Lvalue) && (Lvalue > Rvalue))
      {
        /* tmp = pipe[Lindex];
        pipe[Lindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Lindex],&pipe[pos],sizeof(*pipe));
        pos = Lindex;
      }
      else /* swap pos with R */
      {
        /* tmp = pipe[Rindex];
        pipe[Rindex] = pipe[pos];
        pipe[pos] = tmp; */
        memswap(&pipe[Rindex],&pipe[pos],sizeof(*pipe));
        pos = Rindex;
      }
    }
    else
      return;
  }
}
/* We are assuming separate ports for cache fills; returns true if the fill pipeline
   can accept a fill request. */
inline int cache_fillable(struct cache_t * cp, md_paddr_t paddr)
{
  int bank = GET_BANK(paddr);
  //if(!cp->fill_pipe[bank][0].valid)
  if(cp->fill_num[bank] < cp->latency)
    return true;
  else
    return false;
}

/* Do the actual fill; assume you already called cache_fillable */
inline void cache_fill(struct cache_t * cp,
                       enum cache_command cmd,
                       md_paddr_t paddr)
{
  int bank = GET_BANK(paddr);

  int insert_position = cp->fill_num[bank]+1;
  cache_assert(!cp->fill_pipe[bank][insert_position].valid,(void)0);

  cp->fill_pipe[bank][insert_position].valid = true;
  cp->fill_pipe[bank][insert_position].cmd = cmd;
  cp->fill_pipe[bank][insert_position].paddr = paddr;
  cp->fill_pipe[bank][insert_position].pipe_exit_time = sim_cycle+cp->latency;

  assert(insert_position < cp->heap_size);
  fill_heap_balance(cp->fill_pipe[bank],insert_position);

  cp->fill_num[bank]++;
}

/* simulate one cycle of the cache */
void cache_process(struct cache_t * cp)
{
  int b;
  int i;

  /* this must happen first to give these fills priority into the cache to make sure the
     MSHR's get deallocated to prevent deadlocks */
  int start_point = random() & cp->bank_mask; /* randomized arbitration */
  for(b=0;b<cp->MSHR_banks;b++)
  {
    int bank = (start_point + b) & cp->MSHR_mask;
    if(cp->MSHR_fill_num[bank])
    {
      for(i=0;i<cp->MSHR_size;i++)
      {
        /* process returned MSHR requests that now need to fill the cache */
        struct cache_action_t * MSHR = &cp->MSHR[bank][i];
        if(MSHR->cb && (MSHR->when_returned <= sim_cycle))
        {
          int insert = true;
          if(cp->PF_filter && (MSHR->cmd == CACHE_PREFETCH))
            insert = prefetch_filter_lookup(cp->PF_filter,(MSHR->paddr>>cp->addr_shift));

          /* if using prefetch buffer, insert there instead */
          if((MSHR->cmd == CACHE_PREFETCH) && (cp->PF_buffer) && insert)
          {
            struct prefetch_buffer_t *p, *evictee = NULL, *prev = NULL;
            p = cp->PF_buffer;
            while(p)
            {
              if((MSHR->paddr>>cp->addr_shift) == (p->addr>>cp->addr_shift)) /* already been prefetched */
                break;
              if(!p->next || (p->addr == (md_paddr_t)-1))
              {
                evictee = p;
                break;
              }
              prev = p;
              p = p->next;
            }

            if(evictee) /* not found, overwrite entry */
              p->addr = MSHR->paddr;

            /* else someone else already prefetched this addr */
            /* move to MRU position if not already there */
            if(prev)
            {
              prev->next = p->next;
              p->next = cp->PF_buffer;
              cp->PF_buffer = p;
            }
          }
          else if(  (MSHR->cmd == CACHE_READ) || (MSHR->cmd == CACHE_PREFETCH) ||
                   ((MSHR->cmd == CACHE_WRITE) && (cp->allocate_policy == WRITE_ALLOC)))
          {
            if(insert)
            {
              if(cache_fillable(cp,MSHR->paddr))
                cache_fill(cp,MSHR->cmd,MSHR->paddr);
              else
                continue; /* we'll have to try again next cycle */
            }
          }

          /* deallocate the MSHR */
          if(MSHR->cmd == CACHE_PREFETCH)
            cp->MSHR_num_pf[bank]--;
          MSHR->cb = NULL;
          cp->MSHR_num[bank]--;
          cp->MSHR_fill_num[bank]--;
          cache_assert(cp->MSHR_num[bank] >= 0,(void)0);
        }
      }
    }
  }

  /* process cache fills: */
  for(b=0;b<cp->banks;b++)
  {
    int bank = (start_point + b) & cp->bank_mask;

    struct cache_fill_t * cf = &cp->fill_pipe[bank][1];
    if(cp->fill_num[bank] && cf->valid && cf->pipe_exit_time <= sim_cycle)
    {
      if(!cache_is_hit(cp,cf->cmd,cf->paddr)) /* make sure hasn't been filled in the meantime */
      {
        struct cache_line_t * p = cache_get_evictee(cp,cf->paddr);
        if((cp->write_policy == WRITE_BACK) && p->valid && p->dirty)
        {
          if(!WBB_available(cp,cf->paddr))
            goto no_WBB_available; /* can't go until WBB is available */

          WBB_insert(cp,p);
        }

        if(p->prefetched && cp->PF_filter)
        {
          prefetch_filter_update(cp->PF_filter,p->tag,p->prefetch_used);
        }
        p->valid = p->dirty = false; /* this removes the copy in the cache since the WBB has it now */

        cache_insert_block(cp,cf->cmd,cf->paddr);
      }
      fill_heap_remove(cp->fill_pipe[bank],cp->fill_num[bank]);
      cp->fill_num[bank]--;
      cache_assert(cp->fill_num[bank] >= 0,(void)0);
    }
  no_WBB_available:
    ;
  }

  /* check last stage of cache pipes, process accesses */
  for(b=0;b<cp->banks;b++)
  {
    int bank = (start_point + b) & cp->bank_mask;

    struct cache_action_t * ca = &cp->pipe[bank][1]; // heap root
    if(cp->pipe_num[bank] && ca->cb && ca->pipe_exit_time <= sim_cycle)
    {
      struct cache_line_t * line = cache_is_hit(cp,ca->cmd,ca->paddr);
      if(line != NULL) /* if cache hit */
      {
        if((ca->cmd == CACHE_WRITE) && (cp->write_policy == WRITE_THROUGH))
        {
          if(!WBB_available(cp,ca->paddr))
            continue; /* can't go until WBB is available */

          struct cache_line_t tmp_line;
          tmp_line.tag = ca->paddr >> cp->addr_shift;
          tmp_line.valid = true;
          tmp_line.dirty = false;

          /* on a write-through cache, use a WBB entry to send write to the next level */
          WBB_insert(cp,&tmp_line);
        }

        /* invoke the call-back function; the action-id check makes sure the request is
           still valid (e.g., a request is invalid if the original uop that initiated
           the request had been flushed due to a branch misprediction). */
        if(ca->op && (ca->action_id == ca->get_action_id(ca->op)))
          ca->cb(ca->op);

        if((cp == uncore->LLC) && (ca->core))
          CACHE_STAT(cp->stat.core_lookups[ca->core->current_thread->id]++;)
        if(ca->cmd == CACHE_PREFETCH)
          CACHE_STAT(cp->stat.prefetch_lookups++;)
        else
        {
          if(ca->cmd == CACHE_READ)
            CACHE_STAT(cp->stat.load_lookups++;)
          else if(ca->cmd == CACHE_WRITE)
            CACHE_STAT(cp->stat.store_lookups++;)
          if(line->prefetched && !line->prefetch_used)
          {
            line->prefetch_used = true;
            CACHE_STAT(cp->stat.prefetch_useful_insertions++;)
          }
        }

        /* fill previous level as appropriate */
        if(ca->prev_cp)
          fill_arrived(ca->prev_cp,ca->MSHR_bank,ca->MSHR_index);
      }
      else /* miss in main data array */
      {
        int last_chance_hit = false;
#if 0
        if((ca->cmd == CACHE_READ) || (ca->cmd == CACHE_PREFETCH))
        {
          /* last chance: check in fill and WBBs - this implements something like a victim cache */
          int j;

          for(j=0;j<cp->latency;j++)
          {
            if(cp->fill_pipe[bank][j].valid &&
               (cp->fill_pipe[bank][j].paddr>>cp->addr_shift) == (ca->paddr >> cp->addr_shift))
            {
              CACHE_STAT(cp->stat.fill_pipe_hits++;)
              last_chance_hit = true;
              break;
            }
          }

          if(!last_chance_hit)
          {
            int this_bank = GET_MSHR_BANK(ca->paddr);
            for(j=0;j<cp->WBB_num[this_bank];j++)
            {
              int index = (cp->WBB_head[this_bank] + j)%cp->WBB_size;
              if(cp->WBB[this_bank][index].tag == (ca->paddr >> cp->addr_shift))
              {
                CACHE_STAT(cp->stat.WBB_hits++;)
                last_chance_hit = true;
                break;
              }
            }
          }

          if(!last_chance_hit && cp->PF_buffer)
          {
            struct prefetch_buffer_t * p = cp->PF_buffer, * prev = NULL;
            while(p)
            {
              if((p->addr>>cp->addr_shift) == (ca->paddr>>cp->addr_shift))
              {
                last_chance_hit = true;
                break;
              }
              prev = p;
              p = p->next;
            }

            if(p) /* hit in prefetch buffer */
            {
              last_chance_hit = true;

              /* insert block into cache */
              struct cache_line_t * evictee = cache_get_evictee(cp,ca->paddr);
              int ok_to_insert = true;
              if((cp->write_policy == WRITE_BACK) && evictee->valid && evictee->dirty)
              {
                if(WBB_available(cp,ca->paddr))
                {
                  WBB_insert(cp,evictee);
                }
                else /* we'd like to insert the prefetched line into the cache, but
                        in order to do so, we need to writeback the evictee but
                        we can't get a WBB.  */
                {
                  ok_to_insert = false;
                }
              }

              if(ok_to_insert && cache_fillable(cp,ca->paddr))
              {
                evictee->valid = evictee->dirty = false;
                cache_fill(cp,CACHE_PREFETCH,p->addr);
              }
              else /* if we couldn't do the insertion for some reason (e.g., no WBB
                      entry for dirty evictee or now fill bandwidth, move the
                      block back to the MRU position in the PF-buffer and hope
                      for better luck next time. */
              {
                if(prev)
                {
                  prev->next = p->next;
                  p->next = cp->PF_buffer;
                  cp->PF_buffer = p;
                }
              }
            }
          }
        }
#endif

        if(last_chance_hit)
        {
          if(ca->op && (ca->action_id == ca->get_action_id(ca->op)))
            ca->cb(ca->op);

          if((cp == uncore->LLC) && (ca->core))
            CACHE_STAT(cp->stat.core_lookups[ca->core->current_thread->id]++;)

          if(ca->cmd == CACHE_READ)
            CACHE_STAT(cp->stat.load_lookups++;)
          else if(ca->cmd == CACHE_PREFETCH)
            CACHE_STAT(cp->stat.prefetch_lookups++;)
          else if(ca->cmd == CACHE_WRITE)
            CACHE_STAT(cp->stat.store_lookups++;)

          /* fill previous level as appropriate */
          if(ca->prev_cp)
            fill_arrived(ca->prev_cp,ca->MSHR_bank,ca->MSHR_index);
        }
        else /* ok, we really missed */
        {
          /* place in MSHR */
          if(MSHR_available(cp,ca->paddr))
          {
            struct cache_action_t * MSHR = MSHR_allocate(cp,ca->paddr);
            *MSHR = *ca;
            MSHR->when_enqueued = sim_cycle;
            MSHR->when_started = TICK_T_MAX;
            MSHR->when_returned = TICK_T_MAX;
            int this_bank = GET_MSHR_BANK(ca->paddr);
            cp->MSHR_unprocessed_num[this_bank]++;

            /* update miss stats when requests *leaves* the cache pipeline to avoid double-counting */
            if((cp == uncore->LLC) && ca->core)
            {
              CACHE_STAT(cp->stat.core_lookups[ca->core->current_thread->id]++;)
              CACHE_STAT(cp->stat.core_misses[ca->core->current_thread->id]++;)
            }
            if(ca->cmd == CACHE_READ)
            {
              CACHE_STAT(cp->stat.load_lookups++;)
              CACHE_STAT(cp->stat.load_misses++;)
            }
            else if(ca->cmd == CACHE_PREFETCH)
            {
              CACHE_STAT(cp->stat.prefetch_lookups++;)
              CACHE_STAT(cp->stat.prefetch_misses++;)
              cp->MSHR_num_pf[MSHR->MSHR_bank]++;
            }
            else if(ca->cmd == CACHE_WRITE)
            {
              CACHE_STAT(cp->stat.store_lookups++;)
              CACHE_STAT(cp->stat.store_misses++;)
            }

#ifndef PREFETCH_ALL
            if(ca->PC && (ca->cmd == CACHE_READ))
            {
              int ii;
              for(ii=0;ii<cp->num_prefetchers;ii++)
              {
                md_paddr_t pf_addr = cp->prefetcher[ii]->lookup(ca->PC,ca->paddr);

                if(pf_addr & ~(PAGE_SIZE-1)) /* don't prefetch from zeroth page */
                {
                  int j;

                  /* search PFF to see if pf_addr already requested */
                  int already_requested = false;
                  for(j=0;j<cp->PFF_num;j++)
                  {
                    int index = (j+cp->PFF_head) % cp->PFF_size;
                    if(cp->PFF[index].addr == pf_addr)
                    {
                      already_requested = true;
                      break;
                    }
                  }
                  if(already_requested)
                    continue; /* for(i=0;...) */

                  /* if FIFO full, overwrite oldest */
                  if(cp->PFF_num == cp->PFF_size)
                  {
                    cp->PFF_head = (cp->PFF_head + 1) % cp->PFF_size;
                    cp->PFF_num--;

                  }

                  cp->PFF[cp->PFF_tail].PC = ca->PC;
                  cp->PFF[cp->PFF_tail].addr = pf_addr;
                  cp->PFF_tail = (cp->PFF_tail + 1) % cp->PFF_size;
                  cp->PFF_num++;
                }
              }
            }
#endif

          }
          else
            continue; /* this circumvents the "ca->cb = NULL" at the end of the loop, thereby leaving the ca in the pipe */
        }
      }

      cache_heap_remove(cp->pipe[bank],cp->pipe_num[bank]);
      cp->pipe_num[bank]--;
      cache_assert(cp->pipe_num[bank] >= 0,(void)0);
    }
  }

  /* We actually make three passes over the MSHR's; we give preference
     for demand misses (loads) first, then stores, and then prefetches
     get lowest priority. */
  enum cache_command cmd_order[3] = {CACHE_READ, CACHE_WRITE, CACHE_PREFETCH};

  int c;
  int sent_something = false;

  int bus_available = false;
  if(cp->next_level)
    bus_available = bus_free(cp->next_bus);
  else
    bus_available = bus_free(uncore->fsb);

  if(bus_available)
    for(c=0;c<3 && !sent_something;c++)
    {
      enum cache_command cmd = cmd_order[c];

      for(b=0;b<cp->MSHR_banks;b++)
      {
        int bank = (start_point+b) & cp->MSHR_mask;
        if(cp->MSHR_num[bank])
        {
          /* find oldest not-processed entry */
          if((!cp->next_bus || bus_free(cp->next_bus)) && (cp->MSHR_unprocessed_num[bank] > 0))
          {
            tick_t oldest_age = TICK_T_MAX;
            int index = -1;
            for(i=0;i<cp->MSHR_size;i++)
            {
              struct cache_action_t * MSHR = &cp->MSHR[bank][i];
              if((MSHR->cmd == cmd) && MSHR->cb && (MSHR->when_enqueued < oldest_age) && (!MSHR->translated_cb || MSHR->translated_cb(MSHR->op,MSHR->action_id))) /* if DL1, don't go until translated */
              {
                oldest_age = MSHR->when_enqueued;
                index = i;
              }
            }

            if(index != -1)
            {
              struct cache_action_t * MSHR = &cp->MSHR[bank][index];
              if(MSHR->cmd == cmd && MSHR->cb && (MSHR->when_started == TICK_T_MAX))
              {
                if(cp->next_level) /* enqueue the request to the next-level cache */
                {
                  if(cache_enqueuable(cp->next_level,DO_NOT_TRANSLATE,MSHR->paddr))
                  {
                    cache_enqueue(MSHR->core,cp->next_level,cp,MSHR->cmd,DO_NOT_TRANSLATE,MSHR->PC,MSHR->paddr,MSHR->action_id,bank,index/*our MSHR index*/,MSHR->op,MSHR->cb,MSHR->miss_cb,NULL,MSHR->get_action_id);
                    bus_use(cp->next_bus,(MSHR->cmd==CACHE_WRITE)?cp->linesize:1,cmd==CACHE_PREFETCH);
                    MSHR->when_started = sim_cycle;
                    cp->MSHR_unprocessed_num[bank]--;
                    cache_assert(cp->MSHR_unprocessed_num[bank] >= 0,(void)0);
                    if(MSHR->miss_cb && MSHR->op && (MSHR->action_id == MSHR->get_action_id(MSHR->op)))
                      MSHR->miss_cb(MSHR->op,cp->next_level->latency);
                    sent_something = true;
                    break;
                  }
                }
                else /* or if there is no next level, enqueue to the memory controller */
                { 
                  if(uncore->MC->enqueuable())
                  {
                    uncore->MC->enqueue(cp,MSHR->cmd,MSHR->paddr,cp->linesize,MSHR->action_id,bank/* our MSHR bank */,index/*our MSHR index*/,MSHR->op,MSHR->cb,MSHR->get_action_id);
                    bus_use(uncore->fsb,(MSHR->cmd==CACHE_WRITE)?(cp->linesize>>uncore->fsb_DDR):1,cmd==CACHE_PREFETCH);
                    MSHR->when_started = sim_cycle;
                    cp->MSHR_unprocessed_num[bank]--;
                    cache_assert(cp->MSHR_unprocessed_num[bank] >= 0,(void)0);
                    if(MSHR->miss_cb && MSHR->op && (MSHR->action_id == MSHR->get_action_id(MSHR->op)))
                      MSHR->miss_cb(MSHR->op,BIG_LATENCY);
                    sent_something = true;
                    break;
                  }
                }
              }
            }
          }
        }
      }
  }

  /* process write-backs */
  for(b=0;b<cp->MSHR_banks;b++)
  {
    int bank = (start_point+b) & cp->MSHR_mask;
    if(cp->WBB_num[bank]) /* there are cast-outs pending to be written back */
    {
      struct cache_line_t * WBB = &cp->WBB[bank][cp->WBB_head[bank]];
      if(cp->next_level)
      {
        if(!cp->next_bus || bus_free(cp->next_bus))
        {
          if(cache_enqueuable(cp->next_level,DO_NOT_TRANSLATE,WBB->tag << cp->addr_shift))
          {
            cache_enqueue(cp->core,cp->next_level,NULL,CACHE_WRITE,DO_NOT_TRANSLATE,0,WBB->tag << cp->addr_shift,(md_paddr_t)-1,0,NO_MSHR,NULL,dummy_callback,NULL,NULL,NULL);
            bus_use(cp->next_bus,cp->linesize,false);

            WBB->valid = WBB->dirty = false;
            cp->WBB_num[bank] --;
            cache_assert(cp->WBB_num[bank] >= 0,(void)0);
            cp->WBB_head[bank] = (cp->WBB_head[bank] + 1) % cp->WBB_size;
          }
        }
      }
      else
      {
        /* write back to main memory */
        if(bus_free(uncore->fsb) && uncore->MC->enqueuable())
        {
          uncore->MC->enqueue(NULL,CACHE_WRITE,WBB->tag << cp->addr_shift,cp->linesize,(md_paddr_t)-1,0,NO_MSHR,NULL,NULL,NULL);
          bus_use(uncore->fsb,cp->linesize>>uncore->fsb_DDR,false);

          WBB->valid = WBB->dirty = false;
          cp->WBB_num[bank] --;
          cache_assert(cp->WBB_num[bank] >= 0,(void)0);
          cp->WBB_head[bank] = (cp->WBB_head[bank] + 1) % cp->WBB_size;
        }
      }
    }
  }

  /* occupancy stats */
  int bank;
  int max_size = cp->MSHR_banks * cp->MSHR_size;
  int total_occ = 0;
  for(bank=0;bank<cp->MSHR_banks;bank++)
  {
    total_occ += cp->MSHR_num[bank];
  }
  CACHE_STAT(cp->stat.MSHR_occupancy += total_occ;)
  CACHE_STAT(cp->stat.MSHR_full_cycles += (total_occ == max_size);)

  max_size = cp->MSHR_banks * cp->WBB_size;
  total_occ = 0;
  for(bank=0;bank<cp->MSHR_banks;bank++)
  {
    total_occ += cp->WBB_num[bank];
  }
  CACHE_STAT(cp->stat.WBB_occupancy += total_occ;)
  CACHE_STAT(cp->stat.WBB_full_cycles += (total_occ == max_size);)
}

/* Attempt to enqueue a prefetch request, based on the predicted
   prefetch addresses in the prefetch FIFO (PFF) */
void cache_prefetch(struct cache_t * cp)
{
  /* if the PF controller says the bus hasn't been too busy */
  if(!cp->PF_sample_interval || (cp->PF_state == PF_OK))
  {
    /* check prefetch FIFO for new prefetch requests - max one per cycle */
    if(cp->PFF && cp->PFF_num)
    {
      md_paddr_t pf_addr = cp->PFF[cp->PFF_head].addr;
      int bank = GET_MSHR_BANK(pf_addr);

      if((cp->MSHR_num[bank] < cp->prefetch_threshold) /* if MSHR is too full, don't add more requests */
         && (cp->MSHR_num_pf[bank] < cp->prefetch_max))
      {
        md_addr_t pf_PC = cp->PFF[cp->PFF_head].PC;
        if(cache_enqueuable(cp,DO_NOT_TRANSLATE,pf_addr))
        {
          cache_enqueue(cp->core,cp,NULL,CACHE_PREFETCH,DO_NOT_TRANSLATE,pf_PC,pf_addr,(md_paddr_t)-1,bank,NO_MSHR,NULL,dummy_callback,NULL,NULL,NULL);
          cp->PFF_head = (cp->PFF_head+1) % cp->PFF_size;
          cp->PFF_num --;
          cache_assert(cp->PFF_num >= 0,(void)0);
        }
      }
    }
  }
}

/* advance all cache pipelines by one stage */
void cache_shuffle(struct cache_t * cp)
{
  /* XXX Nothing to do since we're using a heap now */
#if 0
  int b,i;

  /* advance pipes */
  for(b=0;b<cp->banks;b++)
  {
    if(cp->pipe_num[b]) /* don't need to shuffle if pipe is empty */
      for(i=cp->latency-1;i>0;i--)
      {
        /* access pipeline */
        if(cp->pipe[b][i].cb == NULL)
        {
          cp->pipe[b][i] = cp->pipe[b][i-1];
          cp->pipe[b][i-1].cb = NULL;
        }
      }

    if(cp->fill_num[b]) /* don't need to shuffle if pipe is empty */
      for(i=cp->latency-1;i>0;i--)
      {
        /* fill pipeline */
        if(cp->fill_pipe[b][i].valid == false)
        {
          cp->fill_pipe[b][i] = cp->fill_pipe[b][i-1];
          cp->fill_pipe[b][i-1].valid = false;
        }
      }
  }
#endif
}

/* Update the bus-utilization-based prefetch controller. */
void prefetch_controller_update(struct cache_t * cp)
{
  if(cp->PF_sample_interval && ((sim_cycle % cp->PF_sample_interval) == 0))
  {
    int current_sample = cp->next_bus->stat.utilization;
    double duty_cycle = (current_sample - cp->PF_last_sample) / (double) cp->PF_sample_interval;
    if(duty_cycle < cp->PF_low_watermark)
      cp->PF_state = PF_OK;
    else if(duty_cycle > cp->PF_high_watermark)
      cp->PF_state = PF_REFRAIN;
    /* else stay in current state */
    cp->PF_last_sample = current_sample;
  }
}

void step_LLC(struct uncore_t * uncore)
{
  cache_shuffle(uncore->LLC);

  /* update prefetch controllers */
  prefetch_controller_update(uncore->LLC);
}

void prefetch_LLC(struct uncore_t * uncore)
{
  cache_prefetch(uncore->LLC);
}

void step_L1_caches(struct core_t * core)
{
  cache_shuffle(core->memory.DL1);
  cache_shuffle(core->memory.IL1);

  cache_shuffle(core->memory.DTLB2);
  cache_shuffle(core->memory.DTLB);
  cache_shuffle(core->memory.ITLB);

  /* update prefetch controllers */
  prefetch_controller_update(core->memory.DL1);
  prefetch_controller_update(core->memory.IL1);
}

void prefetch_L1_caches(struct core_t * core)
{
  cache_prefetch(core->memory.DL1);
  cache_prefetch(core->memory.IL1);
}

void cache_freeze_stats(struct core_t * core)
{
  core->memory.IL1->frozen = true;
  core->memory.DL1->frozen = true;
  core->memory.ITLB->frozen = true;
  core->memory.DTLB->frozen = true;
  core->memory.DTLB2->frozen = true;
}


void cache_print(struct cache_t * cp)
{
  int i,j;
  fprintf(stderr,"<<<<< %s >>>>>\n",cp->name);
  for(i=0;i<cp->banks;i++)
  {
    fprintf(stderr,"bank[%d]: { ",i);
    for(j=0;j<cp->latency;j++)
    {
      if(cp->pipe[i][j].cb)
      {
        if(cp->pipe[i][j].cmd == CACHE_READ)
          fprintf(stderr,"L:");
        else if(cp->pipe[i][j].cmd == CACHE_WRITE)
          fprintf(stderr,"S:");
        else
          fprintf(stderr,"P:");
        fprintf(stderr,"%p(%lld)",cp->pipe[i][j].op,((struct uop_t*)cp->pipe[i][j].op)->decode.uop_seq);
      }
      else
        fprintf(stderr,"---");

      if(j != (cp->latency-1))
        fprintf(stderr,", ");
    }

    fprintf(stderr,"fill: { ");
    for(j=0;j<cp->latency;j++)
    {
      if(cp->fill_pipe[i][j].valid)
        fprintf(stderr,"%llx",cp->fill_pipe[i][j].paddr);
      else
        fprintf(stderr,"---");
      if(j != cp->latency-1)
        fprintf(stderr,", ");
    }
    fprintf(stderr," }\n");

    for(j=0;j<cp->MSHR_size;j++)
    {
      fprintf(stderr,"MSHR[%d]",j);
      if(cp->MSHR[i][j].cb)
      {
        fprintf(stderr," %c:",(cp->MSHR[i][j].cmd==CACHE_READ)?'L':'S');
        fprintf(stderr,"%p(%lld)",cp->MSHR[i][j].op,((struct uop_t*)cp->MSHR[i][j].op)->decode.uop_seq);
      }
      fprintf(stderr,"\n");
    }
    fprintf(stderr," }\n");
  }
}


