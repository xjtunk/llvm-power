/* zesto-dump.c */
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

#include "zesto-dump.h"

/* TODO: add more useful print functions */
/* TODO: migrate all print functions into individual classes */

/* For use in printing out uops one after the next, for example at
   a given point in the processor (e.g., all uops leaving decode).
   Format is the Mop, followed by the uop decomposition (uops are
   enumerated by the flow offsets/indexes). */
void print_uop_in_sequence(FILE * fp, struct uop_t * uop, int include_dep_info)
{
  struct Mop_t * Mop = uop->Mop;

  /* Mop level info */
  if(uop->flow_index == 0)
  {
    if(Mop->oracle.spec_mode)
      fprintf(fp,"*");
    else
      fprintf(fp," ");

    if(uop->decode.BOM)
      fprintf(fp,"B");
    else if(uop->decode.EOM)
      fprintf(fp,"E");
    else
      fprintf(fp," ");

    fprintf(fp,"  %lld <%s>",Mop->oracle.seq,MD_OP_ENUM(Mop->decode.op));
    if(Mop->fetch.inst.rep)
      fprintf(stderr,"  REP(%d)",Mop->decode.rep_seq);
    fprintf(fp,"\n");
  }

  /* uop info */
  if(uop->Mop->oracle.recover_inst && uop->decode.is_ctrl)
    fprintf(fp,"r");
  else
    fprintf(fp," ");

  fprintf(fp,"            [%d] %s",uop->flow_index,MD_OP_ENUM(uop->decode.op));
  if(include_dep_info)
  {
    fprintf(fp,"  IDEP=(%d,%d,%d) <%lld:%d, %lld:%d, %lld:%d>",
                    uop->decode.idep_name[0],uop->decode.idep_name[1],uop->decode.idep_name[2],
                    (uop->oracle.idep_uop[0]?uop->oracle.idep_uop[0]->Mop->oracle.seq:(seq_t)-1),
                    (uop->oracle.idep_uop[0]?uop->oracle.idep_uop[0]->flow_index:-1),
                    (uop->oracle.idep_uop[1]?uop->oracle.idep_uop[1]->Mop->oracle.seq:(seq_t)-1),
                    (uop->oracle.idep_uop[1]?uop->oracle.idep_uop[1]->flow_index:-1),
                    (uop->oracle.idep_uop[2]?uop->oracle.idep_uop[2]->Mop->oracle.seq:(seq_t)-1),
                    (uop->oracle.idep_uop[2]?uop->oracle.idep_uop[2]->flow_index:-1));
    fprintf(fp,"  ODEP=(%d) ",uop->decode.odep_name);
    if(uop->oracle.odep_uop)
    {
      struct odep_t * p = uop->oracle.odep_uop;
      fprintf(fp,"<");
      while(p)
      {
        fprintf(fp,"%lld:%d(%d)",p->uop->Mop->oracle.seq,p->uop->flow_index,p->op_num);
        p = p->next;
        if(p)
          fprintf(fp,", ");
      }
      fprintf(fp,">");
    }
  }
  fprintf(fp,"\n");
}

/* print out an entire Mop all at once */
void print_Mop(FILE * fp, struct Mop_t * Mop, int include_dep_info)
{
  int i=0;
  while(i<Mop->decode.flow_length)
  {
    struct uop_t * uop = &Mop->uop[i];
    print_uop_in_sequence(fp,uop,include_dep_info);
    i += uop->decode.has_imm ? 3 : 1;
  }
}

/* Simple uop print function - no dependency info */
void print_uop(FILE * fp, struct uop_t * uop)
{
  if(uop->Mop->oracle.spec_mode)
    fprintf(fp,"*");
  else
    fprintf(fp," ");

  if(uop->decode.BOM)
    fprintf(fp,"B");
  else
    fprintf(fp," ");

  if(uop->decode.EOM)
    fprintf(fp,"E");
  else
    fprintf(fp," ");

  if(uop->Mop->oracle.recover_inst && uop->decode.is_ctrl)
    fprintf(fp,"r");
  else
    fprintf(fp," ");

  fprintf(fp,"            [%d] %s",uop->flow_index,MD_OP_ENUM(uop->decode.op));

  fprintf(fp,"  IDEP=(%d,%d,%d) ",uop->decode.idep_name[0],uop->decode.idep_name[1],uop->decode.idep_name[2]);
  fprintf(fp,"  ODEP=(%d) ",uop->decode.odep_name);

  fprintf(fp,"\n");
}



