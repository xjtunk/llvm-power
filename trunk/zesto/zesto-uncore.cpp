/* zesto-uncore.cpp */
/*
 * Copyright (C) 2007 by Gabriel H. Loh and the Georgia Tech
 * Research Corporation Atlanta, GA  30332-0415 All Rights Reserved.
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
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * NOTE: Portions of this release are directly derived from the
 * SimpleScalar Toolset (property of SimpleScalar LLC), and as such,
 * those portions are bound by the corresponding legal terms and
 * conditions.  All source files derived directly or in part from
 * the SimpleScalar Toolset bear the original user agreement.
 * 
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
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
 *   noncommercial scholarship purposes provided that this notice in
 *   its entirety accompanies all copies.  Copies of the modified
 *   software can be delivered to persons who use it solely for
 *   nonprofit, educational, noncommercial research, and
 *   noncommercial scholarship purposes provided that this notice in
 *   its entirety accompanies all copies."
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
 * for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is
 * distributed by someone who received only the executable form, and
 * is accompanied by a copy of the written offer of source code.
 * 
 * 7. Zesto was developed by Gabriel H. Loh, Ph.D.  US Mail: 266
 * Ferst Drive, Georgia Institute of Technology, Atlanta, GA
 * 30332-0765
 * 
 */

#include <limits.h>
#include "thread.h"
#include "stats.h"
#include "options.h"
#include "zesto-core.h"
#include "zesto-opts.h"
#include "zesto-cache.h"
#include "zesto-prefetch.h"
#include "zesto-uncore.h"
#include "zesto-dram.h"
#include "zesto-MC.h"


/* The uncore class is just a wrapper around the last-level cache,
   front-side-bus and memory controller, plus relevant parameters.
 */


/* prototype for call to zesto-MC.c */
MC_t * MC_from_string(char * opt_string);

static double cpu_speed; /* CPU speed in MHz */
static int fsb_width;    /* in bytes */
static bool fsb_DDR;     /* true if DDR */
static double fsb_speed; /* in MHz */
static char * MC_opt_string = NULL;
static char * LLC_opt_str = "LLC:4096:16:64:16:64:9:L:W:B:8:1:8:C";
static int LLC_bus_ratio = 1;
static int LLC_access_rate = 1;

/* LLC prefetcher options */
static char * LLC_PF_opt_str[MAX_PREFETCHERS];
static int LLC_num_PF = 0;
static int LLC_PFFsize = 8;
static int LLC_PFthresh = 2;
static int LLC_PFmax = 1;
static int LLC_PF_buffer_size = 0;
static int LLC_PF_filter_size = 0;
static int LLC_PF_filter_reset = 0;
static int LLC_WMinterval = 10000;
static double LLC_low_watermark = 0.1;
static double LLC_high_watermark = 0.3;

/* The global pointer to the uncore object */
class uncore_t * uncore = NULL;

/* constructor */
uncore_t::uncore_t(double arg_cpu_speed,
                   int arg_fsb_width,
                   int arg_fsb_DDR,
                   double arg_fsb_speed,
                   char * MC_opt_string
              ): cpu_speed(arg_cpu_speed),
                 fsb_speed(arg_fsb_speed),
                 fsb_DDR(arg_fsb_DDR)
{
  /* temp variables for option-string parsing */
  char name[256];
  int sets, assoc, linesize, latency, banks, bank_width, MSHR_banks, MSHR_entries, WBB_entries;
  char rp, ap, wp, wc;

  fsb_width = arg_fsb_width;
  fsb_bits = log_base2(fsb_width);
  cpu_ratio = (int)ceil(cpu_speed/fsb_speed);
  if(cpu_ratio<=0)
    fatal("CPU to mem bus speed ratio (cpu_ratio) must be greater than zero"); 

  fsb = bus_create("FSB",fsb_width,cpu_ratio);
  MC = MC_from_string(MC_opt_string);

  /* Shared LLC */
  if(sscanf(LLC_opt_str,"%[^:]:%d:%d:%d:%d:%d:%d:%c:%c:%c:%d:%d:%d:%c",
      name,&sets,&assoc,&linesize,&banks,&bank_width,&latency,&rp,&ap,&wp, &MSHR_entries, &MSHR_banks, &WBB_entries, &wc) != 14)
    fatal("invalid LLC options: <name:sets:assoc:linesize:banks:bank-width:latency:repl-policy:alloc-policy:write-policy:num-MSHR:WB-buffers:write-combining>\n\t(%s)",LLC_opt_str);

  LLC = cache_create(NULL,name,CACHE_READWRITE,sets,assoc,linesize,
                     rp,ap,wp,wc,banks,bank_width,latency,
                     WBB_entries,MSHR_entries,MSHR_banks,NULL,fsb);
  if(LLC_access_rate & (LLC_access_rate-1))
    fatal("-LLC:rate must be power of two");
  LLC_cycle_mask = LLC_access_rate-1;

  LLC->PFF_size = LLC_PFFsize;
  LLC->PFF = (cache_t::PFF_t*) calloc(LLC_PFFsize,sizeof(*LLC->PFF));
  if(!LLC->PFF)
    fatal("failed to calloc %s's prefetch FIFO",LLC->name);
  prefetch_buffer_create(LLC,LLC_PF_buffer_size);
  prefetch_filter_create(LLC,LLC_PF_filter_size,LLC_PF_filter_reset);
  LLC->prefetch_threshold = LLC_PFthresh;
  LLC->prefetch_max = LLC_PFmax;
  LLC->PF_low_watermark = LLC_low_watermark;
  LLC->PF_high_watermark = LLC_high_watermark;
  LLC->PF_sample_interval = LLC_WMinterval;

  LLC->prefetcher = (struct prefetch_t**) calloc(LLC_num_PF?LLC_num_PF:1/* avoid 0-size alloc */,sizeof(*LLC->prefetcher));
  LLC->num_prefetchers = LLC_num_PF;
  if(!LLC->prefetcher)
    fatal("couldn't calloc %s's prefetcher array",LLC->name);
  for(int i=0;i<LLC_num_PF;i++)
    LLC->prefetcher[i] = prefetch_create(LLC_PF_opt_str[i],LLC);
  if(LLC->prefetcher[0] == NULL)
    LLC->num_prefetchers = LLC_num_PF = 0;

  LLC_bus = bus_create("LLC_bus",LLC->linesize,LLC_bus_ratio);
}

/* destructor */
uncore_t::~uncore_t()
{
  delete(MC);
  MC = NULL;
}

void  
uncore_reg_options(struct opt_odb_t *odb)
{
  opt_reg_string(odb, "-LLC","last-level data cache configuration",
      &LLC_opt_str, /*default*/ "LLC:2048:16:64:16:64:12:L:W:B:8:1:8:C", /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-LLC:bus","cpu clock cycles per LLC-bus cycle",
      &LLC_bus_ratio, /*default*/ LLC_bus_ratio, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-LLC:rate","access LLC once per this many cpu clock cycles",
      &LLC_access_rate, /*default*/ LLC_access_rate, /*print*/true,/*format*/NULL);

  /* LLC prefetch control options */
  opt_reg_string_list(odb, "-LLC:pf", "2nd-level data cache prefetcher configuration",
      LLC_PF_opt_str, MAX_PREFETCHERS, &LLC_num_PF, LLC_PF_opt_str, /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int(odb, "-LLC:pf:fifosize","LLC prefetch FIFO size",
      &LLC_PFFsize, /*default*/ 16, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-LLC:pf:buffer","LLC prefetch buffer size",
      &LLC_PF_buffer_size, /*default*/ 0, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-LLC:pf:filter","LLC prefetch filter size",
      &LLC_PF_filter_size, /*default*/ 0, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-LLC:pf:filterreset","LLC prefetch filter reset interval (cycles)",
      &LLC_PF_filter_reset, /*default*/ 65536, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-LLC:pf:thresh","LLC prefetch threshold (only prefetch if MSHR occupancy < thresh)",
      &LLC_PFthresh, /*default*/ 4, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-LLC:pf:max","max LLC prefetch requests in MSHRs",
      &LLC_PFmax, /*default*/ 2, /*print*/true,/*format*/NULL);
  opt_reg_double(odb, "-LLC:pf:lowWM","low watermark for prefetch control",
      &LLC_low_watermark, /*default*/ 0.1, /*print*/true,/*format*/NULL);
  opt_reg_double(odb, "-LLC:pf:highWM","high watermark for prefetch control",
      &LLC_high_watermark, /*default*/ 0.5, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-LLC:pf:WMinterval","sampling interval (in cycles) for prefetch control (0 = no PF controller)",
      &LLC_WMinterval, /*default*/ 100, /*print*/true,/*format*/NULL);


  opt_reg_int(odb, "-fsb:width", "front-side bus width (in bytes)",
      &fsb_width, /* default */8, /* print */true, /* format */NULL);
  opt_reg_flag(odb, "-fsb:ddr", "front-side bus double-pumped data (DDR)",
      &fsb_DDR, /* default */false, /* print */true, /* format */NULL);
  opt_reg_double(odb, "-fsb:speed", "front-side bus speed in MHz",
      &fsb_speed, /*default*/400.0,/*print*/true,/*format*/NULL); 
  opt_reg_double(odb, "-cpu:speed", "CPU speed in MHz",
      &cpu_speed, /*default*/2000.0,/*print*/true,/*format*/NULL); 
  opt_reg_string(odb, "-MC", "memory controller config string",
      &MC_opt_string,/*default */"simple:16:0",/*print*/true,/*format*/NULL);
}

void uncore_create(void)
{
  uncore = new uncore_t(cpu_speed,fsb_width,fsb_DDR,fsb_speed,MC_opt_string);
}

/* register all of the stats */
void uncore_reg_stats(struct stat_sdb_t * sdb)
{
  stat_reg_note(sdb,"\n#### SHARED L2 STATS ####");
  LLC_reg_stats(sdb, uncore->LLC);
  bus_reg_stats(sdb, NULL, uncore->LLC_bus);

  stat_reg_note(sdb,"\n#### MAIN MEMORY/DRAM STATS ####");

  dram->reg_stats(sdb);

  bus_reg_stats(sdb,NULL,uncore->fsb);

  stat_reg_int(sdb, true, "FSB.clock_ratio", "CPU clocks per FSB clock",
      &uncore->cpu_ratio, uncore->cpu_ratio, /* format */NULL);

  uncore->MC->reg_stats(sdb);
}

