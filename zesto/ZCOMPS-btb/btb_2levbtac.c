/*---------------------------------------------------------*/
/* Fixed capacity BTB, with global branch history indexing */
/*---------------------------------------------------------*/
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
#define COMPONENT_NAME "2levbtac"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int l1size;
  int hist_width;
  int num_ents;
  int num_ways;
  int Xor;
  int tag_width;
  char replace_policy;
  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d:%d:%d:%d:%c",name,&l1size,&hist_width,&Xor,&num_ents,&num_ways,&tag_width,&replace_policy) != 8)
    fatal("bad BTB options string %s (should be \"2levbtac:name:l1size:hist_width:xor:entries:ways:tag_width:replacement_policy\")",opt_string);
  return new BTB_2levbtac_t(name,l1size,hist_width,Xor,num_ents,num_ways,tag_width,replace_policy);
}
#else

class BTB_2levbtac_t:public BTB_t
{

  /* struct definitions */
  struct BTB_2levbtac_Entry_t
  {
    md_addr_t PC;
    md_addr_t target;
    struct BTB_2levbtac_Entry_t * prev;
    struct BTB_2levbtac_Entry_t * next;
  };

  class BTB_2levbtac_sc_t:public BTB_sc_t
  {
    public:
    struct BTB_2levbtac_Entry_t * p;
    struct BTB_2levbtac_Entry_t ** set;
    int * bhr;
    int lookup_bhr;
  };

  protected:

  int bht_size;
  int hist_width;
  int history_mask;
  int num_entries;
  int num_ways;
  int tag_width;
  char replacement_policy;
  struct BTB_2levbtac_Entry_t ** set;
  int *bht;

  int bht_mask;
  int btac_mask;

  int tag_shift;
  int tag_mask;

  int Xor;
  int xorshift;

  public:


  /* CREATE */
  BTB_2levbtac_t(char * arg_name,
                 int arg_bht_size,
                 int arg_hist_width,
                 int arg_Xor,
                 int arg_num_entries,
                 int arg_num_ways,
                 int arg_tag_width,
                 char arg_replacement_policy
                )
  {
    init();

    /* TODO: error check parameters */
    CHECK_PPOW2(arg_bht_size);
    CHECK_PPOW2(arg_num_entries);
    CHECK_POS(arg_num_ways);
    CHECK_BOOL(arg_Xor);
    if(tolower(arg_replacement_policy) != 'l' &&
       tolower(arg_replacement_policy) != 'r')
      fatal("%s(%s) %s must be 'l' (LRU) or 'r' (random).",arg_name,COMPONENT_NAME,"replacement_policy");

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc 2levbtac name (strdup)");

    bht_size = arg_bht_size;
    hist_width = arg_hist_width;
    Xor = arg_Xor;
    num_entries = arg_num_entries;
    num_ways = arg_num_ways;
    replacement_policy = arg_replacement_policy;
    tag_width = arg_tag_width;

    bht_mask = arg_bht_size-1;

    history_mask = (1<<hist_width)-1;

    if(Xor)
    {
      xorshift = log_base2(num_entries)-hist_width;
      if(xorshift < 0)
        xorshift = 0;
    }


    btac_mask = num_entries-1;

    tag_shift = log_base2(num_entries);
    tag_mask = (1<<tag_width)-1;

    bht = (int*) calloc(bht_size,sizeof(int));
    if(!bht)
      fatal("couldn't malloc 2lev BHT");

    set = (struct BTB_2levbtac_Entry_t**) calloc(num_entries,sizeof(*set));
    if(!set)
      fatal("couldn't malloc 2levbtac set array");
    for(int i=0;i<num_entries;i++)
    {
      struct BTB_2levbtac_Entry_t * p;
      for(int j=0;j<num_ways;j++)
      {
        p = (struct BTB_2levbtac_Entry_t*) calloc(1,sizeof(**set));
        if(!p)
          fatal("couldn't malloc 2levbtac entry");
        if(set[i])
        {
          p->next = set[i];
          set[i]->prev = p;
          set[i] = p;
        }
        else
          set[i] = p;
      }
    }

    int bhtsize = bht_size*hist_width;
    int tagsize = 8*sizeof(md_addr_t) - log_base2(num_entries);
    int lrusize = log_base2(num_ways);
    int entrysize = sizeof(md_addr_t) + tagsize + lrusize + 1; /* +1 for valid bit */
    bits =  num_entries*num_ways*entrysize + bhtsize;
    type = strdup("2levbtac Branch Target Predictor");
  }

  /* DESTROY */
  ~BTB_2levbtac_t()
  {
    free(name); name = NULL;
    for(int i=0;i<num_entries;i++)
    {
      struct BTB_2levbtac_Entry_t * p;
      struct BTB_2levbtac_Entry_t * next = NULL;
      for(p=set[i];p;p = next)
      {
        next = p->next;
        free(p);
      }
    }
    free(bht); bht = NULL;
    free(type); type = NULL;
  }

  /* LOOKUP */
  BTB_LOOKUP_HEADER
  {
    class BTB_2levbtac_sc_t * sc = (class BTB_2levbtac_sc_t*) scvp;
    int l1index;
    int l2index;
    struct BTB_2levbtac_Entry_t * p;
    
    l1index = PC&bht_mask;
    sc->bhr = &bht[l1index];
    sc->set = NULL;

    if(Xor)
    {
      l2index = PC^(*sc->bhr<<xorshift);
    }
    else
    {
      l2index = (PC<<hist_width)|*sc->bhr;
    }

    l2index &= btac_mask;
    p = set[l2index];

    BPRED_STAT(lookups++;)
    sc->updated = FALSE;
    sc->lookup_bhr = *sc->bhr;

    sc->set = &set[l2index];
    sc->p = NULL;

    while(p)
    {
      if(p->PC == ((PC>>tag_shift)&tag_mask))
      {
        sc->p = p;
        return p->target;
      }
      p = p->next;
    }

    return 0;
  }

  /* UPDATE */
  BTB_UPDATE_HEADER
  {
    class BTB_2levbtac_sc_t * sc = (class BTB_2levbtac_sc_t*) scvp;

    if(!sc->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += oraclePC == our_target;)
      sc->updated = TRUE;
    }

    if(!outcome) /* don't store not-taken targets */
    {
      BPRED_STAT(num_nt++;)
      return;
    }


    if(!our_pred)
    {
      int l2index;
      struct BTB_2levbtac_Entry_t * p;

      if(Xor)
      {
        l2index = PC^(*sc->bhr<<xorshift);
      }
      else
      {
        l2index = (PC<<hist_width)|*sc->bhr;
      }

      l2index &= btac_mask;
      p = set[l2index];

      BPRED_STAT(lookups++;)

      sc->set = &set[l2index];
      sc->p = NULL;

      while(p)
      {
        if(p->PC == ((PC>>tag_shift)&tag_mask))
        {
          sc->p = p;
          break;
        }
        p = p->next;
      }
    }

    if(sc->p)
    {
      /* had a BTB hit; update the target, put to front of list */
      sc->p->target = oraclePC;
      
      if(sc->p->prev == NULL) /* already at head */
        return;
      sc->p->prev->next = sc->p->next;
      if(sc->p->next)
        sc->p->next->prev = sc->p->prev;

      (*sc->set)->prev = sc->p;
      sc->p->prev = NULL;
      sc->p->next = (*sc->set);
      *sc->set = sc->p;
      return;
    }
    else
    {
      /* was a BTB miss; evict entry depending on replacement policy,
         update, and put to front of list */
      struct BTB_2levbtac_Entry_t * p;
      int pos=0;
      int randpos=0;
      p = *sc->set;
      if(replacement_policy == 'r')
        randpos = random()%num_ways;

      while(p)
      {
        if(replacement_policy == 'l' && !p->next)
          break;
        if(replacement_policy == 'r' && pos==randpos)
          break;
        else
          pos++;
        p = p->next;
      }

      assert(p != NULL);

      p->target = oraclePC;
      p->PC = (PC>>tag_shift)&tag_mask;
      
      if(p->prev == NULL) /* already at head */
        return;

      p->prev->next = p->next;
      if(p->next)
        p->next->prev = p->prev;

      (*sc->set)->prev = p;
      p->prev = NULL;
      p->next = *sc->set;
      *sc->set = p;
      return;
    }
  }

  BTB_SPEC_UPDATE_HEADER
  {
    class BTB_2levbtac_sc_t * sc = (class BTB_2levbtac_sc_t*) scvp;
    BPRED_STAT(spec_updates++;)

    *sc->bhr = ((*sc->bhr<<1)|our_pred)&history_mask;
    return;
  }

  BTB_RECOVER_HEADER
  {
    class BTB_2levbtac_sc_t * sc = (class BTB_2levbtac_sc_t*) scvp;
    if(bht_size == 1)
        bht[0] = (sc->lookup_bhr<<1)|outcome;
  }

  BTB_FLUSH_HEADER
  {
    class BTB_2levbtac_sc_t * sc = (class BTB_2levbtac_sc_t*) scvp;
    if(bht_size == 1)
      bht[0] = sc->lookup_bhr;
  }

  /* GET_CACHE */
  BTB_GET_CACHE_HEADER
  {
    class BTB_2levbtac_sc_t * sc = new BTB_2levbtac_sc_t();
    if(!sc)
      fatal("couldn't malloc 2levbtac state cache");
    return sc;
  }

};


#endif /* BTB_PARSE_ARGS */
#undef COMPONENT_NAME
