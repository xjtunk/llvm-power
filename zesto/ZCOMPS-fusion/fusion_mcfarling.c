/*---------------------------------------*/
/* McFarling selection table [mcfarling] */
/*---------------------------------------*/
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
#define COMPONENT_NAME "mcfarling"

#ifdef BPRED_PARSE_ARGS
if(!strcasecmp("mcfarling",type))
{
  int num_entries;

  if(sscanf(opt_string,"%*[^:]:%[^:]:%d",name,&num_entries) != 2)
    fatal("bad fusion options string %s (should be \"mcfarling:name:num_entries\")",opt_string);
  return new fusion_mcfarling_t(name,num_pred,num_entries);
}
#else

class fusion_mcfarling_t:public fusion_t
{
  protected:
  int meta_size;
  int meta_mask;
  my2bc_t * meta;

  public:

  /* CREATE */
  fusion_mcfarling_t(char * arg_name,
                     int arg_num_pred,
                     int arg_meta_size
                    )
  {
    init();

    /* verify arguments are valid */
    CHECK_PPOW2(arg_meta_size);
    if(arg_num_pred != 2)
      fatal("mcfarling fusion only valid for two component/direction predictors");

    name = strdup(arg_name);
    if(!name)
      fatal("couldn't malloc fusion mcfarling name (strdup)");
    type = strdup("McFarling tournament selection");
    if(!type)
      fatal("couldn't malloc fusion mcfarling type (strdup)");

    num_pred = arg_num_pred;
    meta_size = arg_meta_size;
    meta_mask = meta_size-1;
    meta = (my2bc_t*) calloc(meta_size,sizeof(my2bc_t));
    if(!meta)
      fatal("couldn't malloc mcfarling meta-table");
    bits =  meta_size*2;
  }

  /* DESTROY */
  ~fusion_mcfarling_t()
  {
    free(meta); meta = NULL;
  }

  /* LOOKUP */
  FUSION_LOOKUP_HEADER
  {
    int index = PC&meta_mask;
    int meta_direction = MY2BC_DIR(meta[index]);
    char pred = preds[meta_direction];
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

    /* meta table only updated when predictors disagree. If both
       agree, then either both are right, or both are wrong: in
       either case the meta table is not updated. */
    if(preds[0]^preds[1])
    {
      MY2BC_UPDATE(meta[index],(preds[1]==outcome));
    }
  }

};

#endif /* BPRED_PARSE_ARGS */
#undef COMPONENT_NAME
