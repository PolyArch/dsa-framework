/*
 * Copyright (c) 2012 ARM Limited
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
 * Copyright (c) 2006 The Regents of The University of Michigan
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

#ifndef __CPU_O3_THREAD_STATE_HH__
#define __CPU_O3_THREAD_STATE_HH__

#include "base/callback.hh"
#include "base/output.hh"
#include "cpu/thread_context.hh"
#include "cpu/thread_state.hh"
#include "sim/full_system.hh"
#include "sim/sim_exit.hh"

class EndQuiesceEvent;
class Event;
class FunctionalMemory;
class FunctionProfile;
class Process;
class ProfileNode;

/**
 * Class that has various thread state, such as the status, the
 * current instruction being processed, whether or not the thread has
 * a trap pending or is being externally updated, the ThreadContext
 * pointer, etc.  It also handles anything related to a specific
 * thread's process, such as syscalls and checking valid addresses.
 */
template <class Impl>
struct O3ThreadState : public ThreadState {
    typedef ThreadContext::Status Status;
    typedef typename Impl::O3CPU O3CPU;

  private:
    /** Pointer to the CPU. */
    O3CPU *cpu;
  public:
    /* This variable controls if writes to a thread context should cause a all
     * dynamic/speculative state to be thrown away. Nominally this is the
     * desired behavior because the external thread context write has updated
     * some state that could be used by an inflight instruction, however there
     * are some cases like in a fault/trap handler where this behavior would
     * lead to successive restarts and forward progress couldn't be made. This
     * variable controls if the squashing will occur.
     */
    bool noSquashFromTC;

    /** Whether or not the thread is currently waiting on a trap, and
     * thus able to be externally updated without squashing.
     */
    bool trapPending;

    O3ThreadState(O3CPU *_cpu, int _thread_num, Process *_process)
        : ThreadState(_cpu, _thread_num, _process),
          cpu(_cpu), noSquashFromTC(false), trapPending(false),
          tc(nullptr)
    {
        if (!FullSystem)
            return;

        if (cpu->params()->profile) {
            profile = new FunctionProfile(
                    cpu->params()->system->kernelSymtab);
            Callback *cb =
                new MakeCallback<O3ThreadState,
                &O3ThreadState::dumpFuncProfile>(this);
            registerExitCallback(cb);
        }

        // let's fill with a dummy node for now so we don't get a segfault
        // on the first cycle when there's no node available.
        static ProfileNode dummyNode;
        profileNode = &dummyNode;
        profilePC = 3;
    }

    void serialize(CheckpointOut &cp) const override
    {
        ThreadState::serialize(cp);
        // Use the ThreadContext serialization helper to serialize the
        // TC.
        ::serialize(*tc, cp);
    }

    void unserialize(CheckpointIn &cp) override
    {
        // Prevent squashing - we don't have any instructions in
        // flight that we need to squash since we just instantiated a
        // clean system.
        noSquashFromTC = true;
        ThreadState::unserialize(cp);
        // Use the ThreadContext serialization helper to unserialize
        // the TC.
        ::unserialize(*tc, cp);
        noSquashFromTC = false;
    }

    /** Pointer to the ThreadContext of this thread. */
    ThreadContext *tc;

    /** Returns a pointer to the TC of this thread. */
    ThreadContext *getTC() { return tc; }

    /** Handles the syscall. */
    void syscall(int64_t callnum, Fault *fault)
    {
        process->syscall(callnum, tc, fault);
    }

    void dumpFuncProfile()
    {
        OutputStream *os(
            simout.create(csprintf("profile.%s.dat", cpu->name())));
        profile->dump(tc, *os->stream());
        simout.close(os);
    }
};

#endif // __CPU_O3_THREAD_STATE_HH__
