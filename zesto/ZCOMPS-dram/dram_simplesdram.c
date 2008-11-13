/*------------------------------------------
  Really simple SDRAM memory model  

  This assumes a full 8GB of physical memory.
  User specifies:

  Banks - number of DRAM arrays per chip
  Ranks - number of chips (assume 1 rank per chip)
  
  We assume 4KB pages (num columns)
  We assume an external memory bus width equal to the FSB width
  Any remaining bits are used for row select.

  We assume the 33-bit physical address is decomposed as:
  
  32                                                 0
  +----------+---------+----------+---------+--------+
  | rank num | row num | bank num | col num | unused |
  +----------+---------+----------+---------+--------+

  Access is determined by:

  t_RAS = row access strobe (active to precharge delay)
  t_RCD = row command delay (active to read/write delay)
  t_CAS = column access strobe (read/write to data-out/in)
  t_WR  = write recovery time (write to precharge delay)
  t_RP  = row precharge latency (precharge to active min delay)

  We're not properly simulating data bus contention at this time
  (that requires much uglier callback, or much more sophisticated
   command sequencing/timing to ensure that conflicts don't happen)
  ------------------------------------------*/

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
#define COMPONENT_NAME "simplesdram"

#ifdef DRAM_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int ranks;
  int banks;
  double t_RAS; /* in ns */
  double t_RCD; /* in ns */
  double t_CAS; /* in ns */
  double t_WR;  /* in ns */
  double t_RP;  /* in ns */
  int refresh_period; /* in ms */

  if(sscanf(opt_string,"%*[^:]:%d:%d:%lf:%lf:%lf:%lf:%lf:%d",&ranks,&banks,&t_RAS,&t_RCD,&t_CAS,&t_WR,&t_RP,&refresh_period) != 8)
    fatal("bad dram options string %s (should be \"simplesdram:ranks:banks:t_RAS:t_RCD:t_CAS:t_WR:t_RP:refresh_period\")",opt_string);
  return new dram_simplesdram_t(ranks,banks,t_RAS,t_RCD,t_CAS,t_WR,t_RP,refresh_period);
}
#else


class dram_simplesdram_t:public dram_t
{
  protected:
  struct dram_simplesdram_bank_t
  {
    int current_row; /* row address, if any, currently held in row buffer */
    int precharged;  /* TRUE if precharged, else you need to pay for an extra t_RP */
    tick_t when_available; /* cycle when previous command completed */
    tick_t when_precharge_ready; /* cycle when precharge complete and activation can start */
  };

  counter_t row_buffer_hits;

  int num_ranks;
  int num_banks; /* per rank */
  int num_rows; /* per bank */
  int c_RAS; /* in CPU cycles - different from command line parms! */
  int c_RCD; /* in CPU cycles - different from command line parms! */
  int c_CAS; /* in CPU cycles - different from command line parms! */
  int c_WR;  /* in CPU cycles - different from command line parms! */
  int c_RP;  /* in CPU cycles - different from command line parms! */

  int c_refresh; /* timestep per *row* refresh (!= DRAM refresh time; see comments in the constructor) */
  int refresh_rank;
  int refresh_bank;
  int refresh_row;
  tick_t last_refresh;

  int rank_shift;
  int rank_mask;
  int bank_shift;
  int bank_mask;
  int row_shift;
  int row_mask;

  struct dram_simplesdram_bank_t ** array;  /* first index = rank, second = bank */

  /* assume single channel */
  tick_t when_command_bus_ready;
  tick_t when_data_bus_ready;

  /* METHOD IMPLEMENTATIONS */

  public:
  /* CREATE */
  dram_simplesdram_t(int arg_num_ranks,
                     int arg_num_banks,
                     double t_RAS,
                     double t_RCD,
                     double t_CAS,
                     double t_WR,
                     double t_RP,
                     int arg_refresh_period
                    ): refresh_rank(0), refresh_bank(0), refresh_row(0), last_refresh(0)
  {
    init();

    num_ranks = arg_num_ranks;
    num_banks = arg_num_banks;

    when_command_bus_ready = 0;
    when_data_bus_ready = 0;

    /* can't just multiply by cpu-speed - need to keep as an even
       multiple of FSB clock cycles */
    c_RAS = (int)ceil(t_RAS * uncore->fsb_speed/1000.0) * uncore->cpu_ratio;
    c_RCD = (int)ceil(t_RCD * uncore->fsb_speed/1000.0) * uncore->cpu_ratio;
    c_CAS = (int)ceil(t_CAS * uncore->fsb_speed/1000.0) * uncore->cpu_ratio;
    c_WR = (int)ceil(t_WR * uncore->fsb_speed/1000.0) * uncore->cpu_ratio;
    c_RP = (int)ceil(t_RP * uncore->fsb_speed/1000.0) * uncore->cpu_ratio;

    array = (struct dram_simplesdram_bank_t**) calloc(num_ranks,sizeof(*array));
    if(!array)
      fatal("couldn't calloc dram ranks");

    for(int i=0;i<num_ranks;i++)
    {
      array[i] = (struct dram_simplesdram_bank_t*) calloc(num_banks,(sizeof(**array)));
      if(!array[i])
        fatal("couldn't calloc dram bank[%d]",i);
    }

    int offset_bits = log_base2(uncore->fsb_width);
    int col_bits = 12-offset_bits; /* 4KB page */
    int bank_bits = log_base2(num_banks);
    int rank_bits = log_base2(num_ranks);
    int row_bits = 33 - rank_bits - bank_bits - col_bits - offset_bits;
    num_rows = 1<<row_bits;

    bank_shift = 12;
    bank_mask = (1<<bank_bits)-1;
    row_shift = bank_shift + bank_bits;
    row_mask = (1<<row_bits)-1;
    rank_shift = row_shift + row_bits;
    rank_mask = (1<<rank_bits)-1;

    /* Typical DRAM refresh times are like 32ms or 64ms.  However, if there are
       ten total rows, then a refresh command needs to be issued every 3.2 or
       6.4 milliseconds.  So we set the refresh tick equal to the refresh-period
       divided the total number of rows across all ranks/banks. */
    double refresh_tick = arg_refresh_period/((double)num_ranks*num_banks*num_rows);
    /* convert ms to clock cycles */
    c_refresh = (int)floor(refresh_tick * 1000.0 * uncore->fsb_speed) * uncore->cpu_ratio;

    best_latency = INT_MAX;
  }

  /* DESTROY */
  ~dram_simplesdram_t()
  {
    for(int i=0;i<num_ranks;i++)
    {
      free(array[i]);
      array[i] = NULL;
    }

    free(array);
    array = NULL;
  }

  /* ACCESS */
  DRAM_ACCESS_HEADER
  {
    int chunks = (bsize + (uncore->fsb_width-1)) >> uncore->fsb_bits; /* burst length, rounded up */

    int rank = (baddr>>rank_shift)&rank_mask;
    int bank = (baddr>>bank_shift)&bank_mask;
    int row = (baddr>>row_shift)&row_mask;

    tick_t when_start = MAX(when_command_bus_ready,sim_cycle);
    tick_t when_done = TICK_T_MAX;

    dram_assert(chunks > 0,when_start+c_RP+c_RAS+c_CAS-(sim_cycle));

    int data_lat = (chunks >> uncore->fsb_DDR) * uncore->cpu_ratio;
    int lat = c_CAS + data_lat;

    if(array[rank][bank].current_row == row) /* row buffer hit */
    {
      when_done = MAX(when_start + lat,when_data_bus_ready + data_lat);
      array[rank][bank].when_available = when_done;

      row_buffer_hits++;

      /* TODO?: c_WR shouldn't be paid for until row buffer written back to implement
         write combining in the row buffer... can lead to slower reads though. */
      if(cmd == CACHE_WRITE)
        array[rank][bank].when_precharge_ready = when_done + c_WR + c_RP;

    }
    else /* not in row buffer */
    {
      lat += c_RCD;

      /* can't activate page until precharged */
      when_start = MAX(when_start,array[rank][bank].when_precharge_ready);
      when_done = MAX(when_start + lat,when_data_bus_ready) + data_lat;

      array[rank][bank].when_available = when_done;
      array[rank][bank].current_row = row;
      array[rank][bank].precharged = FALSE;

      if(cmd == CACHE_READ || cmd == CACHE_PREFETCH)
        array[rank][bank].when_precharge_ready = when_start + c_RAS + c_RP;
      else if(cmd == CACHE_WRITE)
        array[rank][bank].when_precharge_ready = when_done + c_WR + c_RP;
    }

    lat = when_done - sim_cycle;

    when_data_bus_ready = when_done;
    when_command_bus_ready += uncore->cpu_ratio; /* +1 FSB cycle */

    total_accesses++;
    total_latency += lat;
    total_burst += chunks;
    if(lat < best_latency)
      best_latency = lat;
    if(lat > worst_latency)
      worst_latency = lat;

    return lat;
  }

  /* REFRESH */
  DRAM_REFRESH_HEADER
  {
    if(c_refresh <= 0)
      return;
    if((sim_cycle - last_refresh) >= c_refresh)
    {
      int rank = refresh_rank;
      int bank = refresh_bank;
      int row = refresh_row;

      /* increment counters for next row to refresh */
      refresh_rank = (refresh_rank+1) % num_ranks;
      if(refresh_rank == 0)
      {
        refresh_bank = (refresh_bank+1) % num_banks;
        if(refresh_bank == 0)
          refresh_row = (refresh_row+1) % num_rows;
      }

      tick_t when_start = MAX(when_command_bus_ready,sim_cycle);
      tick_t when_done = TICK_T_MAX;

      if(array[rank][bank].current_row == row) /* row buffer hit */
      {
        array[rank][bank].when_available = sim_cycle + c_WR;
        array[rank][bank].when_precharge_ready = when_done + c_WR + c_RP;
      }
      else /* not in row buffer - need to read and then write back */
      {
        /* can't activate page until precharged */
        when_start = MAX(sim_cycle,array[rank][bank].when_precharge_ready);
        when_done = when_start + c_CAS + c_RCD;

        array[rank][bank].when_available = when_done;
        array[rank][bank].current_row = row;
        array[rank][bank].precharged = FALSE;

        array[rank][bank].when_precharge_ready = when_done + c_WR + c_RP;
      }

      when_command_bus_ready += uncore->cpu_ratio; /* +1 FSB cycle */

      last_refresh = sim_cycle;
    }
  }

  /* STATS */
  DRAM_REG_STATS_HEADER
  {
    dram_t::reg_stats(sdb);

    stat_reg_counter(sdb, TRUE, "dram.row_buffer_hits", "total number of accesses that hit in row buffer", &row_buffer_hits, /* initial value */0, /* format */NULL);
    stat_reg_formula(sdb, TRUE, "dram.row_buffer_hit_rate","fraction of accesses that hit in row buffer",
        "dram.row_buffer_hits / dram.total_access",/* format */NULL);
  }

};



#endif /* DRAM_PARSE_ARGS */
#undef COMPONENT_NAME
