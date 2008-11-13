/* zesto-exec.cpp */
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

#include "zesto-core.h"
#include "zesto-opts.h"
#include "zesto-oracle.h"
#include "zesto-alloc.h"
#include "zesto-exec.h"
#include "zesto-memdep.h"
#include "zesto-prefetch.h"
#include "zesto-uncore.h"


void exec_reg_options(struct opt_odb_t * odb, struct core_knobs_t * knobs)
{
  /**********************/
  /* buffer/queue sizes */
  /**********************/
  opt_reg_int(odb, "-rs:size","number of reservation station entries",
      &knobs->exec.RS_size, /*default*/ knobs->exec.RS_size, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-ldq:size","number of load queue entries",
      &knobs->exec.LDQ_size, /*default*/ knobs->exec.LDQ_size, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-stq:size","number of load queue entries",
      &knobs->exec.STQ_size, /*default*/ knobs->exec.STQ_size, /*print*/true,/*format*/NULL);

  opt_reg_int(odb, "-exec:width","maximum issues from RS per cycle (equal to num exec ports)",
      &knobs->exec.num_exec_ports, /*default*/ knobs->exec.num_exec_ports, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-payload:depth","number of cycles for payload RAM access (schedule-to-exec delay)",
      &knobs->exec.payload_depth, /*default*/ knobs->exec.payload_depth, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-fp:penalty","extra cycle(s) to forward results to FP cluster",
      &knobs->exec.fp_penalty, /*default*/ knobs->exec.fp_penalty, /*print*/true,/*format*/NULL);

  /*****************************************************************/
  /* functional units (number of bindings implies number of units) */
  /*****************************************************************/
  opt_reg_int_list(odb, "-pb:ieu", "IEU port binding", knobs->exec.fu_bindings[FU_IEU],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_IEU].num_FUs, knobs->exec.fu_bindings[FU_IEU], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:jeu", "JEU port binding", knobs->exec.fu_bindings[FU_JEU],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_JEU].num_FUs, knobs->exec.fu_bindings[FU_JEU], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:imul", "IMUL port binding", knobs->exec.fu_bindings[FU_IMUL],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_IMUL].num_FUs, knobs->exec.fu_bindings[FU_IMUL], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:idiv", "IDIV port binding", knobs->exec.fu_bindings[FU_IDIV],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_IDIV].num_FUs, knobs->exec.fu_bindings[FU_IDIV], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:shift", "SHIFT port binding", knobs->exec.fu_bindings[FU_SHIFT],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_SHIFT].num_FUs, knobs->exec.fu_bindings[FU_SHIFT], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:fadd", "FADD port binding", knobs->exec.fu_bindings[FU_FADD],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_FADD].num_FUs, knobs->exec.fu_bindings[FU_FADD], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:fmul", "FMUL port binding", knobs->exec.fu_bindings[FU_FMUL],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_FMUL].num_FUs, knobs->exec.fu_bindings[FU_FMUL], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:fdiv", "FDIV port binding", knobs->exec.fu_bindings[FU_FDIV],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_FDIV].num_FUs, knobs->exec.fu_bindings[FU_FDIV], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:fcplx", "FCPLX port binding", knobs->exec.fu_bindings[FU_FCPLX],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_FCPLX].num_FUs, knobs->exec.fu_bindings[FU_FCPLX], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:lda", "LD port binding", knobs->exec.fu_bindings[FU_LD],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_LD].num_FUs, knobs->exec.fu_bindings[FU_LD], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:sta", "STA port binding", knobs->exec.fu_bindings[FU_STA],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_STA].num_FUs, knobs->exec.fu_bindings[FU_STA], /* print */true, /* format */NULL, /* !accrue */false);
  opt_reg_int_list(odb, "-pb:std", "STD port binding", knobs->exec.fu_bindings[FU_STD],
      MAX_EXEC_WIDTH, &knobs->exec.port_binding[FU_STD].num_FUs, knobs->exec.fu_bindings[FU_STD], /* print */true, /* format */NULL, /* !accrue */false);

  /*****************************/
  /* functional unit latencies */
  /*****************************/

  opt_reg_int(odb, "-ieu:lat","IEU execution latency",
      &knobs->exec.latency[FU_IEU], /*default*/ knobs->exec.latency[FU_IEU], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-jeu:lat","JEU execution latency",
      &knobs->exec.latency[FU_JEU], /*default*/ knobs->exec.latency[FU_JEU], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-imul:lat","IMUL execution latency",
      &knobs->exec.latency[FU_IMUL], /*default*/ knobs->exec.latency[FU_IMUL], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-idiv:lat","IDIV execution latency",
      &knobs->exec.latency[FU_IDIV], /*default*/ knobs->exec.latency[FU_IDIV], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-shift:lat","SHIFT execution latency",
      &knobs->exec.latency[FU_SHIFT], /*default*/ knobs->exec.latency[FU_SHIFT], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-fadd:lat","FADD execution latency",
      &knobs->exec.latency[FU_FADD], /*default*/ knobs->exec.latency[FU_FADD], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-fmul:lat","FMUL execution latency",
      &knobs->exec.latency[FU_FMUL], /*default*/ knobs->exec.latency[FU_FMUL], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-fdiv:lat","FDIV execution latency",
      &knobs->exec.latency[FU_FDIV], /*default*/ knobs->exec.latency[FU_FDIV], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-fcplx:lat","FCPLX execution latency",
      &knobs->exec.latency[FU_FCPLX], /*default*/ knobs->exec.latency[FU_FCPLX], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-lda:lat","LD-agen execution latency",
      &knobs->exec.latency[FU_LD], /*default*/ knobs->exec.latency[FU_LD], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-sta:lat","ST-agen execution latency",
      &knobs->exec.latency[FU_STA], /*default*/ knobs->exec.latency[FU_STA], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-std:lat","ST-data execution latency",
      &knobs->exec.latency[FU_STD], /*default*/ knobs->exec.latency[FU_STD], /*print*/true,/*format*/NULL);

  /*******************************/
  /* functional unit issue rates */
  /*******************************/

  opt_reg_int(odb, "-ieu:rate","IEU execution issue rate",
      &knobs->exec.issue_rate[FU_IEU], /*default*/ knobs->exec.issue_rate[FU_IEU], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-jeu:rate","JEU execution issue rate",
      &knobs->exec.issue_rate[FU_JEU], /*default*/ knobs->exec.issue_rate[FU_JEU], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-imul:rate","IMUL execution issue rate",
      &knobs->exec.issue_rate[FU_IMUL], /*default*/ knobs->exec.issue_rate[FU_IMUL], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-idiv:rate","IDIV execution issue rate",
      &knobs->exec.issue_rate[FU_IDIV], /*default*/ knobs->exec.issue_rate[FU_IDIV], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-shift:rate","SHIFT execution issue rate",
      &knobs->exec.issue_rate[FU_SHIFT], /*default*/ knobs->exec.issue_rate[FU_SHIFT], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-fadd:rate","FADD execution issue rate",
      &knobs->exec.issue_rate[FU_FADD], /*default*/ knobs->exec.issue_rate[FU_FADD], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-fmul:rate","FMUL execution issue rate",
      &knobs->exec.issue_rate[FU_FMUL], /*default*/ knobs->exec.issue_rate[FU_FMUL], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-fdiv:rate","FDIV execution issue rate",
      &knobs->exec.issue_rate[FU_FDIV], /*default*/ knobs->exec.issue_rate[FU_FDIV], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-fcplx:rate","FCPLX execution issue rate",
      &knobs->exec.issue_rate[FU_FCPLX], /*default*/ knobs->exec.issue_rate[FU_FCPLX], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-lda:rate","LD-agen execution issue rate",
      &knobs->exec.issue_rate[FU_LD], /*default*/ knobs->exec.issue_rate[FU_LD], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-sta:rate","ST-agen execution issue rate",
      &knobs->exec.issue_rate[FU_STA], /*default*/ knobs->exec.issue_rate[FU_STA], /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-std:rate","ST-data execution issue rate",
      &knobs->exec.issue_rate[FU_STD], /*default*/ knobs->exec.issue_rate[FU_STD], /*print*/true,/*format*/NULL);

  /***************/
  /* data caches */
  /***************/
  opt_reg_string(odb, "-dl1","1st-level data cache configuration",
      &knobs->memory.DL1_opt_str, /*default*/ "DL1:64:8:64:8:16:2:L:W:T:8:8:C", /*print*/true,/*format*/NULL);

  opt_reg_string(odb, "-dtlb","data TLB configuration",
      &knobs->memory.DTLB_opt_str, /*default*/ "DTLB:4:4:1:2:L:4", /*print*/true,/*format*/NULL);
  opt_reg_string(odb, "-dtlb2","L2 data TLB configuration",
      &knobs->memory.DTLB2_opt_str, /*default*/ "DTLB2:64:4:1:4:L:1", /*print*/true,/*format*/NULL);

  opt_reg_string_list(odb, "-dl1:pf", "1st-level data cache prefetcher configuration",
      knobs->memory.DL1PF_opt_str, MAX_PREFETCHERS, &knobs->memory.DL1_num_PF, knobs->memory.DL1PF_opt_str, /* print */true, /* format */NULL, /* !accrue */false);

  /* DL1 prefetch control options */
  opt_reg_int(odb, "-dl1:pf:fifosize","dl1 prefetch FIFO size",
      &knobs->memory.DL1_PFFsize, /*default*/ 8, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-dl1:pf:buffer","dl1 prefetch buffer size",
      &knobs->memory.DL1_PF_buffer_size, /*default*/ 0, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-dl1:pf:filter","dl1 prefetch filter size",
      &knobs->memory.DL1_PF_filter_size, /*default*/ 0, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-dl1:pf:filterreset","dl1 prefetch filter reset interval (cycles)",
      &knobs->memory.DL1_PF_filter_reset, /*default*/ 65536, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-dl1:pf:thresh","dl1 prefetch threshold (only prefetch if MSHR occupancy < thresh)",
      &knobs->memory.DL1_PFthresh, /*default*/ 4, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-dl1:pf:max","max dl1 prefetch requests in MSHRs",
      &knobs->memory.DL1_PFmax, /*default*/ 2, /*print*/true,/*format*/NULL);
  opt_reg_double(odb, "-dl1:pf:lowWM","low watermark for prefetch control",
      &knobs->memory.DL1_low_watermark, /*default*/ 0.3, /*print*/true,/*format*/NULL);
  opt_reg_double(odb, "-dl1:pf:highWM","high watermark for prefetch control",
      &knobs->memory.DL1_high_watermark, /*default*/ 0.7, /*print*/true,/*format*/NULL);
  opt_reg_int(odb, "-dl1:pf:WMinterval","sampling interval (in cycles) for prefetch control (0 = no PF controller)",
      &knobs->memory.DL1_WMinterval, /*default*/ 100, /*print*/true,/*format*/NULL);

  opt_reg_flag(odb, "-warm:caches","warm caches during functional fast-forwarding",
      &knobs->memory.warm_caches, /*default*/ false, /*print*/true,/*format*/NULL);


  /*******************************/
  /* memory dependence predictor */
  /*******************************/
  opt_reg_string(odb, "-memdep","memory dependence predictor configuration",
      &knobs->exec.memdep_opt_str, /*default*/ "lwt:LWT:4096:131072", /*print*/true,/*format*/NULL);
}


/* default constructor */
core_exec_t::core_exec_t(void):
  last_completed(0), last_completed_count(0)
{
}


/* load in all definitions */
#include "ZPIPE-exec.list"


class core_exec_t * exec_create(const char * exec_opt_string, struct core_t * core)
{
#define ZESTO_PARSE_ARGS
#include "ZPIPE-exec.list"

  fatal("unknown exec type \"%s\"",exec_opt_string);
#undef ZESTO_PARSE_ARGS
}
