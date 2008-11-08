//
// PTLsim: Cycle Accurate x86-64 Simulator
// Out-of-Order Core Simulator
// Execution Pipeline Stages: Scheduling, Execution, Broadcast
//
// Copyright 2003-2008 Matt T. Yourst <yourst@yourst.com>
// Copyright 2006-2008 Hui Zeng <hzeng@cs.binghamton.edu>
//

#include <globals.h>
#include <elf.h>
#include <ptlsim.h>
#include <branchpred.h>
#include <datastore.h>
#include <logic.h>
#include <dcache.h>

#define INSIDE_OOOCORE
#include <ooocore.h>
#include <stats.h>

#ifndef ENABLE_CHECKS
#undef assert
#define assert(x) (x)
#endif

#ifndef ENABLE_LOGGING
#undef logable
#define logable(level) (0)
#endif

using namespace OutOfOrderModel;

//
// Issue Queue
//
template <int size, int operandcount>
void IssueQueue<size, operandcount>::reset(int coreid) {
  OutOfOrderCore& core = getcore();

  this->coreid = coreid;
  count = 0;
  valid = 0;
  issued = 0;
  allready = 0;
  foreach (i, operandcount) {
    tags[i].reset();
  }
  uopids.reset();

  foreach (i, core.threadcount) {
    ThreadContext* thread = core.threads[i];
    if unlikely (!thread) continue;
    thread->issueq_count = 0;
  }
}

template <int size, int operandcount>
void IssueQueue<size, operandcount>::reset(int coreid, int threadid) {
  OutOfOrderCore& core = getcore();

  if unlikely (core.threadcount == 1) {
    reset(coreid);
    return;
  }
#ifndef MULTI_IQ
  this->coreid = coreid;

  foreach (i, size) {
    if likely (valid[i]) { 
      int slot_threadid = uopids[i] >> MAX_ROB_IDX_BIT;
      if likely (slot_threadid == threadid) {
        remove(i);
        // Now i+1 is moved to position of where i used to be:
        i--;
  
        ThreadContext* thread = core.threads[threadid];
        if (thread->issueq_count > core.reserved_iq_entries) {
          issueq_operation_on_cluster(core, cluster, free_shared_entry());
        }
        thread->issueq_count--;
      }
    }
  }
#endif
}

template <int size, int operandcount>
void IssueQueue<size, operandcount>::clock() {
  allready = (valid & (~issued));
  foreach (operand, operandcount) {
    allready &= ~tags[operand].valid;
  }
}

template <int size, int operandcount>
bool IssueQueue<size, operandcount>::insert(tag_t uopid, const tag_t* operands, const tag_t* preready) {
  if unlikely (count == size)
                return false;

  assert(count < size);
  
  int slot = count++;

  assert(!bit(valid, slot));
  
  uopids.insertslot(slot, uopid);
  
  valid[slot] = 1;
  issued[slot] = 0;
  
  foreach (operand, operandcount) {
    if likely (preready[operand])
      tags[operand].invalidateslot(slot);
    else tags[operand].insertslot(slot, operands[operand]);
  }
  
  return true;
}

template <int size, int operandcount>
void IssueQueue<size, operandcount>::tally_broadcast_matches(IssueQueue<size, operandcount>::tag_t sourceid, const bitvec<size>& mask, int operand) const {
  if likely (!config.event_log_enabled) return;

  OutOfOrderCore& core = getcore();
  int threadid, rob_idx;
  decode_tag(sourceid, threadid, rob_idx);
  ThreadContext* thread = core.threads[threadid];
  const ReorderBufferEntry* source = &thread->ROB[rob_idx];

  bitvec<size> temp = mask;

  while (*temp) {
    int slot = temp.lsb();
    int robid = uopof(slot);

    int threadid_tmp, rob_idx_tmp;
    decode_tag(robid, threadid_tmp, rob_idx_tmp);
    assert(threadid_tmp == threadid);
    assert(inrange(rob_idx_tmp, 0, ROB_SIZE-1));
    const ReorderBufferEntry* target = &thread->ROB[rob_idx_tmp];

    temp[slot] = 0;

    OutOfOrderCoreEvent* event = core.eventlog.add(EVENT_FORWARD, source);
    event->forwarding.operand = operand;
    event->forwarding.forward_cycle = source->forward_cycle;
    event->forwarding.target_uuid = target->uop.uuid;
    event->forwarding.target_rob = target->index();
    event->forwarding.target_physreg = target->physreg->index();
    event->forwarding.target_rfid = target->physreg->rfid;
    event->forwarding.target_cluster = target->cluster;
    bool target_st = isstore(target->uop.opcode);
    event->forwarding.target_st = target_st;
    if (target_st) event->forwarding.target_lsq = target->lsq->index();
    event->forwarding.target_operands_ready = 0;
    foreach (i, MAX_OPERANDS) event->forwarding.target_operands_ready |= ((target->operands[i]->ready()) << i);
    event->forwarding.target_all_operands_ready = target->ready_to_issue();
  }
}

template <int size, int operandcount>
bool IssueQueue<size, operandcount>::broadcast(tag_t uopid) {
  vec_t tagvec = assoc_t::prep(uopid);
  
  if (logable(6)) {
    foreach (operand, operandcount) {
      bitvec<size> mask = tags[operand].invalidate(tagvec);
      if unlikely (config.event_log_enabled) tally_broadcast_matches(uopid, mask, operand);
    }
  } else {
    foreach (operand, operandcount) tags[operand].invalidate(tagvec);
  }
  return true;
}

//
// Select one ready slot and move it to the issued state.
// This function returns the slot id. The returned slot
// id becomes invalid after the next call to remove()
// before the next uop can be processed in any way.
//
template <int size, int operandcount>
int IssueQueue<size, operandcount>::issue() {
  if (!allready) return -1;
  int slot = allready.lsb();
  issued[slot] = 1;
  return slot;
}

//
// Replay a uop that has already issued once.
// The caller may add or reset dependencies here as needed.
//
template <int size, int operandcount>
bool IssueQueue<size, operandcount>::replay(int slot, const tag_t* operands, const tag_t* preready) {
  assert(valid[slot]);
  assert(issued[slot]);
  
  issued[slot] = 0;
  
  foreach (operand, operandcount) {
    if (preready[operand])
      tags[operand].invalidateslot(slot);
    else tags[operand].insertslot(slot, operands[operand]);
  }
  
  return true;
}

//
// Move a given slot to the end of the issue queue, so it will issue last.
// This is used in SMT to resolve deadlocks where the release part of an
// interlocked load or store cannot dispatch because some other interlocked
// uops are already blocking the issue queue. This guarantees one or the
// other will complete, avoiding the deadlock.
//
template <int size, int operandcount>
bool IssueQueue<size, operandcount>::switch_to_end(int slot, const tag_t* operands, const tag_t* preready) {
  tag_t uopid = uopids[slot];
  // remove
  remove(slot);
  // insert at end:
  insert(uopid, operands, preready);
  return true;
}

// NOTE: This is a fairly expensive operation:
template <int size, int operandcount>
bool IssueQueue<size, operandcount>::remove(int slot) {
  uopids.collapse(slot);

  foreach (i, operandcount) {
    tags[i].collapse(slot);
  }
  
  valid = valid.remove(slot, 1);
  issued = issued.remove(slot, 1);
  allready = allready.remove(slot, 1);
  
  count--;
  assert(count >= 0);
  return true;
}

template <int size, int operandcount>
ostream& IssueQueue<size, operandcount>::print(ostream& os) const {
  os << "IssueQueue: count = ", count, ":", endl;
  foreach (i, size) {
    os << "  uop ";
    uopids.printid(os, i);
    os << ": ",
      ((valid[i]) ? 'V' : '-'), ' ',
      ((issued[i]) ? 'I' : '-'), ' ',
      ((allready[i]) ? 'R' : '-'), ' ';
    foreach (j, operandcount) {
      if (j) os << ' ';
      tags[j].printid(os, i);
    }
    os << endl;
  }
  return os;
}

// Instantiate all methods in the specific IssueQueue sizes we're using:
declare_issueq_templates;

//
// Issue a single ROB. 
//
// Returns:
//  +1 if issue was successful
//   0 if no functional unit was available
//  -1 if there was an exception and we should stop issuing this cycle
//
int ReorderBufferEntry::issue() {
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();
  OutOfOrderCoreEvent* event = null;

  W32 executable_on_fu = fuinfo[uop.opcode].fu & clusters[cluster].fu_mask & core.fu_avail;

  // Are any FUs available in this cycle?
  if unlikely (!executable_on_fu) {
    if unlikely (config.event_log_enabled) {
      event = core.eventlog.add(EVENT_ISSUE_NO_FU, this);
      event->issue.fu_avail = core.fu_avail;
    }

    per_context_ooocore_stats_update(threadid, issue.result.no_fu++);
    //
    // When this (very rarely) happens, stop issuing uops to this cluster
    // and try again with the problem uop on the next cycle. In practice
    // this scenario rarely happens.
    //
    issueq_operation_on_cluster(core, cluster, replay(iqslot));
    return ISSUE_NEEDS_REPLAY;
  }

  PhysicalRegister& ra = *operands[RA];
  PhysicalRegister& rb = *operands[RB];
  PhysicalRegister& rc = *operands[RC];

  //
  // Check if any other resources are missing that we didn't
  // know about earlier, and replay like we did above if
  // needed. This is our last chance to do so.
  //

  stats.summary.uops++;
  per_context_ooocore_stats_update(threadid, issue.uops++);

  fu = lsbindex(executable_on_fu);
  clearbit(core.fu_avail, fu);
  core.robs_on_fu[fu] = this;
  cycles_left = fuinfo[uop.opcode].latency;
  changestate(thread.rob_issued_list[cluster]);

  IssueState state;
  state.reg.rdflags = 0;

  W64 radata = ra.data;
  W64 rbdata = (uop.rb == REG_imm) ? uop.rbimm : rb.data;
  W64 rcdata = (uop.rc == REG_imm) ? uop.rcimm : rc.data;

  bool ld = isload(uop.opcode);
  bool st = isstore(uop.opcode);
  bool br = isbranch(uop.opcode);

  assert(operands[RA]->ready());
  assert(rb.ready());
  if likely ((!st || (st && load_store_second_phase)) && (uop.rc != REG_imm)) assert(rc.ready());
  if likely (!st) assert(operands[RS]->ready());

  if likely (ra.nonnull()) {
    ra.get_state_list().issue_source_counter++;
    ra.all_consumers_sourced_from_bypass &= (ra.state == PHYSREG_BYPASS);
    per_physregfile_stats_update(stats.ooocore.issue.source, ra.rfid, [ra.state]++);
  }

  if likely ((!uop.rbimm) & (rb.nonnull())) { 
    rb.get_state_list().issue_source_counter++;
    rb.all_consumers_sourced_from_bypass &= (rb.state == PHYSREG_BYPASS);
    per_physregfile_stats_update(stats.ooocore.issue.source, rb.rfid, [rb.state]++);
  }

  if unlikely ((!uop.rcimm) & (rc.nonnull())) {
    rc.get_state_list().issue_source_counter++;
    rc.all_consumers_sourced_from_bypass &= (rc.state == PHYSREG_BYPASS);
    per_physregfile_stats_update(stats.ooocore.issue.source, rc.rfid, [rc.state]++);
  }

  bool propagated_exception = 0;
  if unlikely ((ra.flags | rb.flags | rc.flags) & FLAG_INV) {
    //
    // Invalid data propagated through operands: mark output as
    // invalid and don't even execute the uop at all.
    //
    state.st.invalid = 1;
    state.reg.rdflags = FLAG_INV;
    state.reg.rddata = EXCEPTION_Propagate;
    propagated_exception = 1;
  } else {
    per_context_ooocore_stats_update(threadid, issue.opclass[opclassof(uop.opcode)]++);

    if unlikely (ld|st) {
      int completed = 0;
      if likely (ld) {
        completed = issueload(*lsq, origvirt, radata, rbdata, rcdata, pteupdate);
      } else if unlikely (uop.opcode == OP_mf) {
        completed = issuefence(*lsq);
      } else {
        completed = issuestore(*lsq, origvirt, radata, rbdata, rcdata, operands[2]->ready(), pteupdate);
      }

      if unlikely (completed == ISSUE_MISSPECULATED) {
        per_context_ooocore_stats_update(threadid, issue.result.misspeculated++);
        return -1;
      } else if unlikely (completed == ISSUE_NEEDS_REFETCH) {
        per_context_ooocore_stats_update(threadid, issue.result.refetch++);
        return -1;
      }

      state.reg.rddata = lsq->data;
      state.reg.rdflags = (lsq->invalid << log2(FLAG_INV)) | ((!lsq->datavalid) << log2(FLAG_WAIT));
      if unlikely (completed == ISSUE_NEEDS_REPLAY) {
        per_context_ooocore_stats_update(threadid, issue.result.replay++);
        return 0;
      }
    } else if unlikely (uop.opcode == OP_ld_pre) {
      issueprefetch(state, radata, rbdata, rcdata, uop.cachelevel);
    } else {
      if unlikely (br) {
        state.brreg.riptaken = uop.riptaken;
        state.brreg.ripseq = uop.ripseq;
      }
      uop.synthop(state, radata, rbdata, rcdata, ra.flags, rb.flags, rc.flags); 
    }
  }

  physreg->flags = state.reg.rdflags;
  physreg->data = state.reg.rddata;

  if unlikely (!physreg->valid()) {
    //
    // If the uop caused an exception, force it directly to the commit
    // state and not through writeback (this keeps dependencies waiting until 
    // they can be properly annulled by the speculation logic.) The commit 
    // stage will detect the exception and take appropriate action.
    //
    // If the exceptional uop was speculatively executed beyond a
    // branch, it will never reach commit anyway since the branch would
    // have to commit before the exception was ever seen.
    //
    cycles_left = 0;
    changestate(thread.rob_ready_to_commit_queue);
    //
    // NOTE: The frontend should not necessarily be stalled on exceptions
    // when extensive speculation is in use, since re-dispatch can be used
    // without refetching to resolve these situations.
    //
    // stall_frontend = true;
  }

  //
  // Memory fences proceed directly to commit. Once they hit
  // the head of the ROB, they get recirculated back to the
  // rob_completed_list state so dependent uops can wake up.
  //
  if unlikely (uop.opcode == OP_mf) {
    cycles_left = 0;
    changestate(thread.rob_ready_to_commit_queue);
  }

  bool mispredicted = (physreg->data != uop.riptaken);

  if unlikely (config.event_log_enabled && (propagated_exception | (!(ld|st)))) {
    event = core.eventlog.add(EVENT_ISSUE_OK, this);
    event->issue.state = state;
    event->issue.cycles_left = cycles_left;
    event->issue.operand_data[0] = radata;
    event->issue.operand_data[1] = rbdata;
    event->issue.operand_data[2] = rcdata;
    event->issue.operand_flags[0] = ra.flags;
    event->issue.operand_flags[1] = rb.flags;
    event->issue.operand_flags[2] = rc.flags;
    event->issue.mispredicted = br & mispredicted;
    event->issue.predrip = uop.riptaken;
  }

  //
  // Release the issue queue entry, since we are beyond the point of no return:
  // the uop cannot possibly be replayed at this point, but may still be annulled
  // or re-dispatched in case of speculation failures.
  //
  release();

  this->issued = 1;

  if likely (physreg->valid()) {
    if unlikely (br) {
      int bptype = uop.predinfo.bptype;

      bool cond = bit(bptype, log2(BRANCH_HINT_COND));
      bool indir = bit(bptype, log2(BRANCH_HINT_INDIRECT));
      bool ret = bit(bptype, log2(BRANCH_HINT_RET));
        
      if unlikely (mispredicted) {
        per_context_ooocore_stats_update(threadid, branchpred.cond[MISPRED] += cond);
        per_context_ooocore_stats_update(threadid, branchpred.indir[MISPRED] += (indir & !ret));
        per_context_ooocore_stats_update(threadid, branchpred.ret[MISPRED] += ret);
        per_context_ooocore_stats_update(threadid, branchpred.summary[MISPRED]++);

        W64 realrip = physreg->data;

        //
        // Correct the branch directions and cond code field.
        // This is required since the branch may again be
        // re-dispatched if we mis-identified a mispredict
        // due to very deep speculation.
        //
        // Basically the riptaken field must always point
        // to the correct next instruction in the ROB after
        // the branch.
        //
        if likely (isclass(uop.opcode, OPCLASS_COND_BRANCH)) {
          assert(realrip == uop.ripseq);
          uop.cond = invert_cond(uop.cond);
          
          //
          // We need to be careful here: we already looked up the synthop for this
          // uop according to the old condition, so redo that here so we call the
          // correct code for the swapped condition.
          //
          uop.synthop = get_synthcode_for_cond_branch(uop.opcode, uop.cond, uop.size, 0);
          swap(uop.riptaken, uop.ripseq);
        } else if unlikely (isclass(uop.opcode, OPCLASS_INDIR_BRANCH)) {
          uop.riptaken = realrip;
          uop.ripseq = realrip;
        } else if unlikely (isclass(uop.opcode, OPCLASS_UNCOND_BRANCH)) { // unconditional branches need no special handling
          assert(realrip == uop.riptaken);
        }

        //
        // Early misprediction handling. Annul everything after the
        // branch and restart fetching in the correct direction
        //
        thread.annul_fetchq();
        annul_after();

        //
        // The fetch queue is reset and fetching is redirected to the
        // correct branch direction.
        //
        // Note that we do NOT just reissue the branch - this would be
        // pointless as we already know the correct direction since
        // it has already been issued once. Just let it writeback and
        // commit like it was predicted perfectly in the first place.
        //
        thread.reset_fetch_unit(realrip);
        per_context_ooocore_stats_update(threadid, issue.result.branch_mispredict++);

        return -1;
      } else {
        per_context_ooocore_stats_update(threadid, branchpred.cond[CORRECT] += cond);
        per_context_ooocore_stats_update(threadid, branchpred.indir[CORRECT] += (indir & !ret));
        per_context_ooocore_stats_update(threadid, branchpred.ret[CORRECT] += ret);
        per_context_ooocore_stats_update(threadid, branchpred.summary[CORRECT]++);
        per_context_ooocore_stats_update(threadid, issue.result.complete++);
      }
    } else {
      per_context_ooocore_stats_update(threadid, issue.result.complete++);
    }
  } else {
    per_context_ooocore_stats_update(threadid, issue.result.exception++);
  }

  return 1;
}

//
// Address generation common to both loads and stores
//
Waddr ReorderBufferEntry::addrgen(LoadStoreQueueEntry& state, Waddr& origaddr, Waddr& virtpage, W64 ra, W64 rb, W64 rc, PTEUpdate& pteupdate, Waddr& addr, int& exception, PageFaultErrorCode& pfec, bool& annul) {
  Context& ctx = getthread().ctx;

  bool st = isstore(uop.opcode);

  int sizeshift = uop.size;
  int aligntype = uop.cond;
  bool internal = uop.internal;
  bool signext = (uop.opcode == OP_ldx);

  addr = (st) ? (ra + rb) : ((aligntype == LDST_ALIGN_NORMAL) ? (ra + rb) : ra);
  //
  // x86-64 requires virtual addresses to be canonical: if bit 47 is set, 
  // all upper 16 bits must be set. If this is not true, we need to signal
  // a general protection fault.
  //
  addr = (W64)signext64(addr, 48);
  addr &= ctx.virt_addr_mask;
  origaddr = addr;
  annul = 0;

  uop.ld_st_truly_unaligned = (lowbits(origaddr, sizeshift) != 0);

  switch (aligntype) {
  case LDST_ALIGN_NORMAL:
    break;
  case LDST_ALIGN_LO:
    addr = floor(addr, 8); break;
  case LDST_ALIGN_HI:
    //
    // Is the high load ever even used? If not, don't check for exceptions;
    // otherwise we may erroneously flag page boundary conditions as invalid
    //
    addr = floor(addr, 8);
    annul = (floor(origaddr + ((1<<sizeshift)-1), 8) == addr);
    addr += 8; 
    break;
  }

  state.physaddr = addr >> 3;
  state.invalid = 0;

  virtpage = addr;

  //
  // Notice that datavalid is not set until both the rc operand to
  // store is ready AND any inherited SFR data is ready to merge.
  //
  state.addrvalid = 1;
  state.datavalid = 0;

  //
  // Special case: if no part of the actual user load/store falls inside
  // of the high 64 bits, do not perform the access and do not signal
  // any exceptions if that page was invalid.
  //
  // However, we must be extremely careful if we're inheriting an SFR
  // from an earlier store: the earlier store may have updated some
  // bytes in the high 64-bit chunk even though we're not updating
  // any bytes. In this case we still must do the write since it
  // could very well be the final commit to that address. In any
  // case, the SFR mismatch and LSAT must still be checked.
  //
  // The store commit code checks if the bytemask is zero and does
  // not attempt the actual store if so. This will always be correct
  // for high stores as described in this scenario.
  //

  exception = 0;

  Waddr physaddr = (annul) ? INVALID_PHYSADDR : ctx.check_and_translate(addr, uop.size, st, uop.internal, exception, pfec, pteupdate);
  return physaddr;
}

bool ReorderBufferEntry::handle_common_load_store_exceptions(LoadStoreQueueEntry& state, Waddr& origaddr, Waddr& addr, int& exception, PageFaultErrorCode& pfec) {
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();

  bool st = isstore(uop.opcode);
  int aligntype = uop.cond;

  state.invalid = 1;
  state.data = exception | ((W64)pfec << 32);
  state.datavalid = 1;

  if unlikely (config.event_log_enabled) core.eventlog.add_load_store((st) ? EVENT_STORE_EXCEPTION : EVENT_LOAD_EXCEPTION, this, null, addr);

  if unlikely (exception == EXCEPTION_UnalignedAccess) {
    //
    // If we have an unaligned access, locate the excepting uop in the
    // basic block cache through the uop.origop pointer. Directly set
    // the unaligned bit in the uop, and restart fetching at the start
    // of the x86 macro-op. The frontend will then split the uop into
    // low and high parts as it is refetched.
    //
    if unlikely (config.event_log_enabled) core.eventlog.add_load_store(EVENT_ALIGNMENT_FIXUP, this, null, addr);

    core.set_unaligned_hint(uop.rip, 1);

    thread.annul_fetchq();
    W64 recoveryrip = annul_after_and_including();
    thread.reset_fetch_unit(recoveryrip);

    if unlikely (st) {
      per_context_ooocore_stats_update(threadid, dcache.store.issue.unaligned++);
    } else {
      per_context_ooocore_stats_update(threadid, dcache.load.issue.unaligned++);
    }

    return false;
  }

  if unlikely (((exception == EXCEPTION_PageFaultOnRead) | (exception == EXCEPTION_PageFaultOnWrite)) & (aligntype == LDST_ALIGN_HI)) {
    //
    // If we have a page fault on an unaligned access, and this is the high
    // half (ld.hi / st.hi) of that access, the page fault address recorded
    // in CR2 must be at the very first byte of the second page the access
    // overlapped onto (otherwise the kernel will repeatedly fault in the
    // first page, even though that one is already present.
    //
    origaddr = addr;
  }

  if unlikely (st) {
    per_context_ooocore_stats_update(threadid, dcache.store.issue.exception++);
  } else {
    per_context_ooocore_stats_update(threadid, dcache.load.issue.exception++);
  }

  return true;
}

namespace OutOfOrderModel {
  // One global interlock buffer for all VCPUs:
  MemoryInterlockBuffer interlocks;
};

//
// Release the lock on the cache block touched by this ld.acq uop.
//
// The lock is not actually released until flush_mem_lock_release_list()
// is called, for instance after the entire macro-op has committed.
//
bool ReorderBufferEntry::release_mem_lock(bool forced) {
  if likely (!lock_acquired) return false;
 
  W64 physaddr = lsq->physaddr << 3;
  MemoryInterlockEntry* lock = interlocks.probe(physaddr);
  assert(lock);
 
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();
 
  if unlikely (config.event_log_enabled) {
    OutOfOrderCoreEvent* event = core.eventlog.add_load_store((forced) ? EVENT_STORE_LOCK_ANNULLED : EVENT_STORE_LOCK_RELEASED, this, null, physaddr);
    event->loadstore.locking_vcpuid = lock->vcpuid;
    event->loadstore.locking_uuid = lock->uuid;
    event->loadstore.locking_rob = lock->rob;
  }
 
  assert(lock->vcpuid == thread.ctx.vcpuid);
  assert(lock->uuid == uop.uuid);
  assert(lock->rob == index());

  //
  // Just add to the release list; do not invalidate until the macro-op commits
  //
  assert(thread.queued_mem_lock_release_count < lengthof(thread.queued_mem_lock_release_list));
  thread.queued_mem_lock_release_list[thread.queued_mem_lock_release_count++] = physaddr;

  lock_acquired = 0;
  return true;
}
 

//
// Stores have special dependency rules: they may issue as soon as operands ra and rb are ready,
// even if rc (the value to store) or rs (the store buffer to inherit from) is not yet ready or
// even known.
//
// After both ra and rb are ready, the store is moved to [ready_to_issue] as a first phase store.
// When the store issues, it generates its physical address [ra+rb] and establishes an SFR with
// the address marked valid but the data marked invalid.
//
// The sole purpose of doing this is to allow other loads and stores to create an rs dependency
// on the SFR output of the store.
//
// The store is then marked as a second phase store, since the address has been generated.
// When the store is replayed and rescheduled, it must now have all operands ready this time.
//

int ReorderBufferEntry::issuestore(LoadStoreQueueEntry& state, Waddr& origaddr, W64 ra, W64 rb, W64 rc, bool rcready, PTEUpdate& pteupdate) {
  ThreadContext& thread = getthread();
  Queue<LoadStoreQueueEntry, LSQ_SIZE>& LSQ = thread.LSQ;
  Queue<ReorderBufferEntry, ROB_SIZE>& ROB = thread.ROB;
  LoadStoreAliasPredictor& lsap = thread.lsap;

  time_this_scope(ctissuestore);

  OutOfOrderCore& core = getcore();
  OutOfOrderCoreEvent* event;

  int sizeshift = uop.size;
  int aligntype = uop.cond;
  
  Waddr addr;
  int exception = 0;
  PageFaultErrorCode pfec;
  bool annul;
  
  Waddr physaddr = addrgen(state, origaddr, virtpage, ra, rb, rc, pteupdate, addr, exception, pfec, annul);

  if unlikely (exception) {
    return (handle_common_load_store_exceptions(state, origaddr, addr, exception, pfec)) ? ISSUE_COMPLETED : ISSUE_MISSPECULATED;
  }

  per_context_ooocore_stats_update(threadid, dcache.store.type.aligned += ((!uop.internal) & (aligntype == LDST_ALIGN_NORMAL)));
  per_context_ooocore_stats_update(threadid, dcache.store.type.unaligned += ((!uop.internal) & (aligntype != LDST_ALIGN_NORMAL)));
  per_context_ooocore_stats_update(threadid, dcache.store.type.internal += uop.internal);
  per_context_ooocore_stats_update(threadid, dcache.store.size[sizeshift]++);

  state.physaddr = (annul) ? INVALID_PHYSADDR : (physaddr >> 3);

  //
  // The STQ is then searched for the most recent prior store S to same 64-bit block. If found, U's
  // rs dependency is set to S by setting the ROB's rs field to point to the prior store's physreg
  // and hence its ROB. If not found, U's rs dependency remains unset (i.e. to PHYS_REG_NULL).
  // If some prior stores are ambiguous (addresses not resolved yet), we assume they are a match
  // to ensure correctness yet avoid additional checks; the store is replayed and tries again 
  // when the ambiguous reference resolves.
  //
  // We also find the first store memory fence (mf.sfence uop) in the LSQ, and
  // if one exists, make this store dependent on the fence via its rs operand.
  // The mf uop issues immediately but does not complete until it's at the head
  // of the ROB and LSQ; only at that point can future loads and stores issue.
  //
  // All memory fences are considered stores, since in this way both loads and
  // stores can depend on them using the rs dependency.
  //

  LoadStoreQueueEntry* sfra = null;

  foreach_backward_before(LSQ, lsq, i) {
    LoadStoreQueueEntry& stbuf = LSQ[i];

    // Skip over loads (we only care about the store queue subset):
    if likely (!stbuf.store) continue;

    if likely (stbuf.addrvalid) {

      // Only considered a match if it's not a fence (which doesn't match anything)
      if unlikely (stbuf.lfence | stbuf.sfence) continue;

      if (stbuf.physaddr == state.physaddr) {
        per_context_ooocore_stats_update(threadid, dcache.load.dependency.stq_address_match++);
        sfra = &stbuf;
        break;
      }
    } else {
      //
      // Address is unknown: stores to a given word must issue in program order
      // to composite data correctly, but we can't do that without the address.
      //
      // This also catches any unresolved store fences (but not load fences).
      //

      if unlikely (stbuf.lfence & !stbuf.sfence) {
        // Stores can always pass load fences
        continue;
      }

      sfra = &stbuf;
      break;
    }
  }

  if (sfra && sfra->addrvalid && sfra->datavalid) {
    assert(sfra->physaddr == state.physaddr);
    assert(sfra->rob->uop.uuid < uop.uuid);
  }

  //
  // Always update deps in case redispatch is required
  // because of a future speculation failure: we must
  // know which loads and stores inherited bogus values
  //
  operands[RS]->unref(*this, thread.threadid);
  operands[RS] = (sfra) ? sfra->rob->physreg : &core.physregfiles[0][PHYS_REG_NULL];
  operands[RS]->addref(*this, thread.threadid);

  bool ready = (!sfra || (sfra && sfra->addrvalid && sfra->datavalid)) && rcready;

  //
  // If any of the following are true:
  // - Prior store S with same address is found but its data is not ready
  // - Prior store S with unknown address is found
  // - Data to store (rc operand) is not yet ready
  //
  // Then the store is moved back into [ready_to_dispatch], where this time all operands are checked.
  // The replay() function will put the newly selected prior store S's ROB as the rs dependency
  // of the current store before replaying it.
  //
  // When the current store wakes up again, it will rescan the STQ to see if any intervening stores
  // slipped in, and may repeatedly go back to sleep on the new store until the entire chain of stores
  // to a given location is resolved in the correct order. This does not mean all stores must issue in
  // program order - it simply means stores to the same address (8-byte chunk) are serialized in
  // program order, but out of order w.r.t. unrelated stores. This is similar to the constraints on
  // store buffer merging in Pentium 4 and AMD K8.
  //

  if unlikely (!ready) {
    if unlikely (config.event_log_enabled) {
      event = core.eventlog.add_load_store(EVENT_STORE_WAIT, this, sfra, addr);
      event->loadstore.rcready = rcready;
    }

    replay();
    load_store_second_phase = 1;

    if unlikely (sfra && sfra->sfence) {
      per_context_ooocore_stats_update(threadid, dcache.store.issue.replay.fence++);
    } else {
      per_context_ooocore_stats_update(threadid, dcache.store.issue.replay.sfr_addr_and_data_and_data_to_store_not_ready += ((!rcready) & (sfra && (!sfra->addrvalid) & (!sfra->datavalid))));
      per_context_ooocore_stats_update(threadid, dcache.store.issue.replay.sfr_addr_and_data_to_store_not_ready += ((!rcready) & (sfra && (!sfra->addrvalid))));
      per_context_ooocore_stats_update(threadid, dcache.store.issue.replay.sfr_data_and_data_to_store_not_ready += ((!rcready) & (sfra && sfra->addrvalid && (!sfra->datavalid))));
      
      per_context_ooocore_stats_update(threadid, dcache.store.issue.replay.sfr_addr_and_data_not_ready += (rcready & (sfra && (!sfra->addrvalid) & (!sfra->datavalid))));
      per_context_ooocore_stats_update(threadid, dcache.store.issue.replay.sfr_addr_not_ready += (rcready & (sfra && ((!sfra->addrvalid) & (sfra->datavalid)))));
      per_context_ooocore_stats_update(threadid, dcache.store.issue.replay.sfr_data_not_ready += (rcready & (sfra && (sfra->addrvalid & (!sfra->datavalid)))));
    }

    return ISSUE_NEEDS_REPLAY;
  }

  //
  // Load/Store Aliasing Prevention
  //
  // We always issue loads as soon as possible even if some entries in the
  // store queue have unresolved addresses. If a load gets erroneously
  // issued before an earlier store in program order to the same address,
  // this is considered load/store aliasing.
  // 
  // Aliasing is detected when stores issue: the load queue is scanned
  // for earlier loads in program order which collide with the store's
  // address. In this case all uops in program order after and including
  // the store (and by extension, the colliding load) must be annulled.
  //
  // To keep this from happening repeatedly, whenever a collision is
  // detected, the store looks up the rip of the colliding load and adds
  // it to a small table called the LSAP (load/store alias predictor).
  //
  // Loads query the LSAP with the rip of the load; if a matching entry
  // is found in the LSAP and the store address is unresolved, the load
  // is not allowed to proceed.
  //
  // Check all later loads in LDQ to see if any have already issued
  // and have already obtained their data but really should have 
  // depended on the data generated by this store. If so, mark the
  // store as invalid (EXCEPTION_LoadStoreAliasing) so it annuls
  // itself and the load after it in program order at commit time.
  //
  foreach_forward_after (LSQ, lsq, i) {
    LoadStoreQueueEntry& ldbuf = LSQ[i];
    //
    // (see notes on Load Replay Conditions below)
    //

    if unlikely ((!ldbuf.store) & ldbuf.addrvalid & ldbuf.rob->issued & (ldbuf.physaddr == state.physaddr)) {
      //
      // Check for the extremely rare case where:
      // - load is in the ready_to_load state at the start of the simulated 
      //   cycle, and is processed by load_issue()
      // - that load gets its data forwarded from a store (i.e., the store
      //   being handled here) scheduled for execution in the same cycle
      // - the load and the store alias each other
      //
      // Handle this by checking the list of addresses for loads processed
      // in the same cycle, and only signal a load speculation failure if
      // the aliased load truly came at least one cycle before the store.
      //
      int i;
      int parallel_forwarding_match = 0;
      foreach (i, thread.loads_in_this_cycle) {
        bool match = (thread.load_to_store_parallel_forwarding_buffer[i] == state.physaddr);
        parallel_forwarding_match |= match;
      }

      if unlikely (parallel_forwarding_match) {
        if unlikely (config.event_log_enabled) event = core.eventlog.add_load_store(EVENT_STORE_PARALLEL_FORWARDING_MATCH, this, &ldbuf, addr);
        per_context_ooocore_stats_update(threadid, dcache.store.issue.replay.parallel_aliasing++);

        replay();
        return ISSUE_NEEDS_REPLAY;
      }

      state.invalid = 1;
      state.data = EXCEPTION_LoadStoreAliasing;
      state.datavalid = 1;

      if unlikely (config.event_log_enabled) event = core.eventlog.add_load_store(EVENT_STORE_ALIASED_LOAD, this, &ldbuf, addr);

      // Add the rip to the load to the load/store alias predictor:
      lsap.select(ldbuf.rob->uop.rip);
      //
      // The load as dependent on this store. Add a new dependency
      // on the store to the load so the normal redispatch mechanism
      // will find this.
      //
      ldbuf.rob->operands[RS]->unref(*this, thread.threadid);
      ldbuf.rob->operands[RS] = physreg;
      ldbuf.rob->operands[RS]->addref(*this, thread.threadid);

      redispatch_dependents();

      per_context_ooocore_stats_update(threadid, dcache.store.issue.ordering++);

      return ISSUE_MISSPECULATED;
    }
  }

  //
  // Cache coherent interlocking
  //
  if unlikely ((contextcount > 1) && (!annul)) {
    //    assert(0);
    W64 physaddr = state.physaddr << 3;
    MemoryInterlockEntry* lock = interlocks.probe(physaddr);

    //
    // All store instructions check if a lock is held by another thread,
    // even if the load lacks the LOCK prefix.
    //
    // This prevents mixing of threads in cases where e.g. thread 0 is
    // acquiring a spinlock by a LOCK DEC of the low 4 bytes of the 8-byte
    // cache chunk, while thread 1 is releasing a spinlock using an
    // *unlocked* ADD [mem],1 on the high 4 bytes of the chunk.
    //

    if unlikely (lock && (lock->vcpuid != thread.ctx.vcpuid)) {
      //
      // Non-interlocked store intersected with a previously
      // locked block. We must replay the store until the block
      // becomes unlocked.
      //
      if unlikely (config.event_log_enabled) {
        event = core.eventlog.add_load_store(EVENT_STORE_LOCK_REPLAY, this, null, addr);
        event->loadstore.locking_vcpuid = lock->vcpuid;
        event->loadstore.locking_uuid = lock->uuid;
        event->loadstore.threadid = lock->threadid;   
      }

      per_context_ooocore_stats_update(threadid, dcache.store.issue.replay.interlocked++);
      replay_locked();
      return ISSUE_NEEDS_REPLAY;
    }

    //
    // st.rel unlocks the chunk ONLY at commit time. This is required since the 
    // other threads knows nothing about remote store queues: unless the value is 
    // committed to the cache (where cache coherency can control its use), other 
    // loads in other threads could slip in and get incorrect values.
    //
  }
 
  //
  // At this point all operands are valid, so merge the data and mark the store as valid.
  //

  byte bytemask = 0;

  switch (aligntype) {
  case LDST_ALIGN_NORMAL:
  case LDST_ALIGN_LO:
    bytemask = ((1 << (1 << sizeshift))-1) << (lowbits(origaddr, 3));
    rc <<= 8*lowbits(origaddr, 3);
    break;
  case LDST_ALIGN_HI:
    bytemask = ((1 << (1 << sizeshift))-1) >> (8 - lowbits(origaddr, 3));
    rc >>= 8*(8 - lowbits(origaddr, 3));
  }

  state.invalid = 0;
  state.data = (sfra) ? mux64(expand_8bit_to_64bit_lut[bytemask], sfra->data, rc) : rc;
  state.bytemask = (sfra) ? (sfra->bytemask | bytemask) : bytemask;
  state.datavalid = 1;

  per_context_ooocore_stats_update(threadid, dcache.store.forward.zero += (sfra == null));
  per_context_ooocore_stats_update(threadid, dcache.store.forward.sfr += (sfra != null));
  per_context_ooocore_stats_update(threadid, dcache.store.datatype[uop.datatype]++);

  if unlikely (config.event_log_enabled) {
    event = core.eventlog.add_load_store(EVENT_STORE_ISSUED, this, sfra, addr);
    event->loadstore.data_to_store = rc;
  }

  load_store_second_phase = 1;

  per_context_ooocore_stats_update(threadid, dcache.store.issue.complete++);

  return ISSUE_COMPLETED;
}

static inline W64 extract_bytes(void* target, int SIZESHIFT, bool SIGNEXT) {
  W64 data;
  switch (SIZESHIFT) {
  case 0:
    data = (SIGNEXT) ? (W64s)(*(W8s*)target) : (*(W8*)target); break;
  case 1:
    data = (SIGNEXT) ? (W64s)(*(W16s*)target) : (*(W16*)target); break;
  case 2:
    data = (SIGNEXT) ? (W64s)(*(W32s*)target) : (*(W32*)target); break;
  case 3:
    data = *(W64*)target; break;
  }
  return data;
}

int ReorderBufferEntry::issueload(LoadStoreQueueEntry& state, Waddr& origaddr, W64 ra, W64 rb, W64 rc, PTEUpdate& pteupdate) {
  time_this_scope(ctissueload);

  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();
  Queue<LoadStoreQueueEntry, LSQ_SIZE>& LSQ = thread.LSQ;
  LoadStoreAliasPredictor& lsap = thread.lsap;

  OutOfOrderCoreEvent* event;

  int sizeshift = uop.size;
  int aligntype = uop.cond;
  bool signext = (uop.opcode == OP_ldx);

  Waddr addr;
  int exception = 0;
  PageFaultErrorCode pfec;
  bool annul;

  Waddr physaddr = addrgen(state, origaddr, virtpage, ra, rb, rc, pteupdate, addr, exception, pfec, annul);

  if unlikely (exception) {
    return (handle_common_load_store_exceptions(state, origaddr, addr, exception, pfec)) ? ISSUE_COMPLETED : ISSUE_MISSPECULATED;
  }

  per_context_ooocore_stats_update(threadid, dcache.load.type.aligned += ((!uop.internal) & (aligntype == LDST_ALIGN_NORMAL)));
  per_context_ooocore_stats_update(threadid, dcache.load.type.unaligned += ((!uop.internal) & (aligntype != LDST_ALIGN_NORMAL)));
  per_context_ooocore_stats_update(threadid, dcache.load.type.internal += uop.internal);
  per_context_ooocore_stats_update(threadid, dcache.load.size[sizeshift]++);

  state.physaddr = (annul) ? INVALID_PHYSADDR : (physaddr >> 3);

  //
  // For simulation purposes only, load the data immediately
  // so it is easier to track. In the hardware this obviously
  // only arrives later, but it saves us from having to copy
  // cache lines around...
  //
  W64 data = (annul) ? 0 : loadphys(physaddr);

  LoadStoreQueueEntry* sfra = null;

#ifdef SMT_ENABLE_LOAD_HOISTING
  bool load_is_known_to_alias_with_store = (lsap(uop.rip) >= 0);
#else
  // For processors that cannot speculatively issue loads before unresolved stores:
  bool load_is_known_to_alias_with_store = 1;
#endif
  //
  // Search the store queue for the most recent store to the same address.
  //
  // We also find the first load memory fence (mf.lfence uop) in the LSQ, and
  // if one exists, make this load dependent on the fence via its rs operand.
  // The mf uop issues immediately but does not complete until it's at the head
  // of the ROB and LSQ; only at that point can future loads and stores issue.
  //
  // All memory fence are considered stores, since in this way both loads and
  // stores can depend on them using the rs dependency.
  //

  foreach_backward_before(LSQ, lsq, i) {
    LoadStoreQueueEntry& stbuf = LSQ[i];
    
    // Skip over loads (we only care about the store queue subset):
    if likely (!stbuf.store) continue;

    if likely (stbuf.addrvalid) {
      // Only considered a match if it's not a fence (which doesn't match anything)
      if unlikely (stbuf.lfence | stbuf.sfence) continue;

      if (stbuf.physaddr == state.physaddr) {
        per_context_ooocore_stats_update(threadid, dcache.load.dependency.stq_address_match++);
        sfra = &stbuf;
        break;
      }
    } else {
      // Address is unknown: is it a memory fence that hasn't committed?
      if unlikely (stbuf.lfence) {
        per_context_ooocore_stats_update(threadid, dcache.load.dependency.fence++);
        sfra = &stbuf;
        break;
      }

      if unlikely (stbuf.sfence) {
        // Loads can always pass store fences
        continue;
      }

      // Is this load known to alias with prior stores, and therefore cannot be hoisted?
      if unlikely (load_is_known_to_alias_with_store) {
        per_context_ooocore_stats_update(threadid, dcache.load.dependency.predicted_alias_unresolved++);
        sfra = &stbuf;
        break;
      }
    }
  }

  per_context_ooocore_stats_update(threadid, dcache.load.dependency.independent += (sfra == null));

  bool ready = (!sfra || (sfra && sfra->addrvalid && sfra->datavalid));

  if (sfra && sfra->addrvalid && sfra->datavalid) assert(sfra->physaddr == state.physaddr);

  //
  // Always update deps in case redispatch is required
  // because of a future speculation failure: we must
  // know which loads and stores inherited bogus values
  //
  operands[RS]->unref(*this, thread.threadid);
  operands[RS] = (sfra) ? sfra->rob->physreg : &core.physregfiles[0][PHYS_REG_NULL];
  operands[RS]->addref(*this, thread.threadid);

  if unlikely (!ready) {
    //
    // Load Replay Conditions:
    //
    // - Earlier store is known to alias (based on rip) yet its address is not yet resolved
    // - Earlier store to the same 8-byte chunk was found but its data has not yet arrived
    //
    // In these cases we create an rs dependency on the earlier store and replay the load uop
    // back to the dispatched state. It will be re-issued once the earlier store resolves.
    //
    // Consider the following sequence of events:
    // - Load B issues
    // - Store A issues and detects aliasing with load B; both A and B annulled
    // - Load B attempts to re-issue but aliasing is predicted, so it creates a dependency on store A
    // - Store A issues but sees that load B has already attempted to issue, so an aliasing replay is taken
    //
    // This becomes an infinite loop unless we clear both the addrvalid and datavalid fields of loads
    // when they replay; clearing both suppresses the aliasing replay the second time around.
    //

    assert(sfra);

    if unlikely (config.event_log_enabled) {
      event = core.eventlog.add_load_store(EVENT_LOAD_WAIT, this, sfra, addr);
      event->loadstore.predicted_alias = (load_is_known_to_alias_with_store && sfra && (!sfra->addrvalid));
    }

    if unlikely (sfra->lfence | sfra->sfence) {
      per_context_ooocore_stats_update(threadid, dcache.load.issue.replay.fence++);
    } else {
      per_context_ooocore_stats_update(threadid, dcache.load.issue.replay.sfr_addr_and_data_not_ready += ((!sfra->addrvalid) & (!sfra->datavalid)));
      per_context_ooocore_stats_update(threadid, dcache.load.issue.replay.sfr_addr_not_ready += ((!sfra->addrvalid) & (sfra->datavalid)));
      per_context_ooocore_stats_update(threadid, dcache.load.issue.replay.sfr_data_not_ready += ((sfra->addrvalid) & (!sfra->datavalid)));
    }

    replay();
    load_store_second_phase = 1;
    return ISSUE_NEEDS_REPLAY;
  }

#ifdef ENFORCE_L1_DCACHE_BANK_CONFLICTS
  foreach (i, thread.loads_in_this_cycle) {
    W64 prevaddr = thread.load_to_store_parallel_forwarding_buffer[i];
    //
    // Replay any loads that collide on the same bank.
    //
    // Two or more loads from the exact same 8-byte chunk are still
    // allowed since the chunk has been loaded anyway, so we might
    // as well use it.
    //
    if unlikely ((prevaddr != state.physaddr) && (lowbits(prevaddr, log2(CacheSubsystem::L1_DCACHE_BANKS)) == lowbits(state.physaddr, log2(CacheSubsystem::L1_DCACHE_BANKS)))) {
      if unlikely (config.event_log_enabled) core.eventlog.add_load_store(EVENT_LOAD_BANK_CONFLICT, this, null, addr);
      per_context_ooocore_stats_update(threadid, dcache.load.issue.replay.bank_conflict++);

      replay();
      load_store_second_phase = 1;
      return ISSUE_NEEDS_REPLAY;
    }
  }
#endif
  //
  // Guarantee that we have at least one LFRQ entry reserved for us.
  // Technically this is only needed later, but it simplifies the
  // control logic to avoid replays once we pass this point.
  //
  if unlikely (core.caches.lfrq_or_missbuf_full()) {
    if unlikely (config.event_log_enabled) core.eventlog.add_load_store(EVENT_LOAD_LFRQ_FULL, this, null, addr);
    per_context_ooocore_stats_update(threadid, dcache.load.issue.replay.missbuf_full++);

    replay();
    load_store_second_phase = 1;
    return ISSUE_NEEDS_REPLAY;
  }

  //
  // Cache coherent interlocking
  //
  // On SMP systems, we must check the memory interlock controller
  // to make sure no other thread or core has started but not finished
  // an atomic instruction on the same word as we are accessing.
  //
  // SMT cores must also use this, since the MESI cache coherence
  // system doesn't work across multiple threads on the same core.
  //
  if unlikely ((contextcount > 1) && (!annul)) {
    MemoryInterlockEntry* lock = interlocks.probe(physaddr);

    //
    // All load instructions check if a lock is held by another thread,
    // even if the load lacks the LOCK prefix.
    //
    // This prevents mixing of threads in cases where e.g. thread 0 is
    // acquiring a spinlock by a LOCK DEC of the low 4 bytes of the 8-byte
    // cache chunk, while thread 1 is releasing a spinlock using an
    // *unlocked* ADD [mem],1 on the high 4 bytes of the chunk.
    //
    if unlikely (lock && (lock->vcpuid != thread.ctx.vcpuid)) {
      //
      // Some other thread or core has locked up this word: replay
      // the uop until it becomes unlocked.
      //
      if unlikely (config.event_log_enabled) {
        event = core.eventlog.add_load_store(EVENT_LOAD_LOCK_REPLAY, this, null, addr);
        event->loadstore.locking_vcpuid = lock->vcpuid;
        event->loadstore.locking_uuid = lock->uuid;
        event->loadstore.locking_rob = lock->rob;
      }
 
      // Double-locking within a thread is NOT allowed!
      assert(lock->vcpuid != thread.ctx.vcpuid);
      assert(lock->threadid != threadid);

      per_context_ooocore_stats_update(threadid, dcache.load.issue.replay.interlocked++);
      replay_locked();
      return ISSUE_NEEDS_REPLAY;
    }

    // Issuing more than one ld.acq on the same block is not allowed:
    if (lock) {
      logfile << "ERROR: thread ", thread.ctx.vcpuid, " uuid ", uop.uuid, " over physaddr ", (void*)physaddr, ": lock was already acquired by vcpuid ", lock->vcpuid, " uuid ", lock->uuid, " rob ", lock->rob, endl;
      assert(false);
    }
 
    if unlikely (uop.locked) {
      //
      // Attempt to acquire an exclusive lock on the block via ld.acq,
      // or replay if another core or thread already has the lock.
      //
      // Each block can only be locked once, i.e. locks are not recursive
      // even within a single VCPU. Any violations of these conditions
      // represent an error in the microcode.
      //
      // If we're attempting to release a lock on block X via st.rel,
      // another ld.acq uop executed within the same macro-op running
      // on the current core must have acquired it. Any violations of
      // these conditions represent an error in the microcode.
      //

      lock = interlocks.select_and_lock(physaddr);
 
      if unlikely (!lock) {
        //
        // We've overflowed the interlock buffer.
        // Replay the load until some entries free up.
        //
        // The maximum number of blocks lockable by any macro-op
        // is two. As long as the lock buffer associativity is
        // bigger than this, we will eventually get an entry.
        //
        if unlikely (config.event_log_enabled) {
          core.eventlog.add_load_store(EVENT_LOAD_LOCK_OVERFLOW, this, null, addr);
        }

        per_context_ooocore_stats_update(threadid, dcache.load.issue.replay.interlock_overflow++);
        replay();
        return ISSUE_NEEDS_REPLAY;
      }
 
      lock->vcpuid = thread.ctx.vcpuid;
      lock->uuid = uop.uuid;
      lock->rob = index();
      lock->threadid = threadid;
      lock_acquired = 1;
      
      if unlikely (config.event_log_enabled) {
        core.eventlog.add_load_store(EVENT_LOAD_LOCK_ACQUIRED, this, null, addr);
      }
    }
  }
 
  state.addrvalid = 1;

  if unlikely (aligntype == LDST_ALIGN_HI) {
    //
    // Concatenate the aligned data from a previous ld.lo uop provided in rb
    // with the currently loaded data D as follows:
    //
    // rb | D
    //
    // Example:
    //
    // floor(a) floor(a)+8
    // ---rb--  --DD---
    // 0123456701234567
    //    XXXXXXXX
    //    ^ origaddr
    //
    if likely (!annul) {
      if unlikely (sfra) data = mux64(expand_8bit_to_64bit_lut[sfra->bytemask], data, sfra->data);
      
      struct {
        W64 lo;
        W64 hi;
      } aligner;
      
      aligner.lo = rb;
      aligner.hi = data;
      
      W64 offset = lowbits(origaddr - floor(origaddr, 8), 4);

      data = extract_bytes(((byte*)&aligner) + offset, sizeshift, signext);
    } else {
      //
      // annulled: we need no data from the high load anyway; only use the low data
      // that was already checked for exceptions and forwarding:
      //
      W64 offset = lowbits(origaddr, 3);
      state.data = extract_bytes(((byte*)&rb) + offset, sizeshift, signext);
      state.invalid = 0;
      state.datavalid = 1;

      if unlikely (config.event_log_enabled) core.eventlog.add_load_store(EVENT_LOAD_HIGH_ANNULLED, this, sfra, addr);

      return ISSUE_COMPLETED;
    }
  } else {
    if unlikely (sfra) data = mux64(expand_8bit_to_64bit_lut[sfra->bytemask], data, sfra->data);
    data = extract_bytes(((byte*)&data) + lowbits(addr, 3), sizeshift, signext);
  }

  // shift is how many bits to shift the 8-bit bytemask left by within the cache line;
  bool covered = core.caches.covered_by_sfr(addr, sfra, sizeshift);
  per_context_ooocore_stats_update(threadid, dcache.load.forward.cache += (sfra == null));
  per_context_ooocore_stats_update(threadid, dcache.load.forward.sfr += ((sfra != null) & covered));
  per_context_ooocore_stats_update(threadid, dcache.load.forward.sfr_and_cache += ((sfra != null) & (!covered)));
  per_context_ooocore_stats_update(threadid, dcache.load.datatype[uop.datatype]++);

  //
  // NOTE: Technically the data is valid right now for simulation purposes
  // only; in reality it may still be arriving from the cache.
  //
  state.data = data;
  state.invalid = 0;
  state.bytemask = 0xff;
  tlb_walk_level = 0;

  assert(thread.loads_in_this_cycle < LOAD_FU_COUNT);
  thread.load_to_store_parallel_forwarding_buffer[thread.loads_in_this_cycle++] = state.physaddr;

  if unlikely (uop.internal) {
    cycles_left = LOADLAT;

    if unlikely (config.event_log_enabled) core.eventlog.add_load_store(EVENT_LOAD_HIT, this, sfra, addr);

    load_store_second_phase = 1;
    state.datavalid = 1;
    physreg->flags &= ~FLAG_WAIT;
    physreg->complete();
    changestate(thread.rob_issued_list[cluster]);
    lfrqslot = -1;
    forward_cycle = 0;

    return ISSUE_COMPLETED;
  }

#ifdef USE_TLB
  if unlikely (!core.caches.dtlb.probe(addr, threadid)) {
    //
    // TLB miss: 
    //
    if unlikely (config.event_log_enabled) event = core.eventlog.add_load_store(EVENT_LOAD_TLB_MISS, this, sfra, addr);
    cycles_left = 0;
    tlb_walk_level = thread.ctx.page_table_level_count();
    changestate(thread.rob_tlb_miss_list);
    per_context_dcache_stats_update(threadid, load.dtlb.misses++);
    
    return ISSUE_COMPLETED;
  }

  per_context_dcache_stats_update(threadid, load.dtlb.hits++);
#endif

  return probecache(physaddr, sfra);
}

//
// Probe the cache and initiate a miss if required
//
int ReorderBufferEntry::probecache(Waddr addr, LoadStoreQueueEntry* sfra) {
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();
  OutOfOrderCoreEvent* event;
  int sizeshift = uop.size;
  int aligntype = uop.cond;
  bool signext = (uop.opcode == OP_ldx);

  LoadStoreQueueEntry& state = *lsq;
  W64 physaddr = state.physaddr << 3;

  bool L1hit = (config.perfect_cache) ? 1 : core.caches.probe_cache_and_sfr(physaddr, sfra, sizeshift);

  if likely (L1hit) {    
    cycles_left = LOADLAT;

    if unlikely (config.event_log_enabled) core.eventlog.add_load_store(EVENT_LOAD_HIT, this, sfra, addr);
    
    load_store_second_phase = 1;
    state.datavalid = 1;
    physreg->flags &= ~FLAG_WAIT;
    physreg->complete();
    changestate(thread.rob_issued_list[cluster]);
    lfrqslot = -1;
    forward_cycle = 0;

    per_context_ooocore_stats_update(threadid, dcache.load.issue.complete++);
    per_context_dcache_stats_update(threadid, load.hit.L1++);
    return ISSUE_COMPLETED;
  }

  per_context_ooocore_stats_update(threadid, dcache.load.issue.miss++);

  cycles_left = 0;
  changestate(thread.rob_cache_miss_list);

  LoadStoreInfo lsi;
  lsi.threadid = thread.threadid;
  lsi.rob = index();
  lsi.sizeshift = sizeshift;
  lsi.aligntype = aligntype;
  lsi.sfrused = 0;
  lsi.internal = uop.internal;
  lsi.signext = signext;

  SFR dummysfr;
  setzero(dummysfr);
  lfrqslot = core.caches.issueload_slowpath(physaddr, dummysfr, lsi);
  assert(lfrqslot >= 0);

  if unlikely (config.event_log_enabled) event = core.eventlog.add_load_store(EVENT_LOAD_MISS, this, sfra, addr);

  return ISSUE_COMPLETED;
}

//
// Hardware page table walk state machine:
// One execution per page table tree level (4 levels)
//
#ifdef PTLSIM_HYPERVISOR
void ReorderBufferEntry::tlbwalk() {
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();
  OutOfOrderCoreEvent* event;
  W64 virtaddr = virtpage;

  if unlikely (!tlb_walk_level) {
    // End of walk sequence: try to probe cache
    if unlikely (core.caches.lfrq_or_missbuf_full()) {
      //
      // Make sure we have at least one miss buffer entry free, to avoid deadlock.
      // This is required because the load or store cannot be replayed if no MB
      // entries are free (since the uop already left the scheduler).
      //
      if unlikely (config.event_log_enabled) event = core.eventlog.add_load_store(EVENT_TLBWALK_NO_LFRQ_MB, this, null, 0);
      per_context_dcache_stats_update(threadid, load.tlbwalk.no_lfrq_mb++);
      return;
    }

    if unlikely (config.event_log_enabled) event = core.eventlog.add_load_store(EVENT_TLBWALK_COMPLETE, this, null, virtaddr);
    core.caches.dtlb.insert(virtaddr, threadid);

    if unlikely (isprefetch(uop.opcode)) {
      physreg->flags &= ~FLAG_WAIT;
      physreg->complete();
      changestate(thread.rob_issued_list[cluster]);
      forward_cycle = 0;
      int exception;
      PageFaultErrorCode pfec;
      PTEUpdate pteupdate;
      Context& ctx = getthread().ctx;
      Waddr physaddr = ctx.check_and_translate(virtaddr, 1, 0, 0, exception, pfec, pteupdate);
      core.caches.initiate_prefetch(physaddr, uop.cachelevel);
    } else {
      probecache(virtaddr, null);
    }

    return;
  }

  W64 pteaddr = thread.ctx.virt_to_pte_phys_addr(virtaddr, tlb_walk_level - 1);
  bool L1hit = (config.perfect_cache) ? 1 : core.caches.probe_cache_and_sfr(pteaddr, null, 3);

  if likely (L1hit) {
    //
    // The PTE was in the cache: directly proceed to the next level
    //
    if unlikely (config.event_log_enabled) event = core.eventlog.add_load_store(EVENT_TLBWALK_HIT, this, null, pteaddr);
    per_context_dcache_stats_update(threadid, load.tlbwalk.L1_dcache_hit++);

    tlb_walk_level--;
    return;
  }

  LoadStoreInfo lsi = 0;
  lsi.threadid = thread.threadid;
  lsi.rob = index();

  SFR dummysfr;
  setzero(dummysfr);
  lfrqslot = core.caches.issueload_slowpath(pteaddr, dummysfr, lsi);

  //
  // No LFRQ or MB slots? Try again on next cycle.
  // TODO: For prefetches, we might want to drop the TLB miss!
  //
  if (lfrqslot < 0) {
    if unlikely (config.event_log_enabled) event = core.eventlog.add_load_store(EVENT_TLBWALK_NO_LFRQ_MB, this, null, pteaddr);
    per_context_dcache_stats_update(threadid, load.tlbwalk.no_lfrq_mb++);
    return;
  }

  cycles_left = 0;
  changestate(thread.rob_cache_miss_list);

  if unlikely (config.event_log_enabled) event = core.eventlog.add_load_store(EVENT_TLBWALK_MISS, this, null, pteaddr);
  per_context_dcache_stats_update(threadid, load.tlbwalk.L1_dcache_miss++);
}

void ThreadContext::tlbwalk() {
  time_this_scope(ctfrontend);

  ReorderBufferEntry* rob;
  foreach_list_mutable(rob_tlb_miss_list, rob, entry, nextentry) {
   rob->tlbwalk();
  }
}
#endif

//
// Find the newest memory fence in program order before the specified ROB,
// so a dependency can be created to avoid immediate replay.
//
LoadStoreQueueEntry* ReorderBufferEntry::find_nearest_memory_fence() {
  ThreadContext& thread = getthread();

  bool ld = isload(uop.opcode);
  bool st = (uop.opcode == OP_st);

  foreach_backward_before(thread.LSQ, lsq, i) {
    LoadStoreQueueEntry& stbuf = thread.LSQ[i];
    
    // Skip over everything except fences
    if unlikely (!(stbuf.lfence | stbuf.sfence)) continue;
    
    // Skip over fences that have already completed
    if unlikely (stbuf.addrvalid) continue;
    
    // Do not allow loads to pass lfence or mfence
    // Do not allow stores to pass sfence or mfence
    bool match = (ld) ? stbuf.lfence : (st) ? stbuf.sfence : 0;
    if unlikely (match) return &stbuf;
    
    // Loads can always pass store fences
    // Stores can always pass load fences
  }

  return null;
}

//
// Issue a memory fence (mf.lfence, mf.sfence, mf.mfence uops)
//
// The mf uop issues immediately but does not complete until it's at the head
// of the ROB and LSQ; only at that point can future loads or stores issue.
//
// All memory fence are considered stores, since in this way both loads and
// stores can depend on them using the rs dependency.
//
// This implementation closely models the Intel Pentium 4 technique described
// in U.S. Patent 6651151, "MFENCE and LFENCE Microarchitectural Implementation
// Method and System" (S. Palanca et al), filed 12 Jul 2002.
//
int ReorderBufferEntry::issuefence(LoadStoreQueueEntry& state) {
  ThreadContext& thread = getthread();

  OutOfOrderCore& core = getcore();
  OutOfOrderCoreEvent* event;

  assert(uop.opcode == OP_mf);

  per_context_ooocore_stats_update(threadid, dcache.fence.lfence += (uop.extshift == MF_TYPE_LFENCE));
  per_context_ooocore_stats_update(threadid, dcache.fence.sfence += (uop.extshift == MF_TYPE_SFENCE));
  per_context_ooocore_stats_update(threadid, dcache.fence.mfence += (uop.extshift == (MF_TYPE_LFENCE|MF_TYPE_SFENCE)));

  //
  // The mf uop is issued but its "data" (for dependency purposes only)
  // does not arrive until it's at the head of the LSQ. It
  //
  state.data = 0;
  state.invalid = 0;
  state.bytemask = 0xff;
  state.datavalid = 0;
  state.addrvalid = 0;
  state.physaddr = bitmask(48-3);

  if unlikely (config.event_log_enabled) {
    event = core.eventlog.add_load_store(EVENT_FENCE_ISSUED, this);
    event->loadstore.data_to_store = 0;
  }

  changestate(thread.rob_memory_fence_list);

  return ISSUE_COMPLETED;
}

//
// Issues a prefetch on the given memory address into the specified cache level.
//
void ReorderBufferEntry::issueprefetch(IssueState& state, W64 ra, W64 rb, W64 rc, int cachelevel) {
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();

  state.reg.rddata = 0;
  state.reg.rdflags = 0;

  int exception = 0;
  Waddr addr;
  Waddr origaddr;
  PTEUpdate pteupdate;
  PageFaultErrorCode pfec;
  bool annul;

  LoadStoreQueueEntry dummy;
  setzero(dummy);
  Waddr physaddr = addrgen(dummy, origaddr, virtpage, ra, rb, rc, pteupdate, addr, exception, pfec, annul);

  // Ignore bogus prefetches:
  if unlikely (exception) return;

  // Ignore unaligned prefetches (should never happen)
  if unlikely (annul) return;

  // (Stats are already updated by initiate_prefetch())
#ifdef USE_TLB
  if unlikely (!core.caches.dtlb.probe(addr, threadid)) {
#if 0
    //
    // TLB miss: Ignore this prefetch but handle the miss!
    //
    // Note that most x86 processors will not prefetch beyond 
    // a TLB miss, so this is disabled by default.
    //
    if unlikely (config.event_log_enabled) OutOfOrderCoreEvent* event = core.eventlog.add_load_store(EVENT_LOAD_TLB_MISS, this, null, addr);
    cycles_left = 0;
    tlb_walk_level = thread.ctx.page_table_level_count();
    changestate(thread.rob_tlb_miss_list);
    per_context_dcache_stats_update(thread.threadid, load.dtlb.misses++);
#endif
    return;
  }

  per_context_dcache_stats_update(threadid, load.dtlb.hits++);
#endif

  core.caches.initiate_prefetch(physaddr, cachelevel);
}

//
// Data cache has delivered a load: wake up corresponding ROB/LSQ/physreg entries
//
void OutOfOrderCoreCacheCallbacks::dcache_wakeup(LoadStoreInfo lsi, W64 physaddr) {
  int idx = lsi.rob;
  ThreadContext* thread = core.threads[lsi.threadid];
  assert(inrange(idx, 0, ROB_SIZE-1));
  ReorderBufferEntry& rob = thread->ROB[idx];

  if(logable(100)) logfile << " dcache_wakeup ", rob, endl;
  assert(rob.current_state_list == &thread->rob_cache_miss_list);

  rob.loadwakeup();
}

void ReorderBufferEntry::loadwakeup() {
  if (tlb_walk_level) {
    // Wake up from TLB walk wait and move to next level
    if unlikely (config.event_log_enabled) getcore().eventlog.add_load_store(EVENT_TLBWALK_WAKEUP, this);
    lfrqslot = -1;
    changestate(getthread().rob_tlb_miss_list);
  } else {
    // Actually wake up the load
    if unlikely (config.event_log_enabled) getcore().eventlog.add_load_store(EVENT_LOAD_WAKEUP, this);

    physreg->flags &= ~FLAG_WAIT;
    physreg->complete();
    
    lsq->datavalid = 1;
    
    changestate(getthread().rob_completed_list[cluster]);
    cycles_left = 0;
    lfrqslot = -1;
    forward_cycle = 0;
    fu = 0;
  }
}

void ReorderBufferEntry::fencewakeup() {
  ThreadContext& thread = getthread();

  if unlikely (config.event_log_enabled) getcore().eventlog.add_commit(EVENT_COMMIT_FENCE_COMPLETED, this);

  assert(!load_store_second_phase);
  assert(current_state_list == &thread.rob_ready_to_commit_queue);

  assert(!lsq->datavalid);
  assert(!lsq->addrvalid);
  
  physreg->flags &= ~FLAG_WAIT;
  physreg->data = 0;
  physreg->complete();
  lsq->datavalid = 1;
  lsq->addrvalid = 1;
  
  cycles_left = 0;
  lfrqslot = -1;
  forward_cycle = 0;
  fu = 0;

  //
  // Set flag to ensure that the second time it reaches
  // commit, it just sits there, rather than looping
  // back to completion and wakeup.
  //
  load_store_second_phase = 1;

  changestate(thread.rob_completed_list[cluster]);
}

//
// Replay the uop by recirculating it back to the dispatched
// state so it can wait for additional dependencies not known
// when it was originally dispatched, e.g. waiting on store
// queue entries or value to store, etc.
//
// This involves re-initializing the uop's operands in its
// already assigned issue queue slot and returning that slot
// to the dispatched but not issued state.
//
// This must be done here instead of simply sending the uop
// back to the dispatch state since otherwise we could have 
// a deadlock if there is not enough room in the issue queue.
//
void ReorderBufferEntry::replay() {
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();

  if unlikely (config.event_log_enabled) {
    OutOfOrderCoreEvent* event = core.eventlog.add(EVENT_REPLAY, this);
    foreach (i, MAX_OPERANDS) {
      operands[i]->fill_operand_info(event->replay.opinfo[i]);
      event->replay.ready |= (operands[i]->ready()) << i;
    }
  }

  assert(!lock_acquired);

  int operands_still_needed = 0;

  issueq_tag_t uopids[MAX_OPERANDS];
  issueq_tag_t preready[MAX_OPERANDS];

  foreach (operand, MAX_OPERANDS) {
    PhysicalRegister& source_physreg = *operands[operand];
    ReorderBufferEntry& source_rob = *source_physreg.rob;

    if likely (source_physreg.state == PHYSREG_WAITING) {
      uopids[operand] = source_rob.get_tag();
      preready[operand] = 0;
      operands_still_needed++;
    } else {
      // No need to wait for it
      uopids[operand] = 0;
      preready[operand] = 1;
    }
  }

  if unlikely (operands_still_needed) {
    changestate(thread.rob_dispatched_list[cluster]);
  } else {
    changestate(get_ready_to_issue_list());
  }

  issueq_operation_on_cluster(core, cluster, replay(iqslot, uopids, preready));  
}


void ReorderBufferEntry::replay_locked() {
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();

  if unlikely (config.event_log_enabled) {
    OutOfOrderCoreEvent* event = core.eventlog.add(EVENT_REPLAY, this);
    foreach (i, MAX_OPERANDS) {
      operands[i]->fill_operand_info(event->replay.opinfo[i]);
      event->replay.ready |= (operands[i]->ready()) << i;
    }
  }

  assert(!lock_acquired);
 
  int operands_still_needed = 0;

  issueq_tag_t uopids[MAX_OPERANDS];
  issueq_tag_t preready[MAX_OPERANDS];

  foreach (operand, MAX_OPERANDS) {
    PhysicalRegister& source_physreg = *operands[operand];
    ReorderBufferEntry& source_rob = *source_physreg.rob;

    if likely (source_physreg.state == PHYSREG_WAITING) {
      uopids[operand] = source_rob.get_tag();
      preready[operand] = 0;
      operands_still_needed++;
    } else {
      // No need to wait for it
      uopids[operand] = 0;
      preready[operand] = 1;
    }
  }

  if unlikely (operands_still_needed) {
    changestate(thread.rob_dispatched_list[cluster]);
  } else {
    changestate(get_ready_to_issue_list());
  }
  
  issueq_operation_on_cluster(core, cluster, switch_to_end(iqslot,  uopids, preready));  
}

//
// Release the ROB from the issue queue after there is
// no possibility it will need to be pulled back for
// replay or annulment.
//
void ReorderBufferEntry::release() {
  issueq_operation_on_cluster(getcore(), cluster, release(iqslot));
  
  ThreadContext& thread = getthread();
  OutOfOrderCore& core = thread.core;

  if unlikely (core.threadcount > 1) {
    if (thread.issueq_count > core.reserved_iq_entries) {
      issueq_operation_on_cluster(core, cluster, free_shared_entry());
    }
  }

  thread.issueq_count--;

  iqslot = -1;
}

//
// Process the ready to issue queue and issue as many ROBs as possible
//
int OutOfOrderCore::issue(int cluster) {
  time_this_scope(ctissue);

  int issuecount = 0;
  ReorderBufferEntry* rob;

  int maxwidth = clusters[cluster].issue_width;

  while (issuecount < maxwidth) {
    int iqslot;
    issueq_operation_on_cluster_with_result(getcore(), cluster, iqslot, issue());
  
    // Is anything ready?
    if unlikely (iqslot < 0) break;

    int robid;
    issueq_operation_on_cluster_with_result(getcore(), cluster, robid, uopof(iqslot));
    int threadid, idx;
    decode_tag(robid, threadid, idx);
    ThreadContext* thread = threads[threadid];
    assert(inrange(idx, 0, ROB_SIZE-1));
    ReorderBufferEntry& rob = thread->ROB[idx];

    rob.iqslot = iqslot;
    int rc = rob.issue();
    // Stop issuing from this cluster once something replays or has a mis-speculation
    issuecount++;
    if unlikely (rc <= 0) break;
  }

  per_cluster_stats_update(stats.ooocore.issue.width, cluster, [min(issuecount, MAX_ISSUE_WIDTH)]++);

  return issuecount;
}

//
// Forward the result of ROB 'result' to any other waiting ROBs
// dispatched to the issue queues. This is done by broadcasting
// the ROB tag to all issue queues in clusters reachable within
// N cycles after the uop issued, where N is forward_cycle. This
// technique is used to model arbitrarily complex multi-cycle
// forwarding networks.
//
int ReorderBufferEntry::forward() {
  ReorderBufferEntry* target;
  int wakeupcount = 0;

  assert(inrange((int)forward_cycle, 0, (MAX_FORWARDING_LATENCY+1)-1));

  W32 targets = forward_at_cycle_lut[cluster][forward_cycle];
  foreach (i, MAX_CLUSTERS) {
    if likely (!bit(targets, i)) continue;
    if unlikely (config.event_log_enabled) {
      OutOfOrderCoreEvent* event = getcore().eventlog.add(EVENT_BROADCAST, this);
      event->forwarding.target_cluster = i;
      event->forwarding.forward_cycle = forward_cycle;
    }

    issueq_operation_on_cluster(getcore(), i, broadcast(get_tag()));
  }

  return 0;
}

//
// Exception recovery and redispatch
//
// Remove any and all ROBs that entered the pipeline after and
// including the misspeculated uop. Because we move all affected
// ROBs to the free state, they are instantly taken out of 
// consideration for future pipeline stages and will be dropped on 
// the next cycle.
//
// Normally this means that mispredicted branch uops are annulled 
// even though only the code after the branch itself is invalid.
// In this special case, the recovery rip is set to the actual
// target of the branch rather than refetching the branch insn.
//
// We must be extremely careful to annul all uops in an
// x86 macro-op; otherwise half the x86 instruction could
// be executed twice once refetched. Therefore, if the
// first uop to annul is not also the first uop in the x86
// macro-op, we may have to scan backwards in the ROB until
// we find the first uop of the macro-op. In this way, we
// ensure that we can annul the entire macro-op. All uops
// comprising the macro-op are guaranteed to still be in 
// the ROB since none of the uops commit until the entire
// macro-op can commit.
//
// Note that this does not apply if the final uop in the
// macro-op is a branch and that branch uop itself is
// being retained as occurs with mispredicted branches.
//

W64 ReorderBufferEntry::annul(bool keep_misspec_uop, bool return_first_annulled_rip) {
  OutOfOrderCore& core = getcore();

  ThreadContext& thread = getthread();
  BranchPredictorInterface& branchpred = thread.branchpred;
  Queue<ReorderBufferEntry, ROB_SIZE>& ROB = thread.ROB;
  Queue<LoadStoreQueueEntry, LSQ_SIZE>& LSQ = thread.LSQ;
  RegisterRenameTable& specrrt = thread.specrrt;
  RegisterRenameTable& commitrrt = thread.commitrrt;
  int& loads_in_flight = thread.loads_in_flight;
  int& stores_in_flight = thread.stores_in_flight;
  int queued_locks_before = thread.queued_mem_lock_release_count;

  OutOfOrderCoreEvent* event;

  int idx;

  //
  // Pass 0: determine macro-op boundaries around uop
  //
  int somidx = index();


  while (!ROB[somidx].uop.som) somidx = add_index_modulo(somidx, -1, ROB_SIZE);
  int eomidx = index();
  while (!ROB[eomidx].uop.eom) eomidx = add_index_modulo(eomidx, +1, ROB_SIZE);

  // Find uop to start annulment at
  int startidx = (keep_misspec_uop) ? add_index_modulo(eomidx, +1, ROB_SIZE) : somidx;
  if unlikely (startidx == ROB.tail) {
    // The uop causing the mis-speculation was the only uop in the ROB:
    // no action is necessary (but in practice this is generally not possible)
    if unlikely (config.event_log_enabled) {
      OutOfOrderCoreEvent* event = core.eventlog.add(EVENT_ANNUL_NO_FUTURE_UOPS, this);
      event->annul.somidx = somidx; event->annul.eomidx = eomidx;
    }

    return uop.rip;
  }

  // Find uop to stop annulment at (later in program order)
  int endidx = add_index_modulo(ROB.tail, -1, ROB_SIZE);

  // For branches, branch must always terminate the macro-op
  if (keep_misspec_uop) assert(eomidx == index());

  if unlikely (config.event_log_enabled) {
    event = core.eventlog.add(EVENT_ANNUL_MISSPECULATION, this);
    event->annul.startidx = startidx; event->annul.endidx = endidx;
    event->annul.somidx = somidx; event->annul.eomidx = eomidx;
  }

  //
  // Pass 1: invalidate issue queue slot for the annulled ROB
  //
  idx = endidx;
  for (;;) {
    ReorderBufferEntry& annulrob = ROB[idx];
    //    issueq_operation_on_cluster(core, annulrob.cluster, annuluop(annulrob.get_tag()));
    bool rc;
    issueq_operation_on_cluster_with_result(core, annulrob.cluster, rc, annuluop(annulrob.get_tag()));  
    if (rc) {
      if unlikely (core.threadcount > 1) {
        if (thread.issueq_count > core.reserved_iq_entries) {
          issueq_operation_on_cluster(core, cluster, free_shared_entry());
        }
      }
      thread.issueq_count--;
    }

    annulrob.iqslot = -1;
    if unlikely (idx == startidx) break;
    idx = add_index_modulo(idx, -1, ROB_SIZE);
  }

  int annulcount = 0;

  //
  // Pass 2: reconstruct the SpecRRT as it existed just before (or after)
  // the mis-speculated operation. This is done using the fast flush with
  // pseudo-commit method as follows:
  //
  // First overwrite the SpecRRT with the CommitRRT.
  //
  // Then, simulate the commit of all non-speculative ROBs up to the branch
  // by updating the SpecRRT as if it were the CommitRRT. This brings the
  // speculative RRT to the same state as if all in flight nonspeculative
  // operations before the branch had actually committed. Resume instruction 
  // fetch at the correct branch target.
  //
  // Other methods (like backwards walk) are difficult to impossible because
  // of the requirement that flag rename tables be restored even if some
  // of the required physical registers with attached flags have since been
  // freed. Therefore we don't do this.
  //
  // Technically RRT checkpointing could be used but due to the load/store
  // replay mechanism in use, this would require a checkpoint at every load
  // and store as well as branches.
  //
  foreach (i, TRANSREG_COUNT) { specrrt[i]->unspecref(i, thread.threadid); }
  specrrt = commitrrt;
  foreach (i, TRANSREG_COUNT) { specrrt[i]->addspecref(i, thread.threadid); }

  // if (logable(6)) logfile << "Restored SpecRRT from CommitRRT; walking forward from:", endl, core.specrrt, endl;
  idx = ROB.head;
  for (idx = ROB.head; idx != startidx; idx = add_index_modulo(idx, +1, ROB_SIZE)) {
    ReorderBufferEntry& rob = ROB[idx];
    rob.pseudocommit();
  }

  // if (logable(6)) logfile << "Recovered SpecRRT:", endl, core.specrrt, endl;

  //
  // Pass 3: For each speculative ROB, reinitialize and free speculative ROBs
  //

  ReorderBufferEntry* lastrob = null;

  idx = endidx;
  for (;;) {
    ReorderBufferEntry& annulrob = ROB[idx];

    lastrob = &annulrob;

    if unlikely (config.event_log_enabled) {
      event = core.eventlog.add(EVENT_ANNUL_EACH_ROB, &annulrob);
      event->annul.annulras = 0;
    }

    //
    // Free the speculatively allocated physical register
    // See notes above on Physical Register Recycling Complications
    //
    foreach (j, MAX_OPERANDS) { annulrob.operands[j]->unref(annulrob, thread.threadid); }
    annulrob.physreg->free();

    if unlikely (isclass(annulrob.uop.opcode, OPCLASS_LOAD|OPCLASS_STORE)) {
      //
      // We have to be careful to not flush any locks that are about to be
      // freed by a committing locked RMW instruction that takes more than
      // one cycle to commit but has already declared the locks it wants
      // to release.
      //
      // There are a few things we can do here: only flush if the annulrob
      // actually held locks, and then only flush those locks (actually only
      // a single lock) added here!
      //
      if (annulrob.release_mem_lock(true)) thread.flush_mem_lock_release_list(queued_locks_before);
      loads_in_flight -= (annulrob.lsq->store == 0);
      stores_in_flight -= (annulrob.lsq->store == 1);
      annulrob.lsq->reset();
      LSQ.annul(annulrob.lsq);
    }

    if unlikely (annulrob.lfrqslot >= 0) {
      core.caches.annul_lfrq_slot(annulrob.lfrqslot);
    }

    if unlikely (isbranch(annulrob.uop.opcode) && (annulrob.uop.predinfo.bptype & (BRANCH_HINT_CALL|BRANCH_HINT_RET))) {
      //
      // Return Address Stack (RAS) correction:
      // Example calls and returns in pipeline
      //
      // C1
      //   C2
      //   R2 
      //   BR (mispredicted branch)
      //   C3
      //     C4
      //
      // BR mispredicts, so everything after BR must be annulled.
      // RAS contains: C1 C3 C4, so we need to annul [C4 C3].
      //
      if unlikely (config.event_log_enabled) event->annul.annulras = 1;
      branchpred.annulras(annulrob.uop.predinfo);
    }

    annulrob.reset();

    ROB.annul(annulrob);
    annulrob.changestate(thread.rob_free_list);
    annulcount++;

    if (idx == startidx) break;
    idx = add_index_modulo(idx, -1, ROB_SIZE);
  }

  assert(ROB[startidx].uop.som);
  if (return_first_annulled_rip) return ROB[startidx].uop.rip;
  return (keep_misspec_uop) ? ROB[startidx].uop.riptaken : (Waddr)ROB[startidx].uop.rip;
}

//
// Return the specified uop back to the ready_to_dispatch state.
// All structures allocated to the uop are reset to the same state
// they had immediately after allocation.
//
// This function is used to handle various types of mis-speculations
// in which only the values are invalid, rather than the actual uops
// as with branch mispredicts and unaligned accesses. It is also
// useful for various kinds of value speculation.
//
// The normal "fast" replay mechanism is still used for scheduler
// related replays - this is much more expensive.
//
// If this function is called for a given uop U, all of U's
// consumers must also be re-dispatched. The redispatch_dependents()
// function automatically does this.
//
// The <prevrob> argument should be the previous ROB, in program
// order, before this one. If this is the first ROB being
// re-dispatched, <prevrob> should be null.
//

void ReorderBufferEntry::redispatch(const bitvec<MAX_OPERANDS>& dependent_operands, ReorderBufferEntry* prevrob) {
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();

  if(issued){
    issued = 0;
  }
  OutOfOrderCoreEvent* event;

  if unlikely (config.event_log_enabled) {
    event = core.eventlog.add(EVENT_REDISPATCH_EACH_ROB, this);
    event->redispatch.current_state_list = current_state_list;
    event->redispatch.dependent_operands = dependent_operands.integer();
    foreach (i, MAX_OPERANDS) operands[i]->fill_operand_info(event->redispatch.opinfo[i]);
  }

  per_context_ooocore_stats_update(threadid, dispatch.redispatch.trigger_uops++);

  // Remove from issue queue, if it was already in some issue queue
  if unlikely (cluster >= 0) {
    bool found = 0;
    issueq_operation_on_cluster_with_result(getcore(), cluster, found, annuluop(get_tag()));
    if (found) {
      if unlikely (core.threadcount > 1) {
        if (thread.issueq_count > core.reserved_iq_entries) {
          issueq_operation_on_cluster(core, cluster, free_shared_entry());
        }
      }
      thread.issueq_count--;
    }
    if unlikely (config.event_log_enabled) event->redispatch.iqslot = found;
    cluster = -1;
  }

  if unlikely (lfrqslot >= 0) {
    core.caches.annul_lfrq_slot(lfrqslot);
    lfrqslot = -1;
  }

  release_mem_lock(true);
  thread.flush_mem_lock_release_list();

  if unlikely (lsq) {
    lsq->physaddr = 0;
    lsq->addrvalid = 0;
    lsq->datavalid = 0;
    lsq->mbtag = -1;
    lsq->data = 0;
    lsq->physaddr = 0;
    lsq->invalid = 0;

    if (operands[RS]->nonnull()) {
      operands[RS]->unref(*this, thread.threadid);
      operands[RS] = &core.physregfiles[0][PHYS_REG_NULL];
      operands[RS]->addref(*this, thread.threadid);
    }
  }

  // Return physreg to state just after allocation
  physreg->data = 0;
  physreg->flags = FLAG_WAIT;
  physreg->changestate(PHYSREG_WAITING);

  // Force ROB to be re-dispatched in program order
  cycles_left = 0;
  forward_cycle = 0;
  load_store_second_phase = 0;
  changestate(thread.rob_ready_to_dispatch_list, true, prevrob);
}

//
// Find all uops dependent on the specified uop, and 
// redispatch each of them.
//
void ReorderBufferEntry::redispatch_dependents(bool inclusive) {
  OutOfOrderCore& core = getcore();
  ThreadContext& thread = getthread();
  Queue<ReorderBufferEntry, ROB_SIZE>& ROB = thread.ROB;

  bitvec<ROB_SIZE> depmap;
  depmap = 0;
  depmap[index()] = 1;

  OutOfOrderCoreEvent* event;
  if unlikely (config.event_log_enabled) event = core.eventlog.add(EVENT_REDISPATCH_DEPENDENTS, this);

  //
  // Go through the ROB and identify the slice of all uops
  // depending on this one, through the use of physical
  // registers as operands.
  //
  int count = 0;

  ReorderBufferEntry* prevrob = null;

  foreach_forward_from(ROB, this, robidx) {
    ReorderBufferEntry& reissuerob = ROB[robidx];

    if (!inclusive) {
      depmap[reissuerob.index()] = 1;
      continue;
    }

    bitvec<MAX_OPERANDS> dependent_operands;
    dependent_operands = 0;

    foreach (i, MAX_OPERANDS) {
      const PhysicalRegister* operand = reissuerob.operands[i];
      dependent_operands[i] = (operand->rob && depmap[operand->rob->index()]);
    }

    //
    // We must also redispatch all stores, since in pathological cases, there may
    // be store-store ordering cases we don't know about, i.e. if some store
    // inherits from a previous store, but that previous store actually has the
    // wrong address because of some other bogus uop providing its address.
    //
    // In addition, ld.acq and st.rel create additional complexity: we can never
    // re-dispatch the ld.acq but not the st.rel and vice versa; both must be
    // redispatched together.
    //
    bool dep = (*dependent_operands) | (robidx == index()) | isstore(uop.opcode);

    if unlikely (dep) {
      count++;
      depmap[reissuerob.index()] = 1;
      reissuerob.redispatch(dependent_operands, prevrob);
      prevrob = &reissuerob;
    }
  }

  assert(inrange(count, 1, ROB_SIZE));
  per_context_ooocore_stats_update(threadid, dispatch.redispatch.dependent_uops[count-1]++);

  if unlikely (config.event_log_enabled) {
    event = core.eventlog.add(EVENT_REDISPATCH_DEPENDENTS_DONE, this);
    event->redispatch.count = count;
  }
}

int ReorderBufferEntry::pseudocommit() {
  OutOfOrderCore& core = getcore();

  ThreadContext& thread = getthread();
  RegisterRenameTable& specrrt = thread.specrrt;
  RegisterRenameTable& commitrrt = thread.commitrrt;

  if unlikely (config.event_log_enabled) core.eventlog.add(EVENT_ANNUL_PSEUDOCOMMIT, this);

  if likely (archdest_can_commit[uop.rd]) {
    specrrt[uop.rd]->unspecref(uop.rd, thread.threadid);
    specrrt[uop.rd] = physreg;
    specrrt[uop.rd]->addspecref(uop.rd, thread.threadid);
  }

  if likely (!uop.nouserflags) {
    if (uop.setflags & SETFLAG_ZF) {
      specrrt[REG_zf]->unspecref(REG_zf, thread.threadid);
      specrrt[REG_zf] = physreg;
      specrrt[REG_zf]->addspecref(REG_zf, thread.threadid);
    }
    if (uop.setflags & SETFLAG_CF) {
      specrrt[REG_cf]->unspecref(REG_cf, thread.threadid);
      specrrt[REG_cf] = physreg;
      specrrt[REG_cf]->addspecref(REG_cf, thread.threadid);
    }
    if (uop.setflags & SETFLAG_OF) {
      specrrt[REG_of]->unspecref(REG_of, thread.threadid);
      specrrt[REG_of] = physreg;
      specrrt[REG_of]->addspecref(REG_of, thread.threadid);
    }
  }

  if unlikely (isclass(uop.opcode, OPCLASS_BARRIER))
                return COMMIT_RESULT_BARRIER;

  return COMMIT_RESULT_OK;
}
