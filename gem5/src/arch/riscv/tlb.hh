/*
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
 * Copyright (c) 2007 MIPS Technologies, Inc.
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
 * Authors: Nathan Binkert
 *          Steve Reinhardt
 *          Jaidev Patwardhan
 *          Korey Sewell
 */

#ifndef __ARCH_RISCV_TLB_HH__
#define __ARCH_RISCV_TLB_HH__

#include <map>

#include "arch/generic/tlb.hh"
#include "arch/riscv/isa_traits.hh"
#include "arch/riscv/pagetable.hh"
#include "arch/riscv/utility.hh"
#include "arch/riscv/vtophys.hh"
#include "base/statistics.hh"
#include "mem/request.hh"
#include "params/RiscvTLB.hh"
#include "sim/sim_object.hh"

class ThreadContext;

/* To maintain compatibility with other architectures, we'll
   simply create an ITLB and DTLB that will point to the real TLB */
namespace RiscvISA {

class TLB : public BaseTLB
{
  protected:
    typedef std::multimap<Addr, int> PageTable;
    PageTable lookupTable;      // Quick lookup into page table

    RiscvISA::PTE *table;        // the Page Table
    int size;                   // TLB Size
    int nlu;                    // not last used entry (for replacement)

    void nextnlu() { if (++nlu >= size) nlu = 0; }
    RiscvISA::PTE *lookup(Addr vpn, uint8_t asn) const;

    mutable Stats::Scalar read_hits;
    mutable Stats::Scalar read_misses;
    mutable Stats::Scalar read_acv;
    mutable Stats::Scalar read_accesses;
    mutable Stats::Scalar write_hits;
    mutable Stats::Scalar write_misses;
    mutable Stats::Scalar write_acv;
    mutable Stats::Scalar write_accesses;
    Stats::Formula hits;
    Stats::Formula misses;
    Stats::Formula accesses;

  public:
    typedef RiscvTLBParams Params;
    TLB(const Params *p);

    int probeEntry(Addr vpn,uint8_t) const;
    RiscvISA::PTE *getEntry(unsigned) const;
    virtual ~TLB();

    void takeOverFrom(BaseTLB *otlb) override {}

    int smallPages;
    int getsize() const { return size; }

    RiscvISA::PTE &index(bool advance = true);
    void insert(Addr vaddr, RiscvISA::PTE &pte);
    void insertAt(RiscvISA::PTE &pte, unsigned Index, int _smallPages);
    void flushAll() override;
    void demapPage(Addr vaddr, uint64_t asn) override
    {
        panic("demapPage unimplemented.\n");
    }

    // static helper functions... really
    static bool validVirtualAddress(Addr vaddr);

    static Fault checkCacheability(const RequestPtr &req);

    // Checkpointing
    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;

    void regStats() override;

    Fault translateAtomic(
            const RequestPtr &req, ThreadContext *tc, Mode mode) override;
    void translateTiming(
            const RequestPtr &req, ThreadContext *tc,
            Translation *translation, Mode mode) override;
    Fault finalizePhysical(
            const RequestPtr &req,
            ThreadContext *tc, Mode mode) const override;

  private:
    Fault translateInst(const RequestPtr &req, ThreadContext *tc);
    Fault translateData(const RequestPtr &req, ThreadContext *tc, bool write);
};

}



#endif // __RISCV_MEMORY_HH__
