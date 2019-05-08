/*
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
 * Authors: Gabe Black
 */

#include "arch/sparc/nativetrace.hh"

#include "arch/sparc/isa_traits.hh"
#include "arch/sparc/registers.hh"
#include "cpu/thread_context.hh"
#include "params/SparcNativeTrace.hh"
#include "sim/byteswap.hh"

namespace Trace {

static const char *intRegNames[SparcISA::NumIntArchRegs] = {
    // Global registers
    "g0", "g1", "g2", "g3", "g4", "g5", "g6", "g7",
    // Output registers
    "o0", "o1", "o2", "o3", "o4", "o5", "o6", "o7",
    // Local registers
    "l0", "l1", "l2", "l3", "l4", "l5", "l6", "l7",
    // Input registers
    "i0", "i1", "i2", "i3", "i4", "i5", "i6", "i7",
};

void
Trace::SparcNativeTrace::check(NativeTraceRecord *record)
{
    ThreadContext *tc = record->getThread();

    uint64_t regVal, realRegVal;

    // Integer registers

    // I doubt a real SPARC will describe more integer registers than this.
    assert(SparcISA::NumIntArchRegs == 32);
    const char **regName = intRegNames;
    for (int i = 0; i < SparcISA::NumIntArchRegs; i++) {
        regVal = tc->readIntReg(i);
        read(&realRegVal, sizeof(realRegVal));
        realRegVal = SparcISA::gtoh(realRegVal);
        checkReg(*(regName++), regVal, realRegVal);
    }

    SparcISA::PCState pc = tc->pcState();
    // PC
    read(&realRegVal, sizeof(realRegVal));
    realRegVal = SparcISA::gtoh(realRegVal);
    regVal = pc.npc();
    checkReg("pc", regVal, realRegVal);

    // NPC
    read(&realRegVal, sizeof(realRegVal));
    realRegVal = SparcISA::gtoh(realRegVal);
    pc.nnpc();
    checkReg("npc", regVal, realRegVal);

    // CCR
    read(&realRegVal, sizeof(realRegVal));
    realRegVal = SparcISA::gtoh(realRegVal);
    regVal = tc->readIntReg(SparcISA::NumIntArchRegs + 2);
    checkReg("ccr", regVal, realRegVal);
}

} // namespace Trace

////////////////////////////////////////////////////////////////////////
//
//  ExeTracer Simulation Object
//
Trace::SparcNativeTrace *
SparcNativeTraceParams::create()
{
    return new Trace::SparcNativeTrace(this);
};
