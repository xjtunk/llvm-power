#ifndef ZESTO_ORACLE_INCLUDED
#define ZESTO_ORACLE_INCLUDED

/* sim-oracle.h */
/*
 * Copyright (C) 2007 by Gabriel H. Loh and the Georgia Tech
 * Research Corporation Atlanta, GA  30332-0415 All Rights
 * Reserved.
 * 
 * THIS IS A LEGAL DOCUMENT BY DOWNLOADING ZESTO, YOU ARE AGREEING
 * TO THESE TERMS AND CONDITIONS.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * NOTE: Portions of this release are directly derived from the
 * SimpleScalar Toolset (property of SimpleScalar LLC), and as
 * such, those portions are bound by the corresponding legal terms
 * and conditions.  All source files derived directly or in part
 * from the SimpleScalar Toolset bear the original user agreement.
 * 
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 
 * 1. Redistributions of source code must retain the above
 * copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 * 
 * 3. Neither the name of the Georgia Tech Research Coporation nor
 * the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior
 * written permission.
 * 
 * 4. Zesto is distributed freely for commercial and non-commercial
 * use.  Note, however, that the portions derived from the
 * SimpleScalar Toolset are bound by the terms and agreements set
 * forth by SimpleScalar, LLC.  In particular:
 * 
 *   "Nonprofit and noncommercial use is encouraged. SimpleScalar
 *   may be downloaded, compiled, executed, copied, and modified
 *   solely for nonprofit, educational, noncommercial research, and
 *   noncommercial scholarship purposes provided that this notice
 *   in its entirety accompanies all copies.  Copies of the
 *   modified software can be delivered to persons who use it
 *   solely for nonprofit, educational, noncommercial research, and
 *   noncommercial scholarship purposes provided that this notice
 *   in its entirety accompanies all copies."
 * 
 * User is responsible for reading and adhering to the terms set
 * forth by SimpleScalar, LLC where appropriate.
 * 
 * 5. No nonprofit user may place any restrictions on the use of
 * this software, including as modified by the user, by any other
 * authorized user.
 * 
 * 6. Noncommercial and nonprofit users may distribute copies of
 * Zesto in compiled or executable form as set forth in Section 2,
 * provided that either: (A) it is accompanied by the corresponding
 * machine-readable source code, or (B) it is accompanied by a
 * written offer, with no time limit, to give anyone a
 * machine-readable copy of the corresponding source code in return
 * for reimbursement of the cost of distribution. This written
 * offer must permit verbatim duplication by anyone, or (C) it is
 * distributed by someone who received only the executable form,
 * and is accompanied by a copy of the written offer of source
 * code.
 * 
 * 7. Zesto was developed by Gabriel H. Loh, Ph.D.  US Mail: 266
 * Ferst Drive, Georgia Institute of Technology, Atlanta, GA
 * 30332-0765
 * 
 */

#ifdef DEBUG
#define zesto_fatal(msg, retval) fatal(msg)
#else
#define zesto_fatal(msg, retval) { \
  core->oracle->hosed = TRUE; \
  fprintf(stderr,"fatal (%s,%d:thread %d): ",__FILE__,__LINE__,core->current_thread->id); \
  fprintf(stderr,"%s\n",msg); \
  return (retval); \
}
#endif

#ifdef DEBUG
#define zesto_assert(cond, retval) assert(cond)
#else
#define zesto_assert(cond, retval) { \
  if(!(cond)) { \
    core->oracle->hosed = TRUE; \
    fprintf(stderr,"assertion failed (%s,%d:thread %d): ",__FILE__,__LINE__,core->current_thread->id); \
    fprintf(stderr,"%s\n",#cond); \
    return (retval); \
  } \
}
#endif

class core_oracle_t {

  /* struct for tracking all in-flight writers of registers */
  struct map_node_t {
    struct uop_t * uop;
    struct map_node_t * prev;
    struct map_node_t * next;
  };

  /* structs for tracking pre-commit memory writes */
#define MEM_HASH_SIZE 32768
#define MEM_HASH_MASK (MEM_HASH_SIZE-1)

  struct spec_mem_t {
    struct {
      struct spec_byte_t * head;
      struct spec_byte_t * tail;
    } hash[MEM_HASH_SIZE];
  };

  public:

  bool spec_mode;  /* are we currently on a wrong-path? */

  bool hosed; /* set to TRUE when something in the architected state (core->arch_state) has been seriously
                corrupted. */

  core_oracle_t(struct core_t * core);
  void reg_stats(struct stat_sdb_t *sdb);
  void update_occupancy(void);

  struct Mop_t * get_Mop(int index);
  int get_index(struct Mop_t * Mop);
  int next_index(int index);

  bool spec_read_byte(md_addr_t addr, byte_t * valp);
  struct spec_byte_t * spec_write_byte(md_addr_t addr, byte_t val);

  struct Mop_t * exec(md_addr_t requested_PC);
  void consume(struct Mop_t * Mop);
  void commit_uop(struct uop_t * uop);
  void commit(struct Mop_t * commit_Mop);

  void recover(struct Mop_t * Mop); /* recovers oracle state */
  void pipe_recover(struct Mop_t * Mop, md_addr_t New_PC); /* unrolls pipeline state */
  void pipe_flush(struct Mop_t * Mop);

  void complete_flush(void);
  void reset_execution(void);

  protected:

  /* static members shared by all cores */

  static bool static_members_initialized;

  struct Mop_t * MopQ;
  int MopQ_head;
  int MopQ_tail;
  int MopQ_num;
  int MopQ_size;
  struct Mop_t * current_Mop; /* pointer to track a Mop that has been executed but not consumed (i.e. due to fetch stall) */

  static struct map_node_t * map_free_pool;  /* for decode.dep_map */
  static int map_free_pool_debt;
  static struct spec_byte_t * spec_mem_free_pool; /* for oracle spec-memory map */
  static int spec_mem_pool_debt;

  struct core_t * core;
  struct spec_mem_t spec_mem_map;
  /* dependency tracking used by oracle */
  struct {
    struct map_node_t * head[MD_TOTAL_REGS];
    struct map_node_t * tail[MD_TOTAL_REGS];
  } dep_map;

  void undo(struct Mop_t * Mop);

  void install_mapping(struct uop_t * uop);
  void commit_mapping(struct uop_t * uop);
  void undo_mapping(struct uop_t * uop);
  struct map_node_t * get_map_node(void);
  void return_map_node(struct map_node_t * p);

  void install_dependencies(struct uop_t * uop);
  void commit_dependencies(struct uop_t * uop);
  void undo_dependencies(struct uop_t * uop);

  struct spec_byte_t * get_spec_mem_node(void);
  void return_spec_mem_node(struct spec_byte_t * p);

  void commit_write_byte(struct spec_byte_t * p);
  void squash_write_byte(struct spec_byte_t * p);

  void cleanup_aborted_mop(struct Mop_t * Mop);
};









#endif /* ZESTO_ORACLE_INCLUDED */
