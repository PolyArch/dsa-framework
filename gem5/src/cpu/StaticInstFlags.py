# Copyright (c) 2003-2005 The Regents of The University of Michigan
# Copyright (c) 2013 Advanced Micro Devices, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Steve Reinhardt

from m5.params import *

# Set of boolean static instruction properties.
#
# Notes:
# - The IsInteger and IsFloating flags are based on the class of registers
# accessed by the instruction.  Although most instructions will have exactly
# one of these two flags set, it is possible for an instruction to have
# neither (e.g., direct unconditional branches, memory barriers) or both
# (e.g., an FP/int conversion).
# - If IsMemRef is set, then exactly one of IsLoad or IsStore will be set.
# - If IsControl is set, then exactly one of IsDirectControl or IsIndirect
# Control will be set, and exactly one of IsCondControl or IsUncondControl
# will be set.
# - IsSerializing, IsMemBarrier, and IsWriteBarrier are implemented as flags
# since in the current model there's no other way for instructions to inject
# behavior into the pipeline outside of fetch.  Once we go to an exec-in-exec
# CPU model we should be able to get rid of these flags and implement this
# behavior via the execute() methods.

class StaticInstFlags(Enum):
    wrapper_name = 'StaticInstFlags'
    wrapper_is_struct = True
    enum_name = 'Flags'

    vals = [
        'IsNop',            # Is a no-op (no effect at all).

        'IsInteger',        # References integer regs.
        'IsFloating',       # References FP regs.
        'IsCC',             # References CC regs.
        'IsVector',         # References Vector regs.
        'IsVectorElem',     # References Vector reg elems.

        'IsMemRef',         # References memory (load, store, or prefetch)
        'IsLoad',           # Reads from memory (load or prefetch).
        'IsStore',          # Writes to memory.
        'IsAtomic',         # Does atomic RMW to memory.
        'IsStoreConditional',   # Store conditional instruction.
        'IsIndexed',        # Accesses memory with an indexed address
                            # computation
        'IsInstPrefetch',   # Instruction-cache prefetch.
        'IsDataPrefetch',   # Data-cache prefetch.

        'IsControl',        # Control transfer instruction.
        'IsDirectControl',  # PC relative control transfer.
        'IsIndirectControl',# Register indirect control transfer.
        'IsCondControl',    # Conditional control transfer.
        'IsUncondControl',  # Unconditional control transfer.
        'IsCall',           # Subroutine call.
        'IsReturn',         # Subroutine return.

        'IsCondDelaySlot',  # Conditional Delay-Slot Instruction

        'IsThreadSync',     # Thread synchronization operation.

        'IsSerializing',    # Serializes pipeline: won't execute until all
                            # older instructions have committed.
        'IsSerializeBefore',
        'IsSerializeAfter',
        'IsMemBarrier',     # Is a memory barrier
        'IsWriteBarrier',   # Is a write barrier
        'IsReadBarrier',    # Is a read barrier
        'IsERET',           # <- Causes the IFU to stall (MIPS ISA)

        'IsNonSpeculative', # Should not be executed speculatively
        'IsQuiesce',        # Is a quiesce instruction

        'IsIprAccess',      # Accesses IPRs
        'IsUnverifiable',   # Can't be verified by a checker

        'IsSyscall',        # Causes a system call to be emulated in syscall
                            # emulation mode.

        # Flags for microcode
        'IsMacroop',        # Is a macroop containing microops
        'IsMicroop',        # Is a microop
        'IsDelayedCommit',  # This microop doesn't commit right away
        'IsLastMicroop',    # This microop ends a microop sequence
        'IsFirstMicroop',   # This microop begins a microop sequence
        # This flag doesn't do anything yet
        'IsMicroBranch',    # This microop branches within the microcode for
                            # a macroop
        'IsDspOp',
        'IsSquashAfter',     # Squash all uncommitted state after executed

        
        'IsSSWait', # Stream Dataflow Wait 
        'IsSSStream', # Stream Dataflow Stream Push
        'IsSSParam', 
        'IsSSRecv', # Stream Dataflow Stream Push
        'IsSSConfig', # Stream Dataflow Config

        ]
