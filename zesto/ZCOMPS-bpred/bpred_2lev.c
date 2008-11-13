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

/*--------------------------------------------------------*/
/* 2-Level predictor (GAg, GAp, PAg, PAp, gshare, pshare) */
/* [Yeh and Patt, Rahmeh et al, McFarling, Evers et al.]  */
/*--------------------------------------------------------*/
#include <math.h>
#define COMPONENT_NAME "2lev"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int l1_size;
  int l2_size;
  int his_width;
  int Xor;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d",name,&l1_size,&l2_size,&his_width,&Xor) != 5)
    fatal("bad bpred options string %s (should be \"2lev:name:l1_size:l2_size:his_width:xor\")",opt_string);
  return new bpred_2lev_t(name,l1_size,l2_size,his_width,Xor);
}
#else

class bpred_2lev_t:public bpred_dir_t
{
  class bpred_2lev_sc_t:public bpred_sc_t
  {
    public:
    my2bc_t * current_ctr;
    int * bhr;
    int lookup_bhr;
  };

  protected:

  int bht_size;
  int bht_mask;
  int * bht;
  int pht_size;
  int pht_mask;
  my2bc_t * pht;
  int history_length;
  int history_mask;
  int Xor; /* all lower-case "xor" is reserved keyword in C++. yuck! >:-P */
  int xorshift;

  public:

  /* CREATE */
  bpred_2lev_t(char * arg_name,
               int arg_bht_size,
               int arg_pht_size,
               int arg_history_length,
               int arg_Xor
              )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_bht_size);
    CHECK_PPOW2(arg_pht_size);
    CHECK_NNEG(arg_history_length);
    CHECK_BOOL(arg_Xor);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc 2lev name (strdup)");
    type = strdup("2lev");
    if(!type)
      fatal("couldn't malloc 2lev type (strdup)");

    bht_size = arg_bht_size;
    bht_mask = arg_bht_size-1;
    pht_size = arg_pht_size;
    pht_mask = arg_pht_size-1;
    history_length = arg_history_length;
    history_mask = (1<<arg_history_length)-1;
    Xor = arg_Xor;
    if(Xor)
    {
      xorshift = log_base2(pht_size)-history_length;
      if(xorshift < 0)
        xorshift = 0;
    }

    bht = (int*) calloc(bht_size,sizeof(*bht));
    if(!bht)
      fatal("couldn't malloc 2lev BHT");
    pht = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    if(!pht)
      fatal("couldn't malloc 2lev PHT");
    for(int i=0;i<pht_size;i++)
      pht[i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;

    bits = bht_size*history_length + pht_size*2;
  }

  /* DESTROY */
  ~bpred_2lev_t()
  {
    if(pht) free(pht); pht = NULL;
    if(bht) free(bht); bht = NULL;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;
    int l1index = PC&bht_mask;
    int l2index;
    sc->bhr = &bht[l1index];

    if(Xor)
    {
      l2index = PC^(*sc->bhr<<xorshift);
    }
    else
    {
      l2index = (PC<<history_length)|*sc->bhr;
    }

    l2index &= pht_mask;

    sc->current_ctr = &pht[l2index];
    sc->updated = FALSE;

    BPRED_STAT(lookups++;)
    sc->lookup_bhr = *sc->bhr;

    return MY2BC_DIR(*sc->current_ctr);
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;

    MY2BC_UPDATE(*sc->current_ctr,outcome);

    if(!sc->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)
      sc->updated = TRUE;
    }
  }

  /* SPEC UPDATE */
  BPRED_SPEC_UPDATE_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;

    BPRED_STAT(spec_updates++;)

    *sc->bhr = ((*sc->bhr<<1)|our_pred)&history_mask;
  }

  BPRED_RECOVER_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;
    if(bht_size == 1) // global history recovery only
        bht[0] = (sc->lookup_bhr<<1)|outcome;
  }

  BPRED_FLUSH_HEADER
  {
    class bpred_2lev_sc_t * sc = (class bpred_2lev_sc_t*) scvp;
    if(bht_size == 1) // global history recovery only
        bht[0] = sc->lookup_bhr;
  }

  /* GETCACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_2lev_sc_t * sc = new bpred_2lev_sc_t();
    if(!sc)
      fatal("couldn't malloc 2lev State Cache");
    return sc;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
