/*
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
 * Copyright (c) 2007-2008 The Florida State University
 * Copyright (c) 2009 The University of Edinburgh
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
 * Authors: Korey Sewell
 *          Stephen Hines
 *          Timothy M. Jones
 */

#include "arch/power/utility.hh"

#include "base/logging.hh"

namespace PowerISA {

void
copyRegs(ThreadContext *src, ThreadContext *dest)
{
    // First loop through the integer registers.
    for (int i = 0; i < NumIntRegs; ++i)
        dest->setIntReg(i, src->readIntReg(i));

    // Then loop through the floating point registers.
    for (int i = 0; i < NumFloatRegs; ++i)
        dest->setFloatRegBits(i, src->readFloatRegBits(i));

    // Would need to add condition-code regs if implemented
    assert(NumCCRegs == 0);

    // Copy misc. registers
    copyMiscRegs(src, dest);

    // Lastly copy PC/NPC
    dest->pcState(src->pcState());
}

uint64_t
getArgument(ThreadContext *tc, int &number, uint16_t size, bool fp)
{
    panic("getArgument not implemented for POWER.\n");
    return 0;
}

void
skipFunction(ThreadContext *tc)
{
    panic("Not Implemented for POWER");
}

void
initCPU(ThreadContext *tc, int cpuId)
{
    panic("initCPU not implemented for POWER.\n");
}


} // namespace PowerISA
