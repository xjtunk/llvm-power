/* MC-simple.c */
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

#define COMPONENT_NAME "simple"

#ifdef MC_PARSE_ARGS
if(!strcasecmp(COMPONENT_NAME,type))
{
  int RQ_size;
  int FIFO;
           
  if(sscanf(opt_string,"%*[^:]:%d:%d",&RQ_size,&FIFO) != 2)
    fatal("bad memory controller options string %s (should be \"simple:RQ-size:FIFO\")",opt_string);
  return new MC_simple_t(RQ_size,FIFO);
}
#else

/* This implements a very simple queue of memory requests.
   Requests are enqueued and dequeued in FIFO order, but the
   MC scans the requests to try to batch requests to the
   same page together to avoid frequent opening/closing of
   pages (or set the flag to force FIFO request scheduling). */
class MC_simple_t:public MC_t
{
  protected:
  int RQ_size;        /* size of request queue, i.e. maximum number of outstanding requests */
  int RQ_num;         /* current number of requests */
  int RQ_head;        /* next transaction to send back to the cpu(s) */
  int RQ_req_head;    /* next transaction to send to DRAM */
  int RQ_tail;
  struct MC_action_t * RQ;

  int FIFO_scheduling;
  md_paddr_t last_request;
  tick_t next_available_time;

  public:

  MC_simple_t(int arg_RQ_size,int arg_FIFO):
              RQ_num(0), RQ_head(0), RQ_req_head(0), RQ_tail(0), last_request(0), next_available_time(0)
  {
    init();

    RQ_size = arg_RQ_size;
    FIFO_scheduling = arg_FIFO;
    RQ = (struct MC_action_t*) calloc(RQ_size,sizeof(*RQ));
    if(!RQ)
      fatal("failed to calloc memory controller request queue");
  }

  ~MC_simple_t()
  {
    free(RQ); RQ = NULL;
  }

  MC_ENQUEUABLE_HEADER
  {
    return RQ_num < RQ_size;
  }

  /* Enqueue a memory command (read/write) to the memory controller. */
  MC_ENQUEUE_HEADER
  {
    MC_assert(RQ_num < RQ_size,(void)0);

    struct MC_action_t * req = &RQ[RQ_tail];

    MC_assert(req->valid == FALSE,(void)0);
    req->valid = TRUE;
    req->prev_cp = prev_cp;
    req->cmd = cmd;
    req->addr = addr;
    req->linesize = linesize;
    req->op = op;
    req->action_id = action_id;
    req->MSHR_bank = MSHR_bank;
    req->MSHR_index = MSHR_index;
    req->cb = cb;
    req->get_action_id = get_action_id;
    req->when_enqueued = sim_cycle;
    req->when_started = TICK_T_MAX;
    req->when_finished = TICK_T_MAX;
    req->when_returned = TICK_T_MAX;

    RQ_num++;
    RQ_tail = (RQ_tail + 1) % RQ_size;
    total_accesses++;
  }

  /* This is called each cycle to process the requests in the memory controller queue. */
  MC_STEP_HEADER
  {
    struct MC_action_t * req;

    /* MC runs at FSB speed */
    if(sim_cycle % uncore->fsb->ratio)
      return;

    if(RQ_num > 0) /* are there any requests at all? */
    {
      int i;

      /* see if there's a new request to send to DRAM */
      if(next_available_time <= sim_cycle)
      {
        int req_index = -1;
        md_paddr_t req_page = (md_paddr_t)-1;

        /* try to find a request to the same page as we're currently accessing */
        for(i=0;i<RQ_num;i++)
        {
          int idx = (RQ_head + i) % RQ_size;
          req = &RQ[idx];
          MC_assert(req->valid,(void)0);

          if(req->when_started == TICK_T_MAX)
          {
            if(req_index == -1) /* don't have a request yet */
            {
              req_index = idx;
              req_page = req->addr >> PAGE_SHIFT;
              if(FIFO_scheduling || (req_page == last_request)) /* using FIFO, or this is access to same page as current access */
                break;
            }
            else if((req->addr >> PAGE_SHIFT) == last_request) /* found an access to same page as current access */
            {
              req_index = idx;
              req_page = req->addr >> PAGE_SHIFT;
              break;
            }
          }
        }

        if(req_index != -1)
        {
          req = &RQ[req_index];
          req->when_started = sim_cycle;
          req->when_finished = sim_cycle + dram->access(req->cmd, req->addr, req->linesize);
          last_request = req_page;
          next_available_time = sim_cycle + uncore->fsb->ratio;
          total_dram_cycles += req->when_finished - req->when_started;
        }
      }

      /* walk request queue and process requests that have completed. */
      for(i=0;i<RQ_num;i++)
      {
        int idx = (RQ_head + i) % RQ_size;
        req = &RQ[idx];

        if((req->when_finished <= sim_cycle) && (req->when_returned == TICK_T_MAX) && (!req->prev_cp || bus_free(uncore->fsb)))
        {
          req->when_returned = sim_cycle;

          total_service_cycles += sim_cycle - req->when_enqueued;

          if(req->cb && req->op && (req->action_id == req->get_action_id(req->op)))
            req->cb(req->op);

          /* fill previous level as appropriate */
          if(req->prev_cp)
          {
            fill_arrived(req->prev_cp,req->MSHR_bank,req->MSHR_index);
            bus_use(uncore->fsb,req->linesize>>uncore->fsb_DDR,req->cmd==CACHE_PREFETCH);
            break; /* might as well break, since only one request can writeback per cycle */
          }
        }
      }

      /* attempt to "commit" the head */
      req = &RQ[RQ_head];
      if(req->valid)
      {
        if(req->when_returned <= sim_cycle)
        {
          req->valid = FALSE;
          RQ_num--;
          MC_assert(RQ_num >= 0,(void)0);
          RQ_head = (RQ_head + 1) % RQ_size;
        }
      }
    }
  }

  MC_PRINT_HEADER
  {
    int i;
    fprintf(stderr,"<<<<< MC >>>>>\n");
    for(i=0;i<RQ_size;i++)
    {
      fprintf(stderr,"MC[%d]: ",i);
      if(RQ[i].valid)
      {
        if(RQ[i].op)
          fprintf(stderr,"%p(%lld)",RQ[i].op,((struct uop_t*)RQ[i].op)->decode.uop_seq);
        fprintf(stderr," --> %s",RQ[i].prev_cp->name);
        fprintf(stderr," MSHR[%d][%d]",RQ[i].MSHR_bank,RQ[i].MSHR_index);
      }
      else
        fprintf(stderr,"---");
      fprintf(stderr,"\n");
    }
  }

};


#endif /* MC_PARSE_ARGS */
