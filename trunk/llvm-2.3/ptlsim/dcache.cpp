//
// PTLsim: Cycle Accurate x86-64 Simulator
// L1 and L2 Data Caches
//
// Copyright 2005-2008 Matt T. Yourst <yourst@yourst.com>
//

#include <dcache.h>
#include <stats.h>

using namespace CacheSubsystem;

#if 0
#define starttimer(timer) timer.start()
#define stoptimer(timer) timer.stop()
#else
#define starttimer(timer) (1)
#define stoptimer(timer) (1)
#endif

#ifdef TRACK_LINE_USAGE
// Lifetime
template <> W64 L1StatsCollectorBase::line_lifetime_histogram[DCACHE_L1_LINE_LIFETIME_SLOTS] = {};
template <> W64 L1IStatsCollectorBase::line_lifetime_histogram[DCACHE_L1I_LINE_LIFETIME_SLOTS] = {};
template <> W64 L2StatsCollectorBase::line_lifetime_histogram[DCACHE_L2_LINE_LIFETIME_SLOTS] = {};
template <> W64 L3StatsCollectorBase::line_lifetime_histogram[DCACHE_L3_LINE_LIFETIME_SLOTS] = {};

// Deadtime
template <> W64 L1StatsCollectorBase::line_deadtime_histogram[DCACHE_L1_LINE_DEADTIME_SLOTS] = {};
template <> W64 L1IStatsCollectorBase::line_deadtime_histogram[DCACHE_L1I_LINE_DEADTIME_SLOTS] = {};
template <> W64 L2StatsCollectorBase::line_deadtime_histogram[DCACHE_L2_LINE_DEADTIME_SLOTS] = {};
template <> W64 L3StatsCollectorBase::line_deadtime_histogram[DCACHE_L3_LINE_DEADTIME_SLOTS] = {};

// Hit count
template <> W64 L1StatsCollectorBase::line_hitcount_histogram[DCACHE_L1_LINE_HITCOUNT_SLOTS] = {};
template <> W64 L1IStatsCollectorBase::line_hitcount_histogram[DCACHE_L1I_LINE_HITCOUNT_SLOTS] = {};
template <> W64 L2StatsCollectorBase::line_hitcount_histogram[DCACHE_L2_LINE_HITCOUNT_SLOTS] = {};
template <> W64 L3StatsCollectorBase::line_hitcount_histogram[DCACHE_L3_LINE_HITCOUNT_SLOTS] = {};
#endif

//
// Load Fill Request Queue
//

template <int size>
void LoadFillReqQueue<size>::restart() {
  while (!freemap.allset()) {
    int idx = (~freemap).lsb();
    LoadFillReq& req = reqs[idx];
    if (logable(6)) logfile << "iter ", iterations, ": force final wakeup/reset of LFRQ slot ", idx, ": ", req, endl;
    annul(idx);
  }
  reset();
  stats.dcache.lfrq.resets++;
}

template <int size>
void LoadFillReqQueue<size>::reset(int threadid) {
  foreach (i, SIZE) {
    LoadFillReq& req = reqs[i];
    if likely ((!freemap[i]) && (req.lsi.threadid == threadid)) {
      if (logable(6)) logfile << "[vcpu ", threadid, "] reset lfrq slot ", i, ": ", req, endl;
      waiting[i] = 0;
      ready[i] = 0;
      freemap[i] = 1;
      count--;
      assert(count >= 0);
    }
  }

  stats.dcache.lfrq.resets++;
}

template <int size>
void LoadFillReqQueue<size>::annul(int lfrqslot) {
  LoadFillReq& req = reqs[lfrqslot];
  if (logable(6)) logfile << "  Annul LFRQ slot ", lfrqslot, endl;
  stats.dcache.lfrq.annuls++;
  hierarchy.missbuf.annul_lfrq(lfrqslot);
  reqs[lfrqslot].mbidx = -1;
  assert(!freemap[lfrqslot]);
  changestate(lfrqslot, ready, freemap);
  count--;
  assert(count >= 0);
}

//
// Add an entry to the LFRQ in the waiting state.
//
template <int size>
int LoadFillReqQueue<size>::add(const LoadFillReq& req) {
  if unlikely (full()) return -1;
#if 0
  // Sanity check: make sure (tid, rob) is unique:
  foreach (i, size) {
    if likely (freemap[i]) continue;
    const LoadFillReq& old = reqs[i];
    if ((old.lsi.threadid == req.lsi.threadid) && (old.lsi.rob == req.lsi.rob)) {
      logfile << "ERROR: during add LFRQ req ", req, ", entry ", i, " (", old, ") already matches at cycle ", sim_cycle, endl;
      logfile << *this;
      logfile << hierarchy.missbuf;
      // assert(false);
    }
  }
#endif
  int idx = freemap.lsb();
  changestate(idx, freemap, waiting);         
  reqs[idx] = req;
  assert(count < size);
  count++;
  stats.dcache.lfrq.inserts++;
  return idx;
}

//
// Move any LFRQ entries in <mask> to the ready state
// in response to the arrival of the corresponding
// line at the L1 level. Once a line is delivered,
// it is copied into the L1 cache and the corresponding
// miss buffer can be freed.
// 
template <int size>
void LoadFillReqQueue<size>::wakeup(W64 address, const bitvec<LFRQ_SIZE>& lfrqmask) {
  if (logable(6)) logfile << "LFRQ.wakeup(", (void*)(Waddr)address, ", ", lfrqmask, ")", endl;
  //assert(L2.probe(address));
  waiting &= ~lfrqmask;
  ready |= lfrqmask;
}

//
// Find the first N requests (N = 2) in the READY state,
// and extract, sign extend and write into their target
// register, then mark that register as ready.
//
// Also mark the entire cache line containing each load
// as fully valid.
//
// Loads will always be allocated a physical register
// since if the load misses the L1, it will have fallen
// off the end of the pipeline and into the register file
// by the earliest time we can receive the data from the
// L2 cache and/or lower levels.
//
template <int size>
void LoadFillReqQueue<size>::clock() {
  //
  // Process up to MAX_WAKEUPS_PER_CYCLE missed loads per cycle:
  //
  int wakeupcount = 0;
  foreach (i, MAX_WAKEUPS_PER_CYCLE) {
    if unlikely (!ready) break;

    int idx = ready.lsb();
    LoadFillReq& req = reqs[idx];
    
    if (logable(6)) logfile << "[vcpu ", req.lsi.threadid, "] at cycle ", sim_cycle, ": wakeup LFRQ slot ", idx, ": ", req, endl;

    W64 delta = LO32(sim_cycle) - LO32(req.initcycle);
    if unlikely (delta >= 65536) {
      // avoid overflow induced erroneous values:
      // logfile << "LFRQ: warning: cycle counter wraparound in initcycle latency (current ", sim_cycle, " vs init ", req.initcycle, " = delta ", delta, ")", endl;
    } else {
      stats.dcache.lfrq.total_latency += delta;
    }
        
    stats.dcache.lfrq.wakeups++;
    wakeupcount++;
    if likely (hierarchy.callback) hierarchy.callback->dcache_wakeup(req.lsi, req.addr);

    assert(!freemap[idx]);
    changestate(idx, ready, freemap);
    count--;
    assert(count >= 0);
  }

  stats.dcache.lfrq.width[wakeupcount]++;
}

LoadFillReq::LoadFillReq(W64 addr, W64 data, byte mask, LoadStoreInfo lsi) {
  this->addr = addr;
  this->data = data;
  this->mask = mask;
  this->lsi = lsi;
  this->lsi.threadid = lsi.threadid; 
  this->fillL1 = 1;
  this->fillL2 = 1;
  this->initcycle = sim_cycle;
  this->mbidx = -1;
}

ostream& LoadFillReq::print(ostream& os) const {
  os << "0x", hexstring(data, 64), " @ ", (void*)(Waddr)addr, " -> rob ", lsi.rob, " @ t", lsi.threadid;
  os << ": shift ", lsi.sizeshift, ", signext ", lsi.signext, ", mask ", bitstring(mask, 8, true);
  return os;
}

template <int size>
ostream& LoadFillReqQueue<size>::print(ostream& os) const {
  os << "LoadFillReqQueue<", size, ">: ", count, " of ", size, " entries (", (size - count), " free)", endl;
  os << "  Free:   ", freemap, endl;
  os << "  Wait:   ", waiting, endl;
  os << "  Ready:  ", ready, endl;
  foreach (i, size) {
    if (!bit(freemap, i)) {
      os << "  slot ", intstring(i, 2), ": ", reqs[i], endl;
    }
  }
  return os;
}

//
// Miss Buffer
//

template <int SIZE>    
void MissBuffer<SIZE>::reset() {
  foreach (i, SIZE) {
    missbufs[i].reset();
  }
  freemap.setall();
  count = 0;
}


template <int SIZE>    
void MissBuffer<SIZE>::reset(int threadid) {
  foreach (i, SIZE) {
    Entry& mb = missbufs[i];
    if likely (mb.threadid == threadid) {
      if (logable(6)) logfile << "[vcpu ", threadid, "] reset missbuf slot ", i, ": for rob", mb.rob, endl;
      assert(!freemap[i]);
      mb.reset();
      freemap[i] = 1;
      count--;
      assert(count >= 0);

      //
      // If multiple threads depend on the same missbuf but one thread is
      // flushed, we'll wake up a stale LFRQ. We have to make sure after
      // a missbuf reset, all the entries point to a valid lfrqmap.
      //
      if (*mb.lfrqmap) {
        bitvec<LFRQ_SIZE> tmp_lfrqmap = mb.lfrqmap ^ hierarchy.lfrq.waiting;
        if (*tmp_lfrqmap) {
          if (logable(6)) logfile << "Multithread share same missbufs[", i, "] : its lfrqmap is ", mb.lfrqmap, " LFRQ waiting map ", hierarchy.lfrq.waiting, ", diff: ", tmp_lfrqmap, endl;
          mb.lfrqmap &= ~tmp_lfrqmap;
          if (logable(6)) logfile << "after remove stale lfrq entries, its lfrqmap is ", mb.lfrqmap, endl;
        }
      }
    }
  }
}

template <int SIZE>    
void MissBuffer<SIZE>::restart() {
  if likely (!(freemap.allset())) {
    foreach (i, SIZE) {
      missbufs[i].lfrqmap = 0;
    }
  }
}

template <int SIZE>    
int MissBuffer<SIZE>::find(W64 addr) {
  W64 match = 0;
  foreach (i, SIZE) {
    if ((missbufs[i].addr == addr) && !freemap[i]) return i;
  }
  return -1;
}

//
// Request fully or partially missed both the L2 and L1
// caches and needs service from below.
//
template <int SIZE>
int MissBuffer<SIZE>::initiate_miss(W64 addr, bool hit_in_L2, bool icache, int rob, int threadid) {
  bool DEBUG = logable(6);

  addr = floor(addr, L1_LINE_SIZE);

  int idx = find(addr);

  // if unlikely (idx >= 0 && threadid == missbufs[idx].threadid) {
  if unlikely (idx >= 0) {
    // Handle case where dcache miss is already in progress but some 
    // code needed in icache is also stored in that line:
    Entry& mb = missbufs[idx];
    mb.icache |= icache;
    mb.dcache |= (!icache);
    // Handle case where icache miss is already in progress but some
    // data needed in dcache is also stored in that line:
    if (DEBUG) logfile << "[vcpu ", threadid, "] miss buffer hit for address ", (void*)(Waddr)addr, ": returning old slot ", idx, endl;
    return idx;
  }

  if unlikely (full()) {
    if (DEBUG) logfile << "[vcpu ", threadid, "] miss buffer full while allocating slot for address ", (void*)(Waddr)addr, endl;
    return -1;
  }

  idx = freemap.lsb();
  freemap[idx] = 0;
  assert(count < SIZE);
  count++;

  stats.dcache.missbuf.inserts++;
  Entry& mb = missbufs[idx];
  mb.addr = addr;
  mb.lfrqmap = 0;
  mb.icache = icache;
  mb.dcache = (!icache);
  mb.rob = rob;
  mb.threadid = threadid;
 
  if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", idx, ": allocated for address ", (void*)(Waddr)addr, " (iter ", iterations, ")", endl;

  if likely (hit_in_L2) {
    if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", idx, ": enter state deliver to L1 on ", (void*)(Waddr)addr, " (iter ", iterations, ")", endl;
    mb.state = STATE_DELIVER_TO_L1;
    mb.cycles = L2_LATENCY;

    if unlikely (icache) per_context_dcache_stats_update(mb.threadid, fetch.hit.L2++); else per_context_dcache_stats_update(mb.threadid, load.hit.L2++);
    return idx;
  }
#ifdef ENABLE_L3_CACHE
  bool L3hit = hierarchy.L3.probe(addr);
  if likely (L3hit) {
    if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", idx, ": enter state deliver to L2 on ", (void*)(Waddr)addr, " (iter ", iterations, ")", endl;
    mb.state = STATE_DELIVER_TO_L2;
    mb.cycles = L3_LATENCY;
    if (icache) per_context_dcache_stats_update(mb.threadid, fetch.hit.L3++); else per_context_dcache_stats_update(mb.threadid, load.hit.L3++);
    return idx;
  }

  if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", idx, ": enter state deliver to L3 on ", (void*)(Waddr)addr, " (iter ", iterations, ")", endl;
  mb.state = STATE_DELIVER_TO_L3;
  mb.cycles = MAIN_MEM_LATENCY;
#else
  // L3 cache disabled
  if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", idx, ": enter state deliver to L2 on ", (void*)(Waddr)addr, " (iter ", iterations, ")", endl;
  mb.state = STATE_DELIVER_TO_L2;
  mb.cycles = MAIN_MEM_LATENCY;
#endif
  if unlikely (icache) per_context_dcache_stats_update(mb.threadid, fetch.hit.mem++); else per_context_dcache_stats_update(mb.threadid, load.hit.mem++);

  return idx;
}

template <int SIZE>
int MissBuffer<SIZE>::initiate_miss(LoadFillReq& req, bool hit_in_L2, int rob) {
  int lfrqslot = hierarchy.lfrq.add(req);

  if (logable(6)) logfile << "[vcpu ", req.lsi.threadid, "] missbuf.initiate_miss(req ", req, ", L2hit? ", hit_in_L2, ") -> lfrqslot ", lfrqslot, endl;

  if unlikely (lfrqslot < 0) return -1;
  
  int mbidx = initiate_miss(req.addr, hit_in_L2, 0, rob, req.lsi.threadid);
  if unlikely (mbidx < 0) {
    hierarchy.lfrq.free(lfrqslot);
    return -1;
  }

  Entry& missbuf = missbufs[mbidx];
  missbuf.lfrqmap[lfrqslot] = 1;
  hierarchy.lfrq[lfrqslot].mbidx = mbidx;
  // missbuf.threadid = req.lsi.threadid;

  return lfrqslot;
}

template <int SIZE>
void MissBuffer<SIZE>::clock() {
  if likely (freemap.allset()) return;

  bool DEBUG = logable(6);

  foreach (i, SIZE) {
    Entry& mb = missbufs[i];
    switch (mb.state) {
    case STATE_IDLE:
      break;
#ifdef ENABLE_L3_CACHE
    case STATE_DELIVER_TO_L3: {
      if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", i, ": deliver ", (void*)(Waddr)mb.addr, " to L3 (", mb.cycles, " cycles left) (iter ", iterations, ")", endl;
      mb.cycles--;
      if unlikely (!mb.cycles) {
        hierarchy.L3.validate(mb.addr);
        mb.cycles = L3_LATENCY;
        mb.state = STATE_DELIVER_TO_L2;
        stats.dcache.missbuf.deliver.mem_to_L3++;
      }
      break;
    }
#endif
    case STATE_DELIVER_TO_L2: {
      if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", i, ": deliver ", (void*)(Waddr)mb.addr, " to L2 (", mb.cycles, " cycles left) (iter ", iterations, ")", endl;
      mb.cycles--;
      if unlikely (!mb.cycles) {
        if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", i, ": delivered to L2 (map ", mb.lfrqmap, ")", endl;
        hierarchy.L2.validate(mb.addr);
        mb.cycles = L2_LATENCY;
        mb.state = STATE_DELIVER_TO_L1;
        stats.dcache.missbuf.deliver.L3_to_L2++;
      }
      break;
    }
    case STATE_DELIVER_TO_L1: {
      if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", i, ": deliver ", (void*)(Waddr)mb.addr, " to L1 (", mb.cycles, " cycles left) (iter ", iterations, ")", endl;
      mb.cycles--;
      if unlikely (!mb.cycles) {
        if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", i, ": delivered to L1 switch (map ", mb.lfrqmap, ")", endl;

        if likely (mb.dcache) {
          if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", i, ": delivered ", (void*)(Waddr)mb.addr, " to L1 dcache (map ", mb.lfrqmap, ")", endl;
          // If the L2 line size is bigger than the L1 line size, this will validate multiple lines in the L1 when an L2 line arrives:
          // foreach (i, L2_LINE_SIZE / L1_LINE_SIZE) L1.validate(mb.addr + i*L1_LINE_SIZE, bitvec<L1_LINE_SIZE>().setall());
          hierarchy.L1.validate(mb.addr, bitvec<L1_LINE_SIZE>().setall());
          stats.dcache.missbuf.deliver.L2_to_L1D++;
          hierarchy.lfrq.wakeup(mb.addr, mb.lfrqmap);
        }
        if unlikely (mb.icache) {
          // Sometimes we can initiate an icache miss on an existing dcache line in the missbuf
          if (DEBUG) logfile << "[vcpu ", mb.threadid, "] mb", i, ": delivered ", (void*)(Waddr)mb.addr, " to L1 icache", endl;
          // If the L2 line size is bigger than the L1 line size, this will validate multiple lines in the L1 when an L2 line arrives:
          // foreach (i, L2_LINE_SIZE / L1I_LINE_SIZE) L1I.validate(mb.addr + i*L1I_LINE_SIZE, bitvec<L1I_LINE_SIZE>().setall());
          hierarchy.L1I.validate(mb.addr, bitvec<L1I_LINE_SIZE>().setall());
          stats.dcache.missbuf.deliver.L2_to_L1I++;
          LoadStoreInfo lsi = 0;
          lsi.rob = mb.rob;
          lsi.threadid = mb.threadid;
          if likely (hierarchy.callback) hierarchy.callback->icache_wakeup(lsi, mb.addr);
        }

        assert(!freemap[i]);
        freemap[i] = 1;
        mb.reset();
        count--;
        assert(count >= 0);
      }
      break;
    }
    }
  }
}

template <int SIZE>
void MissBuffer<SIZE>::annul_lfrq(int slot) {
  foreach (i, SIZE) {
    Entry& mb = missbufs[i];
    mb.lfrqmap[slot] = 0;  // which LFRQ entries should this load wake up?
  }
}

template <int SIZE>
ostream& MissBuffer<SIZE>::print(ostream& os) const {
 
  os << "MissBuffer<", SIZE, ">:", endl;
  foreach (i, SIZE) {
    if likely (freemap[i]) continue;
    const Entry& mb = missbufs[i];
    os << "slot ", intstring(i, 2), ": vcpu ", mb.threadid, ", addr ", (void*)(Waddr)mb.addr, " state ", 
      padstring(missbuf_state_names[mb.state], -8), " ", (mb.dcache ? "dcache" : "      "),
      " ", (mb.icache ? "icache" : "      "), " on ", mb.cycles, " cycles -> lfrq ", mb.lfrqmap, endl;
  }
  return os;
}

template <int linesize>
ostream& CacheLine<linesize>::print(ostream& os, W64 tag) const {
#if 0
  const byte* data = (const byte*)(W64)tag;
  foreach (i, linesize/8) {
    os << "    ", bytemaskstring(data + i*8, (W64)-1LL, 8, 8), " ";
    os << endl;
  }
#endif
  return os;
}

template <int linesize>
ostream& CacheLineWithValidMask<linesize>::print(ostream& os, W64 tag) const {
#if 0
  const byte* data = (const byte*)(W64)tag;
  foreach (i, linesize/8) {
    os << "    ", bytemaskstring(data + i*8, valid(i*8, 8).integer(), 8, 8), " ";
    os << endl;
  }
#endif
  return os;
}

int CacheHierarchy::issueload_slowpath(Waddr physaddr, SFR& sfra, LoadStoreInfo lsi, bool& L2hit) {
  static const bool DEBUG = 0;

  starttimer(load_slowpath_timer);

  L1CacheLine* L1line = L1.probe(physaddr);

  //
  // Loads and stores that also miss the L2 Stores that
  // miss both the L1 and L2 do not require this since
  // there could not possibly be a previous load or 
  // store within the current trace that accessed that
  // line (otherwise it would already have been allocated
  // and locked in the L2). In this case, allocate a
  // fresh L2 line and wait for the data to arrive.
  //

  if (DEBUG) {
    logfile << "issue_load_slowpath: L1line for ", (void*)(Waddr)physaddr, " = ", L1line, " validmask ";
    if (L1line) logfile << L1line->valid; else logfile << "(none)";
    logfile << endl;
  }

  if likely (!L1line) {
    //L1line = L1.select(physaddr);
    stats.dcache.load.transfer.L2_to_L1_full++;
  } else {
    stats.dcache.load.transfer.L2_to_L1_partial++;
  }

  L2hit = 0;
    
  L2CacheLine* L2line = L2.probe(physaddr);

  if likely (L2line) {
    //
    // We had at least a partial L2 hit, but is the requested data actually mapped into the line?
    //
    bitvec<L2_LINE_SIZE> sframask, reqmask;
    prep_L2_sframask_and_reqmask((lsi.sfrused) ? &sfra : null, physaddr, lsi.sizeshift, sframask, reqmask);
    L2hit = (lsi.sfrused) ? ((reqmask & (sframask | L2line->valid)) == reqmask) : ((reqmask & L2line->valid) == reqmask);
#ifdef ISSUE_LOAD_STORE_DEBUG
    logfile << "L2hit = ", L2hit, endl, "  cachemask ", L2line->valid, endl,
      "  sframask  ", sframask, endl, "  reqmask   ", reqmask, endl;
#endif
  }

#ifdef CACHE_ALWAYS_HITS
  L1line = L1.select(physaddr);
  L1line->tag = L1.tagof(physaddr);
  L1line->valid.setall();
  L2line->tag = L2.tagof(physaddr);
  L2line->valid.setall();
  L2hit = 1;
#endif

#ifdef L2_ALWAYS_HITS
  L2line = L2.select(physaddr);
  L2line->tag = L2.tagof(physaddr);
  L2line->valid.setall();
  L2line->lru = sim_cycle;
  L2hit = 1;
#endif

  //
  // Regardless of whether or not we had a hit somewhere,
  // L1 and L2 lines have been allocated by this point.
  // Slap a lock on the L2 line it so it can't get evicted.
  // Once it's locked up, we can move it into the L1 later.
  //
  // If we did have a hit, but either the L1 or L2 lines
  // were still missing bytes, initiate prefetches to fill
  // them in.
  //

  LoadFillReq req(physaddr, lsi.sfrused ? sfra.data : 0, lsi.sfrused ? sfra.bytemask : 0, lsi);

  int lfrqslot = missbuf.initiate_miss(req, L2hit, lsi.rob);

  if unlikely (lfrqslot < 0) {
    if (DEBUG) logfile << "iteration ", iterations, ": LFRQ or MB has no free entries for L2->L1: forcing LFRQFull exception", endl;
    stoptimer(load_slowpath_timer);
    return -1;
  }

  stoptimer(load_slowpath_timer);

  return lfrqslot;
}

int CacheHierarchy::get_lfrq_mb(int lfrqslot) const {
  assert(inrange(lfrqslot, 0, LFRQ_SIZE-1));

  const LoadFillReq& req = lfrq.reqs[lfrqslot];
  return req.mbidx;
}

int CacheHierarchy::get_lfrq_mb_state(int lfrqslot) const {
  assert(inrange(lfrqslot, 0, LFRQ_SIZE-1));

  const LoadFillReq& req = lfrq.reqs[lfrqslot];
  if unlikely (req.mbidx < 0) return -1;

  assert(!missbuf.freemap[req.mbidx]);
  return missbuf.missbufs[req.mbidx].state;
}

bool CacheHierarchy::covered_by_sfr(W64 addr, SFR* sfr, int sizeshift) {
  bitvec<L1_LINE_SIZE> sframask, reqmask;
  prep_sframask_and_reqmask(sfr, addr, sizeshift, sframask, reqmask);
  return ((sframask & reqmask) == reqmask);
}

bool CacheHierarchy::probe_cache_and_sfr(W64 addr, const SFR* sfr, int sizeshift) {
  bitvec<L1_LINE_SIZE> sframask, reqmask;
  prep_sframask_and_reqmask(sfr, addr, sizeshift, sframask, reqmask);

  //
  // Short circuit if the SFR covers the entire load: no need for cache probe
  //
  if unlikely ((sframask & reqmask) == reqmask) return true;

  L1CacheLine* L1line = L1.probe(addr);

  if unlikely (!L1line) return false;

  //
  // We have a hit on the L1 line itself, but still need to make
  // sure all the data can be filled by some combination of
  // bytes from sfra or the cache data.
  //
  // If not, put this request on the LFRQ and mark it as waiting.
  //

  bool hit = ((reqmask & (sframask | L1line->valid)) == reqmask);

  return hit;
}

void CacheHierarchy::annul_lfrq_slot(int lfrqslot) {
  lfrq.annul(lfrqslot);
}
  
//
// NOTE: lsi should specify destination of REG_null for prefetches!
//
static const int PREFETCH_STOPS_AT_L2 = 0;
  
void CacheHierarchy::initiate_prefetch(W64 addr, int cachelevel) {
  static const bool DEBUG = 0;

  addr = floor(addr, L1_LINE_SIZE);
    
  L1CacheLine* L1line = L1.probe(addr);
    
  if unlikely (L1line) {
    stats.dcache.prefetch.in_L1++;
    return;
  }
    
  L2CacheLine* L2line = L2.probe(addr);
    
  if unlikely (L2line) {
    stats.dcache.prefetch.in_L2++;
    if (PREFETCH_STOPS_AT_L2) return; // only move up to L2 level, and it's already there
  }
    
  if (DEBUG) logfile << "Prefetch requested for ", (void*)(Waddr)addr, " to cache level ", cachelevel, endl;
    
  missbuf.initiate_miss(addr, L2line);
  stats.dcache.prefetch.required++;
}

//
// Instruction cache
//

bool CacheHierarchy::probe_icache(Waddr virtaddr, Waddr physaddr) {
  L1ICacheLine* L1line = L1I.probe(physaddr);
  bool hit = (L1line != null);
    
  return hit;
}

int CacheHierarchy::initiate_icache_miss(W64 addr, int rob, int threadid) {
  addr = floor(addr, L1I_LINE_SIZE);
  bool line_in_L2 = (L2.probe(addr) != null);
  int mb = missbuf.initiate_miss(addr, L2.probe(addr), true, rob, threadid);
    
  if (logable(6))
    logfile << "[vcpu ", threadid, "] Initiate icache miss on ", (void*)(Waddr)addr, " to missbuf ", mb, " (", (line_in_L2 ? "in L2" : "not in L2"), ")", endl;
    
  return mb;
}

//
// Commit one store from an SFR to the L2 cache without locking
// any cache lines. The store must have already been checked
// to have no exceptions.
//
W64 CacheHierarchy::commitstore(const SFR& sfr, int threadid, bool perform_actual_write) {
  if unlikely (sfr.invalid | (sfr.bytemask == 0)) return 0;

  static const bool DEBUG = 0;

  starttimer(store_flush_timer);

  W64 addr = sfr.physaddr << 3;

  L2CacheLine* L2line = L2.select(addr);

  if likely (perform_actual_write) storemask(addr, sfr.data, sfr.bytemask);

  L1CacheLine* L1line = L1.select(addr);

  L1line->valid |= ((W64)sfr.bytemask << lowbits(addr, 6));
  L2line->valid |= ((W64)sfr.bytemask << lowbits(addr, 6));

  if unlikely (!L1line->valid.allset()) {
    per_context_dcache_stats_update(threadid, store.prefetches++);
    missbuf.initiate_miss(addr, L2line->valid.allset(), false, 0xffff, threadid);
  }

  stoptimer(store_flush_timer);

  return 0;
}

//
// Submit a speculative store that marks the relevant bytes as valid
// so they can be immediately forwarded to loads, but do not actually
// write to the cache itself.
//
W64 CacheHierarchy::speculative_store(const SFR& sfr, int threadid) {
  return commitstore(sfr, threadid, false);
}

void CacheHierarchy::clock() {
  if unlikely ((sim_cycle & 0x7fffffff) == 0x7fffffff) {
    // Clear any 32-bit cycle-related counters in the cache to prevent wraparound:
    L1.clearstats();
    L1I.clearstats();
    L2.clearstats();
#ifdef ENABLE_L3_CACHE
    L3.clearstats();
#endif
    logfile << "Clearing cache statistics to prevent wraparound...", endl, flush;
  }

  lfrq.clock();
  missbuf.clock();
}

void CacheHierarchy::complete() {
  lfrq.restart();
  missbuf.restart();
}

void CacheHierarchy::complete(int threadid) {
  lfrq.reset(threadid);
  missbuf.reset(threadid);
}

void CacheHierarchy::reset() {
  lfrq.reset();
  missbuf.reset();
#ifdef ENABLE_L3_CACHE
  L3.reset();
#endif
  L2.reset();
  L1.reset();
  L1I.reset();
  itlb.reset();
  dtlb.reset();
}

ostream& CacheHierarchy::print(ostream& os) {
  os << "Data Cache Subsystem:", endl;
  os << lfrq;
  os << missbuf;
  // logfile << L1; 
  // logfile << L2; 
  return os;
}

//
// Make sure the templates and vtables get instantiated:
//
void PerCoreCacheCallbacks::dcache_wakeup(LoadStoreInfo lsi, W64 physaddr) { }
void PerCoreCacheCallbacks::icache_wakeup(LoadStoreInfo lsi, W64 physaddr) { }

template struct LoadFillReqQueue<LFRQ_SIZE>;
template struct MissBuffer<MISSBUF_COUNT>;

/*
// Generator for expand_8bit_to_64bit_lut:

foreach (i, 256) {
byte* m = (byte*)(&expand_8bit_to_64bit_lut[i]);
m[0] = (bit(i, 0) ? 0xff : 0x00);
m[1] = (bit(i, 1) ? 0xff : 0x00);
m[2] = (bit(i, 2) ? 0xff : 0x00);
m[3] = (bit(i, 3) ? 0xff : 0x00);
m[4] = (bit(i, 4) ? 0xff : 0x00);
m[5] = (bit(i, 5) ? 0xff : 0x00);
m[6] = (bit(i, 6) ? 0xff : 0x00);
m[7] = (bit(i, 7) ? 0xff : 0x00);
logfile << "  0x", hexstring(expand_8bit_to_64bit_lut[i], 64), ", ";
if ((i & 3) == 3) logfile << endl;
}
*/

