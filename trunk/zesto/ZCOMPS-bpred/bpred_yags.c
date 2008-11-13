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

/*--------------------------------------------*/
/* yags (Yet Another Global Scheme) Predictor */
/* [Eden and Mudge, MICRO'98]                 */
/*--------------------------------------------*/
#include <math.h>
#define COMPONENT_NAME "yags"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int l1_size;
  int l2_size;
  int his_width;
  int Xor;
  int cache_size;
  int tag_width;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d:%d:%d",name,&l1_size,&l2_size,&his_width,&Xor,&cache_size,&tag_width) != 7)
    fatal("bad bpred options string %s (should be \"yags:name:l1_size:l2_size:his_width:xor:cache_size:tag_width\")",opt_string);
  return new bpred_yags_t(name,l1_size,l2_size,his_width,Xor,cache_size,tag_width);
}
#else

class bpred_yags_t:public bpred_dir_t
{
  struct yags_cache_t{
    unsigned char tag;
    my2bc_t ctr;
  };

  class bpred_yags_sc_t:public bpred_sc_t
  {
    public:
    my2bc_t * pht_ctr;
    struct yags_cache_t * cache_entry;
    int * bhr;
    char tag_match;
    char pred;
    char choice;
    int lookup_bhr;
  };

  protected:

  int bht_size;
  int bht_mask;
  int * bht;
  int pht_size;
  int pht_mask;
  int tag_width;
  int tag_mask;
  int cache_size;
  int cache_mask;
  my2bc_t * pht;
  struct yags_cache_t * T_cache;
  struct yags_cache_t * NT_cache;

  int history_length;
  int history_mask;
  int Xor;
  int xorshift;

  public:

  /* CREATE */
  bpred_yags_t(char * arg_name,
               int arg_bht_size,
               int arg_pht_size,
               int arg_history_length,
               int arg_Xor,
               int arg_cache_size,
               int arg_tag_width
              )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_bht_size);
    CHECK_PPOW2(arg_pht_size);
    CHECK_PPOW2(arg_cache_size);
    CHECK_NNEG(arg_history_length);
    CHECK_BOOL(arg_Xor);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc yags name (strdup)");
    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc yags type (strdup)");

    bht_size = arg_bht_size;
    bht_mask = arg_bht_size-1;
    pht_size = arg_pht_size;
    pht_mask = arg_pht_size-1;
    tag_width = arg_tag_width;
    tag_mask = (1<<arg_tag_width)-1;
    cache_size = arg_cache_size;
    cache_mask = arg_cache_size-1;
    history_length = arg_history_length;
    history_mask = (1<<arg_history_length)-1;
    Xor = arg_Xor;
    if(arg_Xor)
    {
      xorshift = log_base2(arg_cache_size)-arg_history_length;
      if(xorshift < 0)
        xorshift = 0;
    }

    bht = (int*) calloc(bht_size,sizeof(int));
    if(!bht)
      fatal("couldn't malloc yags BHT");
    pht = (my2bc_t*) calloc(pht_size,sizeof(my2bc_t));
    if(!pht)
      fatal("couldn't malloc yags PHT");
    for(int i=0;i<pht_size;i++)
      pht[i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
    T_cache = (struct yags_cache_t*) calloc(cache_size,sizeof(struct yags_cache_t));
    NT_cache = (struct yags_cache_t*) calloc(cache_size,sizeof(struct yags_cache_t));
    if(!T_cache || !NT_cache)
      fatal("couldn't malloc yags cache");
    for(int i=0;i<cache_size;i++)
    {
      T_cache[i].ctr = MY2BC_WEAKLY_TAKEN;
      NT_cache[i].ctr = MY2BC_WEAKLY_NT;
    }

    bits =  bht_size*history_length + pht_size*2
           + (tag_width+2)*cache_size * 2;
  }

  /* DESTROY */
  ~bpred_yags_t()
  {
    free(T_cache); T_cache = NULL;
    free(NT_cache); NT_cache = NULL;
    free(pht); pht = NULL;
    free(bht); bht = NULL;
  }

  /* LOOKUP */
  BPRED_LOOKUP_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    int l1index = PC&bht_mask;
    int l2index;
    char pred;

    sc->bhr = &bht[l1index];

    /* "choice PHT" */
    sc->pht_ctr = &pht[PC&pht_mask];
    pred = MY2BC_DIR(*sc->pht_ctr);
    sc->choice = pred;

    if(Xor)
      l2index = PC^(*sc->bhr<<xorshift);
    else
      l2index = (PC<<history_length)|*sc->bhr;
    l2index &= cache_mask;

    sc->cache_entry = (pred)
                     ? (&NT_cache[l2index])
                     : (&T_cache[l2index]);
    sc->tag_match = sc->cache_entry->tag == (PC & tag_mask);

    if(sc->tag_match)
      pred = MY2BC_DIR(sc->cache_entry->ctr);

    BPRED_STAT(lookups++;)

    sc->pred = pred;
    sc->updated = FALSE;
    sc->lookup_bhr = *sc->bhr;

    return pred;
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    char pred = sc->pred;
    char choice = sc->choice;

    /*
    choice = MY2BC_DIR(*sc->pht_ctr);
    pred = (sc->tag_match)
         ? MY2BC_DIR(sc->cache_entry->ctr)
         : MY2BC_DIR(*sc->pht_ctr);
     */

    if(sc->tag_match)
    {
      MY2BC_UPDATE(sc->cache_entry->ctr,outcome);
    }
    else if(choice ^ outcome)
    {
      MY2BC_UPDATE(sc->cache_entry->ctr,outcome);
      sc->cache_entry->tag = PC & tag_mask;
    }
    if(!(pred == outcome && choice != outcome))
      MY2BC_UPDATE(*sc->pht_ctr,outcome);

    if(!sc->updated)
    {
        BPRED_STAT(updates++;)
        BPRED_STAT(num_hits += our_pred == outcome;)
        sc->updated = TRUE;
    }
  }

  /* SPEC_UPDATE */
  BPRED_SPEC_UPDATE_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    BPRED_STAT(spec_updates++;)

    *sc->bhr = ((*sc->bhr<<1)|our_pred)&history_mask;
  }

  /* RECOVER */
  BPRED_RECOVER_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    if(bht_size == 1)
        bht[0] = (sc->lookup_bhr<<1)|outcome;
  }

  /* FLUSH */
  BPRED_FLUSH_HEADER
  {
    class bpred_yags_sc_t * sc = (class bpred_yags_sc_t*) scvp;
    if(bht_size == 1)
      bht[0] = sc->lookup_bhr;
  }

  /* GET_CACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_yags_sc_t * sc = new bpred_yags_sc_t();
    if(!sc)
      fatal("couldn't malloc yags State Cache");
    return sc;
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
