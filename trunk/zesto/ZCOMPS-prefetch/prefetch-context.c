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
#define COMPONENT_NAME "context"

/* context-based prefetcher.  Similar to markov/correlating prefetcher. */

#ifdef PREFETCH_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int num_entries;
  int tag_bits;
  int next_bits;
  int conf_bits;
  int u_bits;
  if(sscanf(opt_string,"%*[^:]:%d:%d:%d:%d:%d",&num_entries,&tag_bits,&next_bits,&conf_bits,&u_bits) != 5)
    fatal("bad %s prefetcher options string %s (should be \"context:num_entries:tag-bits:next-bits:conf-bits:u-bits\")",cp->name,opt_string);
  return new prefetch_context_t(cp,num_entries,tag_bits,next_bits,conf_bits,u_bits);
}
#else

class prefetch_context_t:public prefetch_t
{
  protected:
  int num_entries;
  int mask;

  int tag_bits;
  int next_bits;
  int conf_bits;
  int u_bits;

  md_paddr_t tag_mask;
  md_paddr_t next_mask;
  int conf_max;
  int u_max;

  struct prefetch_context_table_t {
    md_paddr_t tag;
    md_paddr_t next_paddr;
    int conf;
    int u;
  } * table;

  md_paddr_t last_paddr;

  public:

  /* CREATE */
  prefetch_context_t(struct cache_t * arg_cp,
                    int arg_num_entries,
                    int arg_tag_bits,
                    int arg_next_bits,
                    int arg_conf_bits,
                    int arg_u_bits)
  {
    init();

    last_paddr = 0;

    char name[256]; /* just used for error checking macros */
    type = strdup("context");
    if(!type)
      fatal("couldn't calloc context-prefetch type (strdup)");

    cp = arg_cp;
    sprintf(name,"%s.%s",cp->name,type);

    CHECK_PPOW2(arg_num_entries);
    num_entries = arg_num_entries;
    mask = num_entries - 1;

    tag_bits = arg_tag_bits;
    next_bits = arg_next_bits;
    conf_bits = arg_conf_bits;
    u_bits = arg_u_bits;

    tag_mask = ((1<<tag_bits)-1) << log_base2(num_entries);
    next_mask = (1<<next_bits)-1;
    conf_max = (1<<conf_bits)-1;
    u_max = (1<<u_bits)-1;

    table = (struct prefetch_context_t::prefetch_context_table_t*) calloc(num_entries,sizeof(*table));
    if(!table)
      fatal("couldn't calloc context-prefetch table");

    bits = num_entries * (tag_bits + next_bits + conf_bits + u_bits);
  }

  /* DESTROY */
  ~prefetch_context_t()
  {
    free(table);
    table = NULL;
  }

  /* LOOKUP */
  PREFETCH_LOOKUP_HEADER
  {
    lookups++;

    int index = paddr & mask;
    md_paddr_t pf_paddr = 0;

    /* do current lookup first */
    if((paddr & tag_mask) == table[index].tag)
    {
      /* hit: check if confident */
      if(table[index].conf >= conf_max)
        pf_paddr = (paddr&~next_mask) | (table[index].conf&next_mask);
    }

    /* train */
    index = last_paddr & mask;

    /* is it a hit? */
    if((last_paddr&tag_mask) == table[index].tag) /* hit */
    {
      /* each hit increments the useful counter */
      if(table[index].u < u_max)
        table[index].u++;

      /* increase confidence if we see the same "next" address */
      if((paddr & next_mask) == table[index].next_paddr)
      {
        if(table[index].conf < conf_max)
          table[index].conf ++;
      }
      else
      {
        table[index].next_paddr = paddr & next_mask;
        table[index].conf = 0;
        if(table[index].u > 0) /* also decrease usefulness */
          table[index].u --;
      }
    }
    else /* miss */
    {
      if(table[index].u > 0)
      {
        table[index].u --;
      }
      else /* this entry hasn't been useful lately, replace it */
      {
        table[index].u = u_max;
        table[index].conf = 0;
        table[index].tag = last_paddr & tag_mask;
        table[index].next_paddr = paddr;
      }
    }

    last_paddr = paddr;

    return pf_paddr;
  }
};



#endif /* PREFETCH_PARSE_ARGS */
#undef COMPONENT_NAME
