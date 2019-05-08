/*
 * Copyright (c) 2008 The Regents of The University of Michigan
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

#ifndef __ARCH_X86_CPUID_HH__
#define __ARCH_X86_CPUID_HH__

#include "base/types.hh"

class ThreadContext;

namespace X86ISA
{
    struct CpuidResult
    {
        uint64_t rax;
        uint64_t rbx;
        uint64_t rcx;
        uint64_t rdx;

        // These are not in alphebetical order on purpose. The order reflects
        // how the CPUID orders the registers when it returns results.
        CpuidResult(uint64_t _rax, uint64_t _rbx,
                    uint64_t _rdx, uint64_t _rcx) :
            rax(_rax), rbx(_rbx), rcx(_rcx), rdx(_rdx)
        {}

        CpuidResult()
        {}
    };

    uint64_t stringToRegister(const char *str);

    bool doCpuid(ThreadContext * tc, uint32_t function,
            uint32_t index, CpuidResult &result);
} // namespace X86ISA

#endif
