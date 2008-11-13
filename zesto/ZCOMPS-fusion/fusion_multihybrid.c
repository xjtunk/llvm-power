/*-------------------------------------------------------------------------*/
/* multihybrid selection table: two-level (four-way) tournament selection) */
/*-------------------------------------------------------------------------*/
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
#define COMPONENT_NAME "multihybrid"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp("multihybrid",type))
{
  int num_entries;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d",name,&num_entries) != 2)
    fatal("bad fusion options string %s (should be \"multihybrid:name:num_entries\")",opt_string);
  return new fusion_multihybrid_t(name,num_pred,num_entries);
}
#else

static const char meta_selection[2][2][2] =
{
  {
    {0, 0}, {1, 1}
  },
  
  {
    {2, 3}, {2, 3}
  }
};

class fusion_multihybrid_t:public fusion_t
{
  protected:
 
  int meta_size;
  int meta_mask;
  my2bc_t ** meta;

  int meta_direction;

  public:

  /* CREATE */
  fusion_multihybrid_t(char * arg_name,
                       int arg_num_pred,
                       int arg_meta_size
                      )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_meta_size);
    if(arg_num_pred != 4)
      fatal("multihybrid must have exactly four components");

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc fusion multihybrid name (strdup)");
    type = strdup("multihybrid Selection");
    if(!type)
      fatal("couldn't malloc fusion multihybrid type (strdup)");

    num_pred = arg_num_pred;
    meta_size = arg_meta_size;
    meta_mask = arg_meta_size-1;
    meta = (my2bc_t**) calloc(3,sizeof(*meta));
    if(!meta)
      fatal("couldn't malloc multihybrid meta-table");
    meta[0] = (my2bc_t*) calloc(meta_size,sizeof(my2bc_t));
    meta[1] = (my2bc_t*) calloc(meta_size,sizeof(my2bc_t));
    meta[2] = (my2bc_t*) calloc(meta_size,sizeof(my2bc_t));
    if(!meta[0] || !meta[1] || !meta[2])
      fatal("couldn't malloc multihybrid meta-table entries");

    for(int i=0;i<meta_size;i++)
    {
      meta[0][i] = 3;
      meta[1][i] = (i&1)?MY2BC_WEAKLY_NT:MY2BC_WEAKLY_TAKEN;
      meta[2][i] = (i&1)?MY2BC_WEAKLY_TAKEN:MY2BC_WEAKLY_NT;
    }
    
    bits =  meta_size*7;
  }

  /* DESTROY */
  ~fusion_multihybrid_t()
  {
    if(meta) free(meta); meta = NULL;
  }

  /* LOOKUP */
  FUSION_LOOKUP_HEADER
  {
    int index = PC&meta_mask;
    char pred;

    meta_direction = meta_selection[(meta[0][index]>>2)]
                                   [MY2BC_DIR(meta[1][index])]
                                   [MY2BC_DIR(meta[2][index])];
    pred = preds[meta_direction];

    BPRED_STAT(lookups++;)
    scvp->updated = FALSE;
    return pred;
  }

  /* UPDATE */
  FUSION_UPDATE_HEADER
  {
    int index = PC&meta_mask;

    if(!scvp->updated)
    {
      BPRED_STAT(updates++;)
      BPRED_STAT(num_hits += our_pred == outcome;)
      scvp->updated = TRUE;
    }

    /* update the top level meta-predictor when the outcomes of the two selected
       components disagree */
    if(preds[MY2BC_DIR(meta[1][index])] ^ preds[2+MY2BC_DIR(meta[2][index])])
    {
      if((preds[2+MY2BC_DIR(meta[2][index])]==outcome))
      {
        if(meta[0][index] < 7)
          ++ meta[0][index];
      }
      else
      {
        if(meta[0][index] > 0)
          -- meta[0][index];
      }
    }

    /* meta table for each set of two components only updated when
       those predictors disagree. If both agree, then either both
       are right, or both are wrong: in either case the meta table
       is not updated. */
    if(preds[0]^preds[1])
    {
      MY2BC_UPDATE(meta[1][index],(preds[1]==outcome));
    }
    if(preds[2]^preds[3])
    {
      MY2BC_UPDATE(meta[2][index],(preds[3]==outcome));
    }
  }
};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
