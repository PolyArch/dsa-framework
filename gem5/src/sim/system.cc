/*
 * Copyright (c) 2011-2014,2017-2018 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2006 The Regents of The University of Michigan
 * Copyright (c) 2011 Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Steve Reinhardt
 *          Lisa Hsu
 *          Nathan Binkert
 *          Ali Saidi
 *          Rick Strong
 */

#include "sim/system.hh"

#include <algorithm>

#include "arch/remote_gdb.hh"
#include "arch/utility.hh"
#include "base/loader/object_file.hh"
#include "base/loader/symtab.hh"
#include "base/str.hh"
#include "base/trace.hh"
#include "config/use_kvm.hh"
#if USE_KVM
#include "cpu/kvm/base.hh"
#include "cpu/kvm/vm.hh"
#endif
#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "debug/Loader.hh"
#include "debug/WorkItems.hh"
#include "mem/abstract_mem.hh"
#include "mem/physical.hh"
#include "params/System.hh"
#include "sim/byteswap.hh"
#include "sim/debug.hh"
#include "sim/full_system.hh"

/**
 * To avoid linking errors with LTO, only include the header if we
 * actually have a definition.
 */
#if THE_ISA != NULL_ISA
#include "kern/kernel_stats.hh"

#endif

using namespace std;
using namespace TheISA;

vector<System *> System::systemList;

int System::numSystemsRunning = 0;

System::System(Params *p)
    : MemObject(p), _systemPort("system_port", this),
      multiThread(p->multi_thread),
      pagePtr(0),
      init_param(p->init_param),
      physProxy(_systemPort, p->cache_line_size),
      kernelSymtab(nullptr),
      kernel(nullptr),
      loadAddrMask(p->load_addr_mask),
      loadAddrOffset(p->load_offset),
#if USE_KVM
      kvmVM(p->kvm_vm),
#else
      kvmVM(nullptr),
#endif
      physmem(name() + ".physmem", p->memories, p->mmap_using_noreserve),
      memoryMode(p->mem_mode),
      _cacheLineSize(p->cache_line_size),
      workItemsBegin(0),
      workItemsEnd(0),
      numWorkIds(p->num_work_ids),
      thermalModel(p->thermal_model),
      _params(p),
      totalNumInsts(0),
      instEventQueue("system instruction-based event queue")
{
    // add self to global system list
    systemList.push_back(this);

#if USE_KVM
    if (kvmVM) {
        kvmVM->setSystem(this);
    }
#endif

    if (FullSystem) {
        kernelSymtab = new SymbolTable;
        if (!debugSymbolTable)
            debugSymbolTable = new SymbolTable;
    }

    // check if the cache line size is a value known to work
    if (!(_cacheLineSize == 16 || _cacheLineSize == 32 ||
          _cacheLineSize == 64 || _cacheLineSize == 128))
        warn_once("Cache line size is neither 16, 32, 64 nor 128 bytes.\n");

    // Get the generic system master IDs
    MasterID tmp_id M5_VAR_USED;
    tmp_id = getMasterId(this, "writebacks");
    assert(tmp_id == Request::wbMasterId);
    tmp_id = getMasterId(this, "functional");
    assert(tmp_id == Request::funcMasterId);
    tmp_id = getMasterId(this, "interrupt");
    assert(tmp_id == Request::intMasterId);

    if (FullSystem) {
        if (params()->kernel == "") {
            inform("No kernel set for full system simulation. "
                   "Assuming you know what you're doing\n");
        } else {
            // Get the kernel code
            kernel = createObjectFile(params()->kernel);
            inform("kernel located at: %s", params()->kernel);

            if (kernel == NULL)
                fatal("Could not load kernel file %s", params()->kernel);

            // setup entry points
            kernelStart = kernel->textBase();
            kernelEnd = kernel->bssBase() + kernel->bssSize();
            kernelEntry = kernel->entryPoint();

            // If load_addr_mask is set to 0x0, then auto-calculate
            // the smallest mask to cover all kernel addresses so gem5
            // can relocate the kernel to a new offset.
            if (loadAddrMask == 0) {
                Addr shift_amt = findMsbSet(kernelEnd - kernelStart) + 1;
                loadAddrMask = ((Addr)1 << shift_amt) - 1;
            }

            // load symbols
            if (!kernel->loadGlobalSymbols(kernelSymtab))
                fatal("could not load kernel symbols\n");

            if (!kernel->loadLocalSymbols(kernelSymtab))
                fatal("could not load kernel local symbols\n");

            if (!kernel->loadGlobalSymbols(debugSymbolTable))
                fatal("could not load kernel symbols\n");

            if (!kernel->loadLocalSymbols(debugSymbolTable))
                fatal("could not load kernel local symbols\n");

            // Loading only needs to happen once and after memory system is
            // connected so it will happen in initState()
        }

        for (const auto &obj_name : p->kernel_extras) {
            inform("Loading additional kernel object: %s", obj_name);
            ObjectFile *obj = createObjectFile(obj_name);
            fatal_if(!obj, "Failed to additional kernel object '%s'.\n",
                     obj_name);
            kernelExtras.push_back(obj);
        }
    }

    // increment the number of running systems
    numSystemsRunning++;

    // Set back pointers to the system in all memories
    for (int x = 0; x < params()->memories.size(); x++)
        params()->memories[x]->system(this);
    //---------------------
    for(int i=0; i<64; ++i) {
      _is_spu_done[i] = 0;
      _is_spu_global_wait_released[i] = 0;
    }
    //-----------------
}

System::~System()
{
    delete kernelSymtab;
    delete kernel;

    for (uint32_t j = 0; j < numWorkIds; j++)
        delete workItemStats[j];
}

void
System::init()
{
    // check that the system port is connected
    if (!_systemPort.isConnected())
        panic("System port on %s is not connected.\n", name());
}

BaseMasterPort&
System::getMasterPort(const std::string &if_name, PortID idx)
{
    // no need to distinguish at the moment (besides checking)
    return _systemPort;
}

void
System::setMemoryMode(Enums::MemoryMode mode)
{
    assert(drainState() == DrainState::Drained);
    memoryMode = mode;
}

bool System::breakpoint()
{
    if (remoteGDB.size())
        return remoteGDB[0]->breakpoint();
    return false;
}

ContextID
System::registerThreadContext(ThreadContext *tc, ContextID assigned)
{
    int id = assigned;
    if (id == InvalidContextID) {
        // Find an unused context ID for this thread.
        id = 0;
        while (id < threadContexts.size() && threadContexts[id])
            id++;
    }

    if (threadContexts.size() <= id)
        threadContexts.resize(id + 1);

    fatal_if(threadContexts[id],
             "Cannot have two CPUs with the same id (%d)\n", id);

    threadContexts[id] = tc;

#if THE_ISA != NULL_ISA
    int port = getRemoteGDBPort();
    if (port) {
        RemoteGDB *rgdb = new RemoteGDB(this, tc, port + id);
        rgdb->listen();

        BaseCPU *cpu = tc->getCpuPtr();
        if (cpu->waitForRemoteGDB()) {
            inform("%s: Waiting for a remote GDB connection on port %d.\n",
                   cpu->name(), rgdb->port());

            rgdb->connect();
        }
        if (remoteGDB.size() <= id) {
            remoteGDB.resize(id + 1);
        }

        remoteGDB[id] = rgdb;
    }
#endif

    activeCpus.push_back(false);

    return id;
}

int
System::numRunningContexts()
{
    return std::count_if(
        threadContexts.cbegin(),
        threadContexts.cend(),
        [] (ThreadContext* tc) {
            return tc->status() != ThreadContext::Halted;
        }
    );
}

void
System::initState()
{
    if (FullSystem) {
        for (int i = 0; i < threadContexts.size(); i++)
            TheISA::startupCPU(threadContexts[i], i);
        // Moved from the constructor to here since it relies on the
        // address map being resolved in the interconnect
        /**
         * Load the kernel code into memory
         */
        if (params()->kernel != "")  {
            if (params()->kernel_addr_check) {
                // Validate kernel mapping before loading binary
                if (!(isMemAddr((kernelStart & loadAddrMask) +
                                loadAddrOffset) &&
                      isMemAddr((kernelEnd & loadAddrMask) +
                                loadAddrOffset))) {
                    fatal("Kernel is mapped to invalid location (not memory). "
                          "kernelStart 0x(%x) - kernelEnd 0x(%x) %#x:%#x\n",
                          kernelStart,
                          kernelEnd, (kernelStart & loadAddrMask) +
                          loadAddrOffset,
                          (kernelEnd & loadAddrMask) + loadAddrOffset);
                }
            }
            // Load program sections into memory
            kernel->loadSections(physProxy, loadAddrMask, loadAddrOffset);
            for (const auto &extra_kernel : kernelExtras) {
                extra_kernel->loadSections(physProxy, loadAddrMask,
                                           loadAddrOffset);
            }

            DPRINTF(Loader, "Kernel start = %#x\n", kernelStart);
            DPRINTF(Loader, "Kernel end   = %#x\n", kernelEnd);
            DPRINTF(Loader, "Kernel entry = %#x\n", kernelEntry);
            DPRINTF(Loader, "Kernel loaded...\n");
        }
    }
}

void
System::replaceThreadContext(ThreadContext *tc, ContextID context_id)
{
    if (context_id >= threadContexts.size()) {
        panic("replaceThreadContext: bad id, %d >= %d\n",
              context_id, threadContexts.size());
    }

    threadContexts[context_id] = tc;
    if (context_id < remoteGDB.size())
        remoteGDB[context_id]->replaceThreadContext(tc);
}

bool
System::validKvmEnvironment() const
{
#if USE_KVM
    if (threadContexts.empty())
        return false;

    for (auto tc : threadContexts) {
        if (dynamic_cast<BaseKvmCPU*>(tc->getCpuPtr()) == nullptr) {
            return false;
        }
    }
    return true;
#else
    return false;
#endif
}

Addr
System::allocPhysPages(int npages)
{
    Addr return_addr = pagePtr << PageShift;
    pagePtr += npages;

    Addr next_return_addr = pagePtr << PageShift;

    AddrRange m5opRange(0xffff0000, 0xffffffff);
    if (m5opRange.contains(next_return_addr)) {
        warn("Reached m5ops MMIO region\n");
        return_addr = 0xffffffff;
        pagePtr = 0xffffffff >> PageShift;
    }

    if ((pagePtr << PageShift) > physmem.totalSize())
        fatal("Out of memory, please increase size of physical memory.");
    return return_addr;
}

Addr
System::memSize() const
{
    return physmem.totalSize();
}

Addr
System::freeMemSize() const
{
   return physmem.totalSize() - (pagePtr << PageShift);
}

bool
System::isMemAddr(Addr addr) const
{
    return physmem.isMemAddr(addr);
}

void
System::drainResume()
{
    totalNumInsts = 0;
}

void
System::serialize(CheckpointOut &cp) const
{
    if (FullSystem)
        kernelSymtab->serialize("kernel_symtab", cp);
    SERIALIZE_SCALAR(pagePtr);
    serializeSymtab(cp);

    // also serialize the memories in the system
    physmem.serializeSection(cp, "physmem");
}


void
System::unserialize(CheckpointIn &cp)
{
    if (FullSystem)
        kernelSymtab->unserialize("kernel_symtab", cp);
    UNSERIALIZE_SCALAR(pagePtr);
    unserializeSymtab(cp);

    // also unserialize the memories in the system
    physmem.unserializeSection(cp, "physmem");
}

void
System::regStats()
{
    MemObject::regStats();

    for (uint32_t j = 0; j < numWorkIds ; j++) {
        workItemStats[j] = new Stats::Histogram();
        stringstream namestr;
        ccprintf(namestr, "work_item_type%d", j);
        workItemStats[j]->init(20)
                         .name(name() + "." + namestr.str())
                         .desc("Run time stat for" + namestr.str())
                         .prereq(*workItemStats[j]);
    }
}

void
System::workItemEnd(uint32_t tid, uint32_t workid)
{
    std::pair<uint32_t,uint32_t> p(tid, workid);
    if (!lastWorkItemStarted.count(p))
        return;

    Tick samp = curTick() - lastWorkItemStarted[p];
    DPRINTF(WorkItems, "Work item end: %d\t%d\t%lld\n", tid, workid, samp);

    if (workid >= numWorkIds)
        fatal("Got workid greater than specified in system configuration\n");

    workItemStats[workid]->sample(samp);
    lastWorkItemStarted.erase(p);
}

void
System::printSystems()
{
    ios::fmtflags flags(cerr.flags());

    vector<System *>::iterator i = systemList.begin();
    vector<System *>::iterator end = systemList.end();
    for (; i != end; ++i) {
        System *sys = *i;
        cerr << "System " << sys->name() << ": " << hex << sys << endl;
    }

    cerr.flags(flags);
}

void
printSystems()
{
    System::printSystems();
}

MasterID
System::getGlobalMasterId(std::string master_name)
{
    return _getMasterId(nullptr, master_name);
}

MasterID
System::getMasterId(const SimObject* master, std::string submaster)
{
    auto master_name = leafMasterName(master, submaster);
    return _getMasterId(master, master_name);
}

MasterID
System::_getMasterId(const SimObject* master, std::string master_name)
{
    if (startswith(master_name, name()))
        master_name = master_name.erase(0, name().size() + 1);

    // CPUs in switch_cpus ask for ids again after switching
    for (int i = 0; i < masters.size(); i++) {
        if (masters[i].masterName == master_name) {
            return i;
        }
    }

    // Verify that the statistics haven't been enabled yet
    // Otherwise objects will have sized their stat buckets and
    // they will be too small

    if (Stats::enabled()) {
        fatal("Can't request a masterId after regStats(). "
                "You must do so in init().\n");
    }

    // Generate a new MasterID incrementally
    MasterID master_id = masters.size();

    // Append the new Master metadata to the group of system Masters.
    masters.emplace_back(master, master_name, master_id);

    return masters.back().masterId;
}

std::string
System::leafMasterName(const SimObject* master, const std::string& submaster)
{
    if (submaster.empty()) {
        return master->name();
    } else {
        // Get the full master name by appending the submaster name to
        // the root SimObject master name
        return master->name() + "." + submaster;
    }
}

std::string
System::getMasterName(MasterID master_id)
{
    if (master_id >= masters.size())
        fatal("Invalid master_id passed to getMasterName()\n");

    const auto& master_info = masters[master_id];
    return master_info.masterName;
}

System *
SystemParams::create()
{
    return new System(this);
}

void
System::set_spu_done(int spu_id) {
  _is_spu_done[spu_id-1]=1;
  /*for(int i=0; i<64; ++i) {
    printf("At i, it is: %d\n",_is_spu_done[i]);
  }*/
}

bool
System::all_spu_done(int num_active_threads) {
  for(int i=0; i<num_active_threads; ++i) {
    if(_is_spu_done[i]==0) {
      // printf("SPU not done id: %d\n",i);
      return false;
    }
  }
  return true;
}

void
System::reset_all_spu() {
  for(int i=0; i<64; ++i) {
    _is_spu_done[i]=0;
  }
}

void
System::set_spu_global_wait_released(int spu_id) {
  _is_spu_global_wait_released[spu_id-1]=1;
}

void
System::reset_all_spu_global_wait() {
  for(int i=0; i<64; ++i) {
    _is_spu_global_wait_released[i]=0;
  }
}

bool
System::is_last_spu(int num_active_threads) {
  for(int i=0; i<num_active_threads; ++i) {
    if(_is_spu_global_wait_released[i]==0)
      return false;
  }
  return true;
}
