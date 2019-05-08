/*
 * Copyright (c) 2012, 2014 ARM Limited
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
 * Copyright (c) 2004-2006 The Regents of The University of Michigan
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
 * Authors: Kevin Lim
 */

#ifndef __CPU_O3_MEM_DEP_UNIT_HH__
#define __CPU_O3_MEM_DEP_UNIT_HH__

#include <list>
#include <memory>
#include <set>
#include <unordered_map>

#include "base/statistics.hh"
#include "cpu/inst_seq.hh"
#include "debug/MemDepUnit.hh"

struct SNHash {
    size_t operator() (const InstSeqNum &seq_num) const {
        unsigned a = (unsigned)seq_num;
        unsigned hash = (((a >> 14) ^ ((a >> 2) & 0xffff))) & 0x7FFFFFFF;

        return hash;
    }
};

struct DerivO3CPUParams;

template <class Impl>
class InstructionQueue;

/**
 * Memory dependency unit class.  This holds the memory dependence predictor.
 * As memory operations are issued to the IQ, they are also issued to this
 * unit, which then looks up the prediction as to what they are dependent
 * upon.  This unit must be checked prior to a memory operation being able
 * to issue.  Although this is templated, it's somewhat hard to make a generic
 * memory dependence unit.  This one is mostly for store sets; it will be
 * quite limited in what other memory dependence predictions it can also
 * utilize.  Thus this class should be most likely be rewritten for other
 * dependence prediction schemes.
 */
template <class MemDepPred, class Impl>
class MemDepUnit
{
  protected:
    std::string _name;

  public:
    typedef typename Impl::DynInstPtr DynInstPtr;

    /** Empty constructor. Must call init() prior to using in this case. */
    MemDepUnit();

    /** Constructs a MemDepUnit with given parameters. */
    MemDepUnit(DerivO3CPUParams *params);

    /** Frees up any memory allocated. */
    ~MemDepUnit();

    /** Returns the name of the memory dependence unit. */
    std::string name() const { return _name; }

    /** Initializes the unit with parameters and a thread id. */
    void init(DerivO3CPUParams *params, ThreadID tid);

    /** Registers statistics. */
    void regStats();

    /** Determine if we are drained. */
    bool isDrained() const;

    /** Perform sanity checks after a drain. */
    void drainSanityCheck() const;

    /** Takes over from another CPU's thread. */
    void takeOverFrom();

    /** Sets the pointer to the IQ. */
    void setIQ(InstructionQueue<Impl> *iq_ptr);

    /** Inserts a memory instruction. */
    void insert(DynInstPtr &inst);

    /** Inserts a non-speculative memory instruction. */
    void insertNonSpec(DynInstPtr &inst);

    /** Inserts a barrier instruction. */
    void insertBarrier(DynInstPtr &barr_inst);

    /** Indicate that an instruction has its registers ready. */
    void regsReady(DynInstPtr &inst);

    /** Indicate that a non-speculative instruction is ready. */
    void nonSpecInstReady(DynInstPtr &inst);

    /** Reschedules an instruction to be re-executed. */
    void reschedule(DynInstPtr &inst);

    /** Replays all instructions that have been rescheduled by moving them to
     *  the ready list.
     */
    void replay();

    /** Completes a memory instruction. */
    void completed(DynInstPtr &inst);

    /** Completes a barrier instruction. */
    void completeBarrier(DynInstPtr &inst);

    /** Wakes any dependents of a memory instruction. */
    void wakeDependents(DynInstPtr &inst);

    /** Squashes all instructions up until a given sequence number for a
     *  specific thread.
     */
    void squash(const InstSeqNum &squashed_num, ThreadID tid);

    /** Indicates an ordering violation between a store and a younger load. */
    void violation(DynInstPtr &store_inst, DynInstPtr &violating_load);

    /** Issues the given instruction */
    void issue(DynInstPtr &inst);

    /** Debugging function to dump the lists of instructions. */
    void dumpLists();

  private:
    typedef typename std::list<DynInstPtr>::iterator ListIt;

    class MemDepEntry;

    typedef std::shared_ptr<MemDepEntry> MemDepEntryPtr;

    /** Memory dependence entries that track memory operations, marking
     *  when the instruction is ready to execute and what instructions depend
     *  upon it.
     */
    class MemDepEntry {
      public:
        /** Constructs a memory dependence entry. */
        MemDepEntry(DynInstPtr &new_inst)
            : inst(new_inst), regsReady(false), memDepReady(false),
              completed(false), squashed(false)
        {
#ifdef DEBUG
            ++memdep_count;

            DPRINTF(MemDepUnit, "Memory dependency entry created.  "
                    "memdep_count=%i %s\n", memdep_count, inst->pcState());
#endif
        }

        /** Frees any pointers. */
        ~MemDepEntry()
        {
            for (int i = 0; i < dependInsts.size(); ++i) {
                dependInsts[i] = NULL;
            }
#ifdef DEBUG
            --memdep_count;

            DPRINTF(MemDepUnit, "Memory dependency entry deleted.  "
                    "memdep_count=%i %s\n", memdep_count, inst->pcState());
#endif
        }

        /** Returns the name of the memory dependence entry. */
        std::string name() const { return "memdepentry"; }

        /** The instruction being tracked. */
        DynInstPtr inst;

        /** The iterator to the instruction's location inside the list. */
        ListIt listIt;

        /** A vector of any dependent instructions. */
        std::vector<MemDepEntryPtr> dependInsts;

        /** If the registers are ready or not. */
        bool regsReady;
        /** If all memory dependencies have been satisfied. */
        bool memDepReady;
        /** If the instruction is completed. */
        bool completed;
        /** If the instruction is squashed. */
        bool squashed;

        /** For debugging. */
#ifdef DEBUG
        static int memdep_count;
        static int memdep_insert;
        static int memdep_erase;
#endif
    };

    /** Finds the memory dependence entry in the hash map. */
    inline MemDepEntryPtr &findInHash(const DynInstPtr &inst);

    /** Moves an entry to the ready list. */
    inline void moveToReady(MemDepEntryPtr &ready_inst_entry);

    typedef std::unordered_map<InstSeqNum, MemDepEntryPtr, SNHash> MemDepHash;

    typedef typename MemDepHash::iterator MemDepHashIt;

    /** A hash map of all memory dependence entries. */
    MemDepHash memDepHash;

    /** A list of all instructions in the memory dependence unit. */
    std::list<DynInstPtr> instList[Impl::MaxThreads];

    /** A list of all instructions that are going to be replayed. */
    std::list<DynInstPtr> instsToReplay;

    /** The memory dependence predictor.  It is accessed upon new
     *  instructions being added to the IQ, and responds by telling
     *  this unit what instruction the newly added instruction is dependent
     *  upon.
     */
    MemDepPred depPred;

    /** Is there an outstanding load barrier that loads must wait on. */
    bool loadBarrier;
    /** The sequence number of the load barrier. */
    InstSeqNum loadBarrierSN;
    /** Is there an outstanding store barrier that loads must wait on. */
    bool storeBarrier;
    /** The sequence number of the store barrier. */
    InstSeqNum storeBarrierSN;

    /** Pointer to the IQ. */
    InstructionQueue<Impl> *iqPtr;

    /** The thread id of this memory dependence unit. */
    int id;

    /** Stat for number of inserted loads. */
    Stats::Scalar insertedLoads;
    /** Stat for number of inserted stores. */
    Stats::Scalar insertedStores;
    /** Stat for number of conflicting loads that had to wait for a store. */
    Stats::Scalar conflictingLoads;
    /** Stat for number of conflicting stores that had to wait for a store. */
    Stats::Scalar conflictingStores;
};

#endif // __CPU_O3_MEM_DEP_UNIT_HH__
