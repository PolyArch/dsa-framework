# Copyright (c) 2012-2013, 2015-2017 ARM Limited
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
# Copyright (c) 2005-2008 The Regents of The University of Michigan
# Copyright (c) 2011 Regents of the University of California
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
# Authors: Nathan Binkert
#          Rick Strong
#          Andreas Hansson
#          Glenn Bergmans

from __future__ import print_function

import sys

from m5.SimObject import *
from m5.defines import buildEnv
from m5.params import *
from m5.proxy import *
from m5.util.fdthelper import *

from XBar import L2XBar
from InstTracer import InstTracer
from CPUTracers import ExeTracer
from MemObject import MemObject
from SubSystem import SubSystem
from ClockDomain import *
from Platform import Platform

default_tracer = ExeTracer()

if buildEnv['TARGET_ISA'] == 'alpha':
    from AlphaTLB import AlphaDTB as ArchDTB, AlphaITB as ArchITB
    from AlphaInterrupts import AlphaInterrupts
    from AlphaISA import AlphaISA
    default_isa_class = AlphaISA
elif buildEnv['TARGET_ISA'] == 'sparc':
    from SparcTLB import SparcTLB as ArchDTB, SparcTLB as ArchITB
    from SparcInterrupts import SparcInterrupts
    from SparcISA import SparcISA
    default_isa_class = SparcISA
elif buildEnv['TARGET_ISA'] == 'x86':
    from X86TLB import X86TLB as ArchDTB, X86TLB as ArchITB
    from X86LocalApic import X86LocalApic
    from X86ISA import X86ISA
    default_isa_class = X86ISA
elif buildEnv['TARGET_ISA'] == 'mips':
    from MipsTLB import MipsTLB as ArchDTB, MipsTLB as ArchITB
    from MipsInterrupts import MipsInterrupts
    from MipsISA import MipsISA
    default_isa_class = MipsISA
elif buildEnv['TARGET_ISA'] == 'arm':
    from ArmTLB import ArmTLB as ArchDTB, ArmTLB as ArchITB
    from ArmTLB import ArmStage2IMMU, ArmStage2DMMU
    from ArmInterrupts import ArmInterrupts
    from ArmISA import ArmISA
    default_isa_class = ArmISA
elif buildEnv['TARGET_ISA'] == 'power':
    from PowerTLB import PowerTLB as ArchDTB, PowerTLB as ArchITB
    from PowerInterrupts import PowerInterrupts
    from PowerISA import PowerISA
    default_isa_class = PowerISA
elif buildEnv['TARGET_ISA'] == 'riscv':
    from RiscvTLB import RiscvTLB as ArchDTB, RiscvTLB as ArchITB
    from RiscvInterrupts import RiscvInterrupts
    from RiscvISA import RiscvISA
    default_isa_class = RiscvISA

class BaseCPU(MemObject):
    type = 'BaseCPU'
    abstract = True
    cxx_header = "cpu/base.hh"

    cxx_exports = [
        PyBindMethod("switchOut"),
        PyBindMethod("takeOverFrom"),
        PyBindMethod("switchedOut"),
        PyBindMethod("flushTLBs"),
        PyBindMethod("totalInsts"),
        PyBindMethod("scheduleInstStop"),
        PyBindMethod("scheduleLoadStop"),
        PyBindMethod("getCurrentInstCount"),
    ]

    @classmethod
    def memory_mode(cls):
        """Which memory mode does this CPU require?"""
        return 'invalid'

    @classmethod
    def require_caches(cls):
        """Does the CPU model require caches?

        Some CPU models might make assumptions that require them to
        have caches.
        """
        return False

    @classmethod
    def support_take_over(cls):
        """Does the CPU model support CPU takeOverFrom?"""
        return False

    def takeOverFrom(self, old_cpu):
        self._ccObject.takeOverFrom(old_cpu._ccObject)


    system = Param.System(Parent.any, "system object")
    cpu_id = Param.Int(-1, "CPU identifier")
    socket_id = Param.Unsigned(0, "Physical Socket identifier")
    numThreads = Param.Unsigned(1, "number of HW thread contexts")
    pwr_gating_latency = Param.Cycles(300,
        "Latency to enter power gating state when all contexts are suspended")

    power_gating_on_idle = Param.Bool(False, "Control whether the core goes "\
        "to the OFF power state after all thread are disabled for "\
        "pwr_gating_latency cycles")

    function_trace = Param.Bool(False, "Enable function trace")
    function_trace_start = Param.Tick(0, "Tick to start function trace")

    checker = Param.BaseCPU(NULL, "checker CPU")

    syscallRetryLatency = Param.Cycles(10000, "Cycles to wait until retry")

    do_checkpoint_insts = Param.Bool(True,
        "enable checkpoint pseudo instructions")
    do_statistics_insts = Param.Bool(True,
        "enable statistics pseudo instructions")

    profile = Param.Latency('0ns', "trace the kernel stack")
    do_quiesce = Param.Bool(True, "enable quiesce instructions")

    wait_for_remote_gdb = Param.Bool(False,
        "Wait for a remote GDB connection");

    workload = VectorParam.Process([], "processes to run")

    dtb = Param.BaseTLB(ArchDTB(), "Data TLB")
    itb = Param.BaseTLB(ArchITB(), "Instruction TLB")
    if buildEnv['TARGET_ISA'] == 'sparc':
        interrupts = VectorParam.SparcInterrupts(
                [], "Interrupt Controller")
        isa = VectorParam.SparcISA([], "ISA instance")
    elif buildEnv['TARGET_ISA'] == 'alpha':
        interrupts = VectorParam.AlphaInterrupts(
                [], "Interrupt Controller")
        isa = VectorParam.AlphaISA([], "ISA instance")
    elif buildEnv['TARGET_ISA'] == 'x86':
        interrupts = VectorParam.X86LocalApic([], "Interrupt Controller")
        isa = VectorParam.X86ISA([], "ISA instance")
    elif buildEnv['TARGET_ISA'] == 'mips':
        interrupts = VectorParam.MipsInterrupts(
                [], "Interrupt Controller")
        isa = VectorParam.MipsISA([], "ISA instance")
    elif buildEnv['TARGET_ISA'] == 'arm':
        istage2_mmu = Param.ArmStage2MMU(ArmStage2IMMU(), "Stage 2 trans")
        dstage2_mmu = Param.ArmStage2MMU(ArmStage2DMMU(), "Stage 2 trans")
        interrupts = VectorParam.ArmInterrupts(
                [], "Interrupt Controller")
        isa = VectorParam.ArmISA([], "ISA instance")
    elif buildEnv['TARGET_ISA'] == 'power':
        UnifiedTLB = Param.Bool(True, "Is this a Unified TLB?")
        interrupts = VectorParam.PowerInterrupts(
                [], "Interrupt Controller")
        isa = VectorParam.PowerISA([], "ISA instance")
    elif buildEnv['TARGET_ISA'] == 'riscv':
        interrupts = VectorParam.RiscvInterrupts(
                [], "Interrupt Controller")
        isa = VectorParam.RiscvISA([], "ISA instance")
    else:
        print("Don't know what TLB to use for ISA %s" %
              buildEnv['TARGET_ISA'])
        sys.exit(1)

    max_insts_all_threads = Param.Counter(0,
        "terminate when all threads have reached this inst count")
    max_insts_any_thread = Param.Counter(0,
        "terminate when any thread reaches this inst count")
    simpoint_start_insts = VectorParam.Counter([],
        "starting instruction counts of simpoints")
    max_loads_all_threads = Param.Counter(0,
        "terminate when all threads have reached this load count")
    max_loads_any_thread = Param.Counter(0,
        "terminate when any thread reaches this load count")
    progress_interval = Param.Frequency('0Hz',
        "frequency to print out the progress message")

    switched_out = Param.Bool(False,
        "Leave the CPU switched out after startup (used when switching " \
        "between CPU models)")

    tracer = Param.InstTracer(default_tracer, "Instruction tracer")

    icache_port = MasterPort("Instruction Port")
    dcache_port = MasterPort("Data Port")
    _cached_ports = ['icache_port', 'dcache_port']

    if buildEnv['TARGET_ISA'] in ['x86', 'arm']:
        _cached_ports += ["itb.walker.port", "dtb.walker.port"]

    _uncached_slave_ports = []
    _uncached_master_ports = []
    if buildEnv['TARGET_ISA'] == 'x86':
        _uncached_slave_ports += ["interrupts[0].pio",
                                  "interrupts[0].int_slave"]
        _uncached_master_ports += ["interrupts[0].int_master"]

    def createInterruptController(self):
        if buildEnv['TARGET_ISA'] == 'sparc':
            self.interrupts = [SparcInterrupts() for i in xrange(self.numThreads)]
        elif buildEnv['TARGET_ISA'] == 'alpha':
            self.interrupts = [AlphaInterrupts() for i in xrange(self.numThreads)]
        elif buildEnv['TARGET_ISA'] == 'x86':
            self.apic_clk_domain = DerivedClockDomain(clk_domain =
                                                      Parent.clk_domain,
                                                      clk_divider = 16)
            self.interrupts = [X86LocalApic(clk_domain = self.apic_clk_domain,
                                           pio_addr=0x2000000000000000)
                               for i in xrange(self.numThreads)]
            _localApic = self.interrupts
        elif buildEnv['TARGET_ISA'] == 'mips':
            self.interrupts = [MipsInterrupts() for i in xrange(self.numThreads)]
        elif buildEnv['TARGET_ISA'] == 'arm':
            self.interrupts = [ArmInterrupts() for i in xrange(self.numThreads)]
        elif buildEnv['TARGET_ISA'] == 'power':
            self.interrupts = [PowerInterrupts() for i in xrange(self.numThreads)]
        elif buildEnv['TARGET_ISA'] == 'riscv':
            self.interrupts = \
                [RiscvInterrupts() for i in xrange(self.numThreads)]
        else:
            print("Don't know what Interrupt Controller to use for ISA %s" %
                  buildEnv['TARGET_ISA'])
            sys.exit(1)

    def connectCachedPorts(self, bus):
        for p in self._cached_ports:
            exec('self.%s = bus.slave' % p)

    def connectUncachedPorts(self, bus):
        for p in self._uncached_slave_ports:
            exec('self.%s = bus.master' % p)
        for p in self._uncached_master_ports:
            exec('self.%s = bus.slave' % p)

    def connectAllPorts(self, cached_bus, uncached_bus = None):
        self.connectCachedPorts(cached_bus)
        if not uncached_bus:
            uncached_bus = cached_bus
        self.connectUncachedPorts(uncached_bus)

    def addPrivateSplitL1Caches(self, ic, dc, iwc = None, dwc = None):
        self.icache = ic
        self.dcache = dc
        self.icache_port = ic.cpu_side
        self.dcache_port = dc.cpu_side
        self._cached_ports = ['icache.mem_side', 'dcache.mem_side']
        if buildEnv['TARGET_ISA'] in ['x86', 'arm']:
            if iwc and dwc:
                self.itb_walker_cache = iwc
                self.dtb_walker_cache = dwc
                self.itb.walker.port = iwc.cpu_side
                self.dtb.walker.port = dwc.cpu_side
                self._cached_ports += ["itb_walker_cache.mem_side", \
                                       "dtb_walker_cache.mem_side"]
            else:
                self._cached_ports += ["itb.walker.port", "dtb.walker.port"]

            # Checker doesn't need its own tlb caches because it does
            # functional accesses only
            if self.checker != NULL:
                self._cached_ports += ["checker.itb.walker.port", \
                                       "checker.dtb.walker.port"]

    def addTwoLevelCacheHierarchy(self, ic, dc, l2c, iwc=None, dwc=None,
                                  xbar=None):
        self.addPrivateSplitL1Caches(ic, dc, iwc, dwc)
        self.toL2Bus = xbar if xbar else L2XBar()
        self.connectCachedPorts(self.toL2Bus)
        self.l2cache = l2c
        self.toL2Bus.master = self.l2cache.cpu_side
        self._cached_ports = ['l2cache.mem_side']

    def createThreads(self):
        # If no ISAs have been created, assume that the user wants the
        # default ISA.
        if len(self.isa) == 0:
            self.isa = [ default_isa_class() for i in xrange(self.numThreads) ]
        else:
            if len(self.isa) != int(self.numThreads):
                raise RuntimeError("Number of ISA instances doesn't "
                                   "match thread count")
        if self.checker != NULL:
            self.checker.createThreads()

    def addCheckerCpu(self):
        pass

    def createPhandleKey(self, thread):
        # This method creates a unique key for this cpu as a function of a
        # certain thread
        return 'CPU-%d-%d-%d' % (self.socket_id, self.cpu_id, thread)

    #Generate simple CPU Device Tree structure
    def generateDeviceTree(self, state):
        """Generate cpu nodes for each thread and the corresponding part of the
        cpu-map node. Note that this implementation does not support clusters
        of clusters. Note that GEM5 is not compatible with the official way of
        numbering cores as defined in the Device Tree documentation. Where the
        cpu_id needs to reset to 0 for each cluster by specification, GEM5
        expects the cpu_id to be globally unique and incremental. This
        generated node adheres the GEM5 way of doing things."""
        if bool(self.switched_out):
            return

        cpus_node = FdtNode('cpus')
        cpus_node.append(state.CPUCellsProperty())
        #Special size override of 0
        cpus_node.append(FdtPropertyWords('#size-cells', [0]))

        # Generate cpu nodes
        for i in range(int(self.numThreads)):
            reg = (int(self.socket_id)<<8) + int(self.cpu_id) + i
            node = FdtNode("cpu@%x" % reg)
            node.append(FdtPropertyStrings("device_type", "cpu"))
            node.appendCompatible(["gem5,arm-cpu"])
            node.append(FdtPropertyWords("reg", state.CPUAddrCells(reg)))
            platform, found = self.system.unproxy(self).find_any(Platform)
            if found:
                platform.annotateCpuDeviceNode(node, state)
            else:
                warn("Platform not found for device tree generation; " \
                     "system or multiple CPUs may not start")

            freq = round(self.clk_domain.unproxy(self).clock[0].frequency)
            node.append(FdtPropertyWords("clock-frequency", freq))

            # Unique key for this CPU
            phandle_key = self.createPhandleKey(i)
            node.appendPhandle(phandle_key)
            cpus_node.append(node)

        yield cpus_node
