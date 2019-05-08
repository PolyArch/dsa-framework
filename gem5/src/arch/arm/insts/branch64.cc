/*
 * Copyright (c) 2011-2013 ARM Limited
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

#include "arch/arm/insts/branch64.hh"

namespace ArmISA
{

ArmISA::PCState
BranchImm64::branchTarget(const ArmISA::PCState &branchPC) const
{
    ArmISA::PCState pcs = branchPC;
    pcs.instNPC(pcs.pc() + imm);
    pcs.advance();
    return pcs;
}

ArmISA::PCState
BranchImmReg64::branchTarget(const ArmISA::PCState &branchPC) const
{
    ArmISA::PCState pcs = branchPC;
    pcs.instNPC(pcs.pc() + imm);
    pcs.advance();
    return pcs;
}

ArmISA::PCState
BranchImmImmReg64::branchTarget(const ArmISA::PCState &branchPC) const
{
    ArmISA::PCState pcs = branchPC;
    pcs.instNPC(pcs.pc() + imm2);
    pcs.advance();
    return pcs;
}

std::string
BranchImmCond64::generateDisassembly(
        Addr pc, const SymbolTable *symtab) const
{
    std::stringstream ss;
    printMnemonic(ss, "", false, true, condCode);
    printTarget(ss, pc + imm, symtab);
    return ss.str();
}

std::string
BranchImm64::generateDisassembly(
        Addr pc, const SymbolTable *symtab) const
{
    std::stringstream ss;
    printMnemonic(ss, "", false);
    printTarget(ss, pc + imm, symtab);
    return ss.str();
}

std::string
BranchReg64::generateDisassembly(
        Addr pc, const SymbolTable *symtab) const
{
    std::stringstream ss;
    printMnemonic(ss, "", false);
    printIntReg(ss, op1);
    return ss.str();
}

std::string
BranchRet64::generateDisassembly(
        Addr pc, const SymbolTable *symtab) const
{
    std::stringstream ss;
    printMnemonic(ss, "", false);
    if (op1 != INTREG_X30)
        printIntReg(ss, op1);
    return ss.str();
}

std::string
BranchEret64::generateDisassembly(
        Addr pc, const SymbolTable *symtab) const
{
    std::stringstream ss;
    printMnemonic(ss, "", false);
    return ss.str();
}

std::string
BranchImmReg64::generateDisassembly(
        Addr pc, const SymbolTable *symtab) const
{
    std::stringstream ss;
    printMnemonic(ss, "", false);
    printIntReg(ss, op1);
    ccprintf(ss, ", ");
    printTarget(ss, pc + imm, symtab);
    return ss.str();
}

std::string
BranchImmImmReg64::generateDisassembly(
        Addr pc, const SymbolTable *symtab) const
{
    std::stringstream ss;
    printMnemonic(ss, "", false);
    printIntReg(ss, op1);
    ccprintf(ss, ", #%#x, ", imm1);
    printTarget(ss, pc + imm2, symtab);
    return ss.str();
}

} // namespace ArmISA
