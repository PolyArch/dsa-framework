/*
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
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
 *          Ali Saidi
 */

#ifndef __ARCH_SPARC_REGISTERS_HH__
#define __ARCH_SPARC_REGISTERS_HH__

#include "arch/generic/vec_reg.hh"
#include "arch/sparc/generated/max_inst_regs.hh"
#include "arch/sparc/miscregs.hh"
#include "arch/sparc/sparc_traits.hh"
#include "base/types.hh"

namespace SparcISA
{

using SparcISAInst::MaxInstSrcRegs;
using SparcISAInst::MaxInstDestRegs;
using SparcISAInst::MaxMiscDestRegs;

typedef uint64_t IntReg;
typedef uint64_t MiscReg;
typedef float FloatReg;
typedef uint32_t FloatRegBits;

// dummy typedef since we don't have CC regs
typedef uint8_t CCReg;

// dummy typedefs since we don't have vector regs
constexpr unsigned NumVecElemPerVecReg = 2;
using VecElem = uint32_t;
using VecReg = ::VecRegT<VecElem, NumVecElemPerVecReg, false>;
using ConstVecReg = ::VecRegT<VecElem, NumVecElemPerVecReg, true>;
using VecRegContainer = VecReg::Container;
// This has to be one to prevent warnings that are treated as errors
constexpr unsigned NumVecRegs = 1;

typedef union
{
    IntReg intReg;
    FloatReg fpreg;
    MiscReg ctrlreg;
} AnyReg;

// semantically meaningful register indices
const int ZeroReg = 0;      // architecturally meaningful
// the rest of these depend on the ABI
const int ReturnAddressReg = 31; // post call, precall is 15
const int ReturnValueReg = 8;  // Post return, 24 is pre-return.
const int StackPointerReg = 14;
const int FramePointerReg = 30;

// Some OS syscall use a second register (o1) to return a second value
const int SyscallPseudoReturnReg = 9;

const int NumIntArchRegs = 32;
const int NumIntRegs = (MaxGL + 1) * 8 + NWindows * 16 + NumMicroIntRegs;
const int NumCCRegs = 0;

const int TotalNumRegs = NumIntRegs + NumFloatRegs + NumMiscRegs;

} // namespace SparcISA

#endif
