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

/*------------------------------------------*/
/* 2bC (2-bit saturating counter) [Smith89] */
/*------------------------------------------*/
#define COMPONENT_NAME "2bC"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int num_entries;
           
  if(sscanf(opt_string,"%*[^:]:%[^:]:%d",name,&num_entries) != 2)
    fatal("bad bpred options string %s (should be \"2bc:name:num_entries\")",opt_string);
  return new bpred_2bc_t(name,num_entries);
}
#else

class bpred_2bc_t:public bpred_dir_t
{
  class bpred_2bc_sc_t:public bpred_sc_t
  {
    public:
    my2bc_t * current_ctr;
  };

  protected:

  int num_entries;
  int entry_mask;
  my2bc_t * table;

  public:

  /* CREATE */
  bpred_2bc_t(char * arg_name,
              int arg_num_entries
             )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_num_entries);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc 2bc name (strdup)");
    type = strdup("2bc");
    if(!type)
      fatal("couldn't malloc 2bc type (strdup)");

    num_entries = arg_num_entries;
    entry_mask = arg_num_entries-1;
    table = (my2bc_t*) calloc(arg_num_entries,sizeof(my2bc_t));
    if(!table)
      fatal("couldn't malloc 2bc table");
    for(int i=0;i<num_entries;i++)
      table[i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;

    bits = num_entries * 2;
  }

  /* DESTROY */
  ~bpred_2bc_t()
  {
    if(table) free(table); table = NULL;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_2bc_sc_t * sc = (class bpred_2bc_sc_t*) scvp;
    sc->current_ctr = &table[PC&entry_mask];
    sc->updated = FALSE;
    BPRED_STAT(lookups++;)
    return MY2BC_DIR(*sc->current_ctr);
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_2bc_sc_t * sc = (class bpred_2bc_sc_t*) scvp;
    if(!sc->updated)
    {
      BPRED_STAT(num_hits += our_pred == outcome;)
      BPRED_STAT(updates++;)
      sc->updated = TRUE;
    }
    MY2BC_UPDATE(*sc->current_ctr,outcome);
  }

  /* GET_CACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_2bc_sc_t * sc = new bpred_2bc_sc_t();
    if(!sc)
      fatal("couldn't malloc 2bc State Cache");
    return sc;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
