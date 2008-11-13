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
#define COMPONENT_NAME "IP"

/* IP-based (PC) prefetcher.  See http://download.intel.com/technology/architecture/sma.pdf */

#ifdef PREFETCH_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int num_entries;
  int last_addr_bits;
  int last_stride_bits;
  int last_prefetch_bits;
  if(sscanf(opt_string,"%*[^:]:%d:%d:%d:%d",&num_entries,&last_addr_bits,&last_stride_bits,&last_prefetch_bits) != 4)
    fatal("bad %s prefetcher options string %s (should be \"IP:num_entries:addr-bits:stride-bits:last-PF-bits\")",cp->name,opt_string);
  return new prefetch_IP_t(cp,num_entries,last_addr_bits,last_stride_bits,last_prefetch_bits);
}
#else

class prefetch_IP_t:public prefetch_t
{
  protected:
  int num_entries;
  int mask;

  int last_addr_bits;
  int last_stride_bits;
  int last_prefetch_bits;

  int last_addr_mask;
  int last_stride_mask;
  int last_prefetch_mask;

  struct prefetch_IP_table_t {
    md_paddr_t last_paddr;
    int last_stride;
    my2bc_t conf;
    md_paddr_t last_prefetch;
  } * table;

  public:
  /* CREATE */
  prefetch_IP_t(struct cache_t * arg_cp,
                int arg_num_entries,
                int arg_last_addr_bits,
                int arg_last_stride_bits,
                int arg_last_prefetch_bits)
  {
    init();

    char name[256]; /* just used for error checking macros */

    type = strdup("IP");
    if(!type)
      fatal("couldn't calloc IP-prefetch type (strdup)");

    cp = arg_cp;
    sprintf(name,"%s.%s",cp->name,type);

    CHECK_PPOW2(arg_num_entries);
    num_entries = arg_num_entries;
    mask = num_entries - 1;

    last_addr_bits = arg_last_addr_bits;
    last_stride_bits = arg_last_stride_bits;
    last_prefetch_bits = arg_last_prefetch_bits;

    last_addr_mask = (1<<last_addr_bits)-1;
    last_stride_mask = (1<<last_stride_bits)-1;
    last_prefetch_mask = (1<<last_prefetch_bits)-1;

    table = (struct prefetch_IP_t::prefetch_IP_table_t*) calloc(num_entries,sizeof(*table));
    if(!table)
      fatal("couldn't calloc IP-prefetch table");

    bits = num_entries * (last_addr_bits + last_stride_bits + last_prefetch_bits + 2);
    assert(arg_cp);
  }

  /* DESTROY */
  ~prefetch_IP_t()
  {
    free(table);
    table = NULL;
  }

  /* LOOKUP */
  PREFETCH_LOOKUP_HEADER
  {
    lookups++;

    int index = PC & mask;
    md_paddr_t pf_paddr = 0;

    /* train entry first */
    md_paddr_t last_paddr = (paddr&~last_addr_mask) | table[index].last_paddr;
    int this_stride = (paddr - last_paddr) & last_stride_mask;

    if(this_stride == table[index].last_stride)
      MY2BC_UPDATE(table[index].conf,TRUE);
    else
      table[index].conf = MY2BC_STRONG_NT;

    table[index].last_paddr = paddr&last_addr_mask;
    table[index].last_stride = this_stride;

    /* only make a prefetch if confident */
    if(table[index].conf == MY2BC_STRONG_TAKEN)
    {
      pf_paddr = paddr + this_stride;
      if((pf_paddr & last_prefetch_mask) == table[index].last_prefetch)
        pf_paddr = 0; /* don't keep prefetching the same thing */
      table[index].last_prefetch = pf_paddr & last_prefetch_mask;
    }

    return pf_paddr;
  }
};


#endif /* PREFETCH_PARSE_ARGS */
#undef COMPONENT_NAME
