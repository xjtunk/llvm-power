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

/*-----------------------------*/
/* Path-based Neural Predictor */
/* [Jimenez, MICRO'2003]       */
/*-----------------------------*/
#include <math.h>
#define COMPONENT_NAME "pathneural"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int l1_size;
  int his_width;
  int top_size;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d:%d:%d",name,&l1_size,&his_width,&top_size) != 4)
    fatal("bad bpred options string %s (should be \"pathneural:name:l1_size:his_width:top_size\")",opt_string);
  return new bpred_pathneural_t(name,l1_size,his_width,top_size);
}
#else

class bpred_pathneural_t:public bpred_dir_t
{
#define BIPOLAR(x) (((x)<<1)-1)
//#define BIPOLAR(x) (2*(x)-1)

#define MAX_PATHNEURAL_PATH 128

  class bpred_pathneural_sc_t:public bpred_sc_t
  {
    public:
    qword_t * bhr;
    qword_t lookup_bhr;
    md_addr_t lookup_path[MAX_PATHNEURAL_PATH];
    md_addr_t lookup_PC;
    int sum;
  };

  protected:

  int bht_size;
  int bht_mask;
  qword_t * bht;
  int top_size;
  int top_mask;
  short **top; /* table of pathneurals */
  int weight_width;
  int weight_max;
  int weight_min;
  int theta;

  md_addr_t path[MAX_PATHNEURAL_PATH];
  int path_head;

  counter_t weights_read;
  counter_t weights_written;

  int history_length;
  qword_t history_mask;

  public:

  /* CREATE */
  bpred_pathneural_t(char * arg_name,
                     int arg_bht_size,
                     int arg_history_length,
                     int arg_top_size
                    )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_bht_size);
    CHECK_NNEG(arg_history_length);

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc pathneural name (strdup)");
    type = strdup(COMPONENT_NAME);
    if(!type)
      fatal("couldn't malloc pathneural type (strdup)");

    bht_size = arg_bht_size;
    bht_mask = arg_bht_size-1;
    history_length = arg_history_length;
    history_mask = (((qword_t)1)<<arg_history_length)-1;

    top_size = arg_top_size;
    top_mask = (1<<(int)(rint(ceil(log(arg_top_size)/log(2.0)))))-1;

    /* see Dan Jimenez's HPCA paper for these magic formulae */
    theta = (int) rint(floor(1.93*arg_history_length + 14.0));

    /* if power of two, then actually need an extra bit to represent.
       also add one bit for the sign. */
    //if(theta == (1<<mylog2(theta)))
      //weight_width = mylog2(theta)+2;
    //else
      //weight_width = mylog2(theta)+1;
    weight_width = 8;

    weight_max = (1<<(weight_width-1))-1;
    weight_min = -(1<<(weight_width-1));

    bht = (qword_t*) calloc(bht_size,sizeof(*bht));
    if(!bht)
      fatal("couldn't malloc pathneural BHT");
    top = (short**) calloc(top_size,sizeof(*top));
    if(!top)
      fatal("couldn't malloc pathneural ToP");
    for(int i=0;i<top_size;i++)
    {
      top[i] = (short*) calloc(1+history_length,sizeof(*top[i]));
      if(!top[i])
        fatal("couldn't malloc pathneural ToP entry");
    }

    memset(path,0,sizeof(*path)*MAX_PATHNEURAL_PATH);
    path_head = 0;

    weights_read = 0;
    weights_written = 0;

    bits =  bht_size*history_length
         + top_size*(history_length+1)*weight_width
         + history_length*log_base2(top_size);
  }

  /* DESTROY */
  ~bpred_pathneural_t()
  {
    for(int i=0;i<top_size;i++)
    {
      free(top[i]); top[i] = NULL;
    }
    free(top); top = NULL;
    free(bht); bht = NULL;
  }

  BPRED_LOOKUP_HEADER
  {
    class bpred_pathneural_sc_t * sc = (class bpred_pathneural_sc_t*) scvp;
    int l1index = PC&bht_mask;
    char pred;
    int i;

    sc->bhr = &bht[l1index];
    sc->sum = top[(PC&top_mask)%top_size][0];
    for(i=0;i<history_length;i++)
    {
      int hist_dir = ((*sc->bhr >> i) & 1) ? 1:-1;
      md_addr_t addr = path[(path_head - i + MAX_PATHNEURAL_PATH) % MAX_PATHNEURAL_PATH];
      sc->sum += hist_dir * top[(addr&top_mask)%top_size][i+1];
    }

    pred = (sc->sum >= 0);
    sc->lookup_PC = PC;
    sc->lookup_bhr = *sc->bhr;
    for(i=0;i<history_length;i++)
    {
      sc->lookup_path[history_length-i-1] = path[(path_head - i + MAX_PATHNEURAL_PATH)%MAX_PATHNEURAL_PATH];
    }

    weights_read += 1 + history_length;

    BPRED_STAT(lookups++;)
    sc->updated = FALSE;

    return pred;
  }

  /* UPDATE */
  BPRED_UPDATE_HEADER
  {
    class bpred_pathneural_sc_t * sc = (class bpred_pathneural_sc_t*) scvp;

    if(!sc->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)
      sc->updated = TRUE;
    }

    if(((sc->sum >= 0) != outcome) || ((sc->sum > -theta) && (sc->sum <theta)))
    {
      short * weight;
      weight = &top[(PC&top_mask)%top_size][0];
      if(outcome) {
          if(*weight < weight_max) ++*weight;
      } else {
          if(*weight > weight_min) --*weight;
      }

      for(int i=0;i<history_length;i++)
      {
        md_addr_t addr = sc->lookup_path[history_length-1-i];
        weight = &top[(addr&top_mask)%top_size][i+1];
        if( ((sc->lookup_bhr>>i)&1) == (unsigned)outcome ) {
            if(*weight < weight_max) ++*weight;
        } else {
            if(*weight > weight_min) --*weight;
        }
      }
      weights_written += 1 + history_length;
    }

  }

  /* SPEC_UPDATE */
  BPRED_SPEC_UPDATE_HEADER
  {
    class bpred_pathneural_sc_t * sc = (class bpred_pathneural_sc_t*) scvp;
    BPRED_STAT(spec_updates++;)

    *sc->bhr = ((*sc->bhr<<1)|outcome)&history_mask;
    path_head = (path_head + 1) % MAX_PATHNEURAL_PATH;
    path[path_head] = PC;
  }

  /* RECOVER */
  BPRED_RECOVER_HEADER
  {
    class bpred_pathneural_sc_t * sc = (class bpred_pathneural_sc_t*) scvp;
    *sc->bhr = (sc->lookup_bhr<<1)|outcome;
    for(int i=0;i<history_length;i++)
        path[i] = sc->lookup_path[i];
    path_head = history_length-1;

    path_head = (path_head + 1) % MAX_PATHNEURAL_PATH;
    path[path_head] = sc->lookup_PC;
  }

  /* FLUSH */
  BPRED_FLUSH_HEADER
  {
    class bpred_pathneural_sc_t * sc = (class bpred_pathneural_sc_t*) scvp;
    *sc->bhr = sc->lookup_bhr;
    for(int i=0;i<history_length;i++)
      path[i] = sc->lookup_path[i];
    path_head = history_length-1;
  }

  /* REG_STATS */
  BPRED_REG_STATS_HEADER
  {
    bpred_dir_t::reg_stats(sdb,core);

    int id = core->current_thread->id;
    char buf[256];
    char buf2[256];

    sprintf(buf,"c%d.%s.threshold",id,name);
    sprintf(buf2,"%s training threshold",type);
    stat_reg_int(sdb, TRUE, buf, buf2, &theta, theta, NULL);
    sprintf(buf,"c%d.%s.weight_width",id,name);
    sprintf(buf2,"%s weight/counter width in bits",type);
    stat_reg_int(sdb, TRUE, buf, buf2, &weight_width, weight_width, NULL);
    sprintf(buf,"c%d.%s.weights_read",id,name);
    sprintf(buf2,"total number of weights read by %s",name);
    stat_reg_counter(sdb, TRUE, buf, buf2, &weights_read, weights_read, NULL);
    sprintf(buf,"c%d.%s.weights_written",id,name);
    sprintf(buf2,"total number of weights written by %s",name);
    stat_reg_counter(sdb, TRUE, buf, buf2, &weights_written, weights_written, NULL);
  }

  /* GET_CACHE */
  BPRED_GET_CACHE_HEADER
  {
    class bpred_pathneural_sc_t * sc = new bpred_pathneural_sc_t();
    if(!sc)
      fatal("couldn't malloc pathneural State Cache");
    return sc;
  }

};

#undef BIPOLAR
#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
