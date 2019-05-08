# Copyright (c) 2016 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2005-2007 The Regents of The University of Michigan
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
# Authors: Kevin Lim

from __future__ import print_function

from m5.defines import buildEnv
from m5.params import *
from m5.proxy import *
from BaseCPU import BaseCPU
from FUPool import *
from O3Checker import O3Checker
from BranchPredictor import *

class DerivO3CPU(BaseCPU):
    type = 'DerivO3CPU'
    cxx_header = 'cpu/o3/deriv.hh'

    @classmethod
    def memory_mode(cls):
        return 'timing'

    @classmethod
    def require_caches(cls):
        return True

    @classmethod
    def support_take_over(cls):
        return True

    activity = Param.Unsigned(0, "Initial count")

    cacheStorePorts = Param.Unsigned(200, "Cache Ports. "
          "Constrains stores only. Loads are constrained by load FUs.")

    executeMaxAccessesInMemory = Param.Unsigned(2,
        "Maximum number of concurrent accesses allowed to the memory system"
        " from the dcache port")

    decodeToFetchDelay = Param.Cycles(1, "Decode to fetch delay")
    renameToFetchDelay = Param.Cycles(1 ,"Rename to fetch delay")
    iewToFetchDelay = Param.Cycles(1, "Issue/Execute/Writeback to fetch "
                                   "delay")
    commitToFetchDelay = Param.Cycles(1, "Commit to fetch delay")
    fetchWidth = Param.Unsigned(6, "Fetch width")
    fetchBufferSize = Param.Unsigned(64, "Fetch buffer size in bytes")
    fetchQueueSize = Param.Unsigned(32, "Fetch queue size in micro-ops "
                                    "per-thread")

    renameToDecodeDelay = Param.Cycles(1, "Rename to decode delay")
    iewToDecodeDelay = Param.Cycles(1, "Issue/Execute/Writeback to decode "
                                    "delay")
    commitToDecodeDelay = Param.Cycles(1, "Commit to decode delay")
    fetchToDecodeDelay = Param.Cycles(1, "Fetch to decode delay")
    decodeWidth = Param.Unsigned(6, "Decode width")

    iewToRenameDelay = Param.Cycles(1, "Issue/Execute/Writeback to rename "
                                    "delay")
    commitToRenameDelay = Param.Cycles(1, "Commit to rename delay")
    decodeToRenameDelay = Param.Cycles(1, "Decode to rename delay")
    renameWidth = Param.Unsigned(8, "Rename width")

    commitToIEWDelay = Param.Cycles(1, "Commit to "
               "Issue/Execute/Writeback delay")
    renameToIEWDelay = Param.Cycles(2, "Rename to "
               "Issue/Execute/Writeback delay")
    issueToExecuteDelay = Param.Cycles(1, "Issue to execute delay (internal "
              "to the IEW stage)")
    dispatchWidth = Param.Unsigned(6, "Dispatch width")
    issueWidth = Param.Unsigned(6, "Issue width")
    wbWidth = Param.Unsigned(6, "Writeback width")
    fuPool = Param.FUPool(DefaultFUPool(), "Functional Unit pool")

    iewToCommitDelay = Param.Cycles(1, "Issue/Execute/Writeback to commit "
               "delay")
    renameToROBDelay = Param.Cycles(1, "Rename to reorder buffer delay")
    commitWidth = Param.Unsigned(6, "Commit width")
    squashWidth = Param.Unsigned(6, "Squash width")
    trapLatency = Param.Cycles(13, "Trap latency")
    fetchTrapLatency = Param.Cycles(1, "Fetch trap latency")

    backComSize = Param.Unsigned(5, "Time buffer size for backwards communication")
    forwardComSize = Param.Unsigned(5, "Time buffer size for forward communication")

    LQEntries = Param.Unsigned(32, "Number of load queue entries")
    SQEntries = Param.Unsigned(32, "Number of store queue entries")
    LSQDepCheckShift = Param.Unsigned(4, "Number of places to shift addr before check")
    LSQCheckLoads = Param.Bool(True,
        "Should dependency violations be checked for loads & stores or just stores")
    store_set_clear_period = Param.Unsigned(250000,
            "Number of load/store insts before the dep predictor should be invalidated")
    LFSTSize = Param.Unsigned(1024, "Last fetched store table size")
    SSITSize = Param.Unsigned(1024, "Store set ID table size")

    numRobs = Param.Unsigned(1, "Number of Reorder Buffers");

    numPhysIntRegs = Param.Unsigned(256, "Number of physical integer registers")
    numPhysFloatRegs = Param.Unsigned(256, "Number of physical floating point "
                                      "registers")
    # most ISAs don't use condition-code regs, so default is 0
    _defaultNumPhysCCRegs = 0
    if buildEnv['TARGET_ISA'] in ('arm','x86'):
        # For x86, each CC reg is used to hold only a subset of the
        # flags, so we need 4-5 times the number of CC regs as
        # physical integer regs to be sure we don't run out.  In
        # typical real machines, CC regs are not explicitly renamed
        # (it's a side effect of int reg renaming), so they should
        # never be the bottleneck here.
        _defaultNumPhysCCRegs = Self.numPhysIntRegs * 5
    numPhysVecRegs = Param.Unsigned(256, "Number of physical vector "
                                      "registers")
    numPhysCCRegs = Param.Unsigned(_defaultNumPhysCCRegs,
                                   "Number of physical cc registers")
    numIQEntries = Param.Unsigned(64, "Number of instruction queue entries")
    numROBEntries = Param.Unsigned(192, "Number of reorder buffer entries")

    smtNumFetchingThreads = Param.Unsigned(1, "SMT Number of Fetching Threads")
    smtFetchPolicy = Param.String('SingleThread', "SMT Fetch policy")
    smtLSQPolicy    = Param.String('Partitioned', "SMT LSQ Sharing Policy")
    smtLSQThreshold = Param.Int(100, "SMT LSQ Threshold Sharing Parameter")
    smtIQPolicy    = Param.String('Partitioned', "SMT IQ Sharing Policy")
    smtIQThreshold = Param.Int(100, "SMT IQ Threshold Sharing Parameter")
    smtROBPolicy   = Param.String('Partitioned', "SMT ROB Sharing Policy")
    smtROBThreshold = Param.Int(100, "SMT ROB Threshold Sharing Parameter")
    smtCommitPolicy = Param.String('RoundRobin', "SMT Commit Policy")

    branchPred = Param.BranchPredictor(TournamentBP(numThreads =
                                                       Parent.numThreads),
                                       "Branch Predictor")
    needsTSO = Param.Bool(buildEnv['TARGET_ISA'] == 'x86',
                          "Enable TSO Memory model")

    def addCheckerCpu(self):
        if buildEnv['TARGET_ISA'] in ['arm']:
            from ArmTLB import ArmTLB

            self.checker = O3Checker(workload=self.workload,
                                     exitOnError=False,
                                     updateOnError=True,
                                     warnOnlyOnLoadError=True)
            self.checker.itb = ArmTLB(size = self.itb.size)
            self.checker.dtb = ArmTLB(size = self.dtb.size)
            self.checker.cpu_id = self.cpu_id

        else:
            print("ERROR: Checker only supported under ARM ISA!")
            exit(1)
