/*------------------------------------------------------------
   Based on SimpleScalar memory model (constant latency DRAM):
   Given latency in cpu cycles, or in ns which
   we'll convert to cpu cycle for you.
   Per-chunk latency computed based on cpu-speed,
   fsb-speed, and whether fsb uses DDR.

   usage:
     -dram simplescalar:160   (160 sim_cycle's per access)
     -dram simplescalar:160:0 (ditto)
     -dram simplescalar:120:1 (120 ns per access)

  ------------------------------------------------------------*/

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

#define COMPONENT_NAME "simplescalar"

#ifdef DRAM_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int latency;
  int units = 0; /* 0=cycle, 1=ns */
           
  if(sscanf(opt_string,"%*[^:]:%d:%d",&latency,&units) != 2)
  {
    if(sscanf(opt_string,"%*[^:]:%d",&latency) != 1)
      fatal("bad dram options string %s (should be \"simplescalar:latency[:units]\")",opt_string);
  }
  return new dram_simplescalar_t(latency,units);
}
#else

class dram_simplescalar_t:public dram_t
{
  protected:
  int latency;
  int chunk_latency;

  public:

  /* CREATE */
  dram_simplescalar_t(int arg_latency,
                      int arg_units)
  {
    init();

    if(arg_units)
      latency = (int)ceil(arg_latency * uncore->cpu_speed/1000.0);
    else
      latency = arg_latency;

    chunk_latency = uncore->cpu_ratio;
    best_latency = INT_MAX;
  }

  /* ACCESS */
  DRAM_ACCESS_HEADER
  {
    int chunks = (bsize + (uncore->fsb_width-1)) >> uncore->fsb_bits; /* burst length, rounded up */
    dram_assert(chunks > 0,latency);
    int lat = latency + ((chunk_latency * chunks)>>uncore->fsb_DDR);

    total_accesses++;
    total_latency += lat;
    total_burst += chunks;
    if(lat < best_latency)
      best_latency = lat;
    if(lat > worst_latency)
      worst_latency = lat;

    return lat;
  }
};

#endif /* DRAM_PARSE_ARGS */
#undef COMPONENT_NAME
