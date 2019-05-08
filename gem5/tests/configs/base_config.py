# Copyright (c) 2012-2013, 2017-2018 ARM Limited
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
# Authors: Andreas Sandberg
#          Andreas Hansson

from abc import ABCMeta, abstractmethod
import optparse
import m5
from m5.objects import *
from m5.proxy import *
m5.util.addToPath('../configs/')
from common import FSConfig
from common import Options
from common.Caches import *
from ruby import Ruby

_have_kvm_support = 'BaseKvmCPU' in globals()

class BaseSystem(object):
    """Base system builder.

    This class provides some basic functionality for creating an ARM
    system with the usual peripherals (caches, GIC, etc.). It allows
    customization by defining separate methods for different parts of
    the initialization process.
    """

    __metaclass__ = ABCMeta

    def __init__(self, mem_mode='timing', mem_class=SimpleMemory,
                 cpu_class=TimingSimpleCPU, num_cpus=1, num_threads=1,
                 checker=False, mem_size=None, use_ruby=False):
        """Initialize a simple base system.

        Keyword Arguments:
          mem_mode -- String describing the memory mode (timing or atomic)
          mem_class -- Memory controller class to use
          cpu_class -- CPU class to use
          num_cpus -- Number of CPUs to instantiate
          checker -- Set to True to add checker CPUs
          mem_size -- Override the default memory size
          use_ruby -- Set to True to use ruby memory
        """
        self.mem_mode = mem_mode
        self.mem_class = mem_class
        self.cpu_class = cpu_class
        self.num_cpus = num_cpus
        self.num_threads = num_threads
        self.checker = checker
        self.use_ruby = use_ruby

    def create_cpus(self, cpu_clk_domain):
        """Return a list of CPU objects to add to a system."""
        cpus = [ self.cpu_class(clk_domain=cpu_clk_domain,
                                numThreads=self.num_threads,
                                cpu_id=i)
                 for i in range(self.num_cpus) ]
        if self.checker:
            for c in cpus:
                c.addCheckerCpu()
        return cpus

    def create_caches_private(self, cpu):
        """Add private caches to a CPU.

        Arguments:
          cpu -- CPU instance to work on.
        """
        cpu.addPrivateSplitL1Caches(L1_ICache(size='32kB', assoc=1),
                                    L1_DCache(size='32kB', assoc=4))

    def create_caches_shared(self, system):
        """Add shared caches to a system.

        Arguments:
          system -- System to work on.

        Returns:
          A bus that CPUs should use to connect to the shared cache.
        """
        system.toL2Bus = L2XBar(clk_domain=system.cpu_clk_domain)
        system.l2c = L2Cache(clk_domain=system.cpu_clk_domain,
                             size='4MB', assoc=8)
        system.l2c.cpu_side = system.toL2Bus.master
        system.l2c.mem_side = system.membus.slave
        return system.toL2Bus

    def init_cpu(self, system, cpu, sha_bus):
        """Initialize a CPU.

        Arguments:
          system -- System to work on.
          cpu -- CPU to initialize.
        """
        if not cpu.switched_out:
            self.create_caches_private(cpu)
            cpu.createInterruptController()
            cpu.connectAllPorts(sha_bus if sha_bus != None else system.membus,
                                system.membus)

    def init_kvm(self, system):
        """Do KVM-specific system initialization.

        Arguments:
          system -- System to work on.
        """
        system.vm = KvmVM()

    def init_system(self, system):
        """Initialize a system.

        Arguments:
          system -- System to initialize.
        """
        self.create_clk_src(system)
        system.cpu = self.create_cpus(system.cpu_clk_domain)

        if _have_kvm_support and \
                any([isinstance(c, BaseKvmCPU) for c in system.cpu]):
            self.init_kvm(system)

        if self.use_ruby:
            # Add the ruby specific and protocol specific options
            parser = optparse.OptionParser()
            Options.addCommonOptions(parser)
            Ruby.define_options(parser)
            (options, args) = parser.parse_args()

            # Set the default cache size and associativity to be very
            # small to encourage races between requests and writebacks.
            options.l1d_size="32kB"
            options.l1i_size="32kB"
            options.l2_size="4MB"
            options.l1d_assoc=4
            options.l1i_assoc=2
            options.l2_assoc=8
            options.num_cpus = self.num_cpus
            options.num_dirs = 2

            bootmem = getattr(system, 'bootmem', None)
            Ruby.create_system(options, True, system, system.iobus,
                               system._dma_ports, bootmem)

            # Create a seperate clock domain for Ruby
            system.ruby.clk_domain = SrcClockDomain(
                clock = options.ruby_clock,
                voltage_domain = system.voltage_domain)
            for i, cpu in enumerate(system.cpu):
                if not cpu.switched_out:
                    cpu.createInterruptController()
                    cpu.connectCachedPorts(system.ruby._cpu_ports[i])
        else:
            sha_bus = self.create_caches_shared(system)
            for cpu in system.cpu:
                self.init_cpu(system, cpu, sha_bus)


    def create_clk_src(self,system):
        # Create system clock domain. This provides clock value to every
        # clocked object that lies beneath it unless explicitly overwritten
        # by a different clock domain.
        system.voltage_domain = VoltageDomain()
        system.clk_domain = SrcClockDomain(clock = '1GHz',
                                           voltage_domain =
                                           system.voltage_domain)

        # Create a seperate clock domain for components that should
        # run at CPUs frequency
        system.cpu_clk_domain = SrcClockDomain(clock = '2GHz',
                                               voltage_domain =
                                               system.voltage_domain)

    @abstractmethod
    def create_system(self):
        """Create an return an initialized system."""
        pass

    @abstractmethod
    def create_root(self):
        """Create and return a simulation root using the system
        defined by this class."""
        pass

class BaseSESystem(BaseSystem):
    """Basic syscall-emulation builder."""

    def __init__(self, **kwargs):
        BaseSystem.__init__(self, **kwargs)

    def init_system(self, system):
        BaseSystem.init_system(self, system)

    def create_system(self):
        system = System(physmem = self.mem_class(),
                        membus = SystemXBar(),
                        mem_mode = self.mem_mode,
                        multi_thread = (self.num_threads > 1))
        if not self.use_ruby:
            system.system_port = system.membus.slave
        system.physmem.port = system.membus.master
        self.init_system(system)
        return system

    def create_root(self):
        system = self.create_system()
        m5.ticks.setGlobalFrequency('1THz')
        return Root(full_system=False, system=system)

class BaseSESystemUniprocessor(BaseSESystem):
    """Basic syscall-emulation builder for uniprocessor systems.

    Note: This class is only really needed to provide backwards
    compatibility in existing test cases.
    """

    def __init__(self, **kwargs):
        BaseSESystem.__init__(self, **kwargs)

    def create_caches_private(self, cpu):
        # The atomic SE configurations do not use caches
        if self.mem_mode == "timing":
            # @todo We might want to revisit these rather enthusiastic L1 sizes
            cpu.addTwoLevelCacheHierarchy(L1_ICache(size='128kB'),
                                          L1_DCache(size='256kB'),
                                          L2Cache(size='2MB'))

    def create_caches_shared(self, system):
        return None

class BaseFSSystem(BaseSystem):
    """Basic full system builder."""

    def __init__(self, **kwargs):
        BaseSystem.__init__(self, **kwargs)

    def init_system(self, system):
        BaseSystem.init_system(self, system)

        if self.use_ruby:
            # Connect the ruby io port to the PIO bus,
            # assuming that there is just one such port.
            system.iobus.master = system.ruby._io_port.slave
        else:
            # create the memory controllers and connect them, stick with
            # the physmem name to avoid bumping all the reference stats
            system.physmem = [self.mem_class(range = r)
                              for r in system.mem_ranges]
            system.llc = [NoncoherentCache(addr_ranges = [r],
                                           size = '4kB',
                                           assoc = 2,
                                           mshrs = 128,
                                           tag_latency = 10,
                                           data_latency = 10,
                                           sequential_access = True,
                                           response_latency = 20,
                                           tgts_per_mshr = 8)
                          for r in system.mem_ranges]
            for i in xrange(len(system.physmem)):
                system.physmem[i].port = system.llc[i].mem_side
                system.llc[i].cpu_side = system.membus.master

            # create the iocache, which by default runs at the system clock
            system.iocache = IOCache(addr_ranges=system.mem_ranges)
            system.iocache.cpu_side = system.iobus.master
            system.iocache.mem_side = system.membus.slave

    def create_root(self):
        system = self.create_system()
        m5.ticks.setGlobalFrequency('1THz')
        return Root(full_system=True, system=system)

class BaseFSSystemUniprocessor(BaseFSSystem):
    """Basic full system builder for uniprocessor systems.

    Note: This class is only really needed to provide backwards
    compatibility in existing test cases.
    """

    def __init__(self, **kwargs):
        BaseFSSystem.__init__(self, **kwargs)

    def create_caches_private(self, cpu):
        cpu.addTwoLevelCacheHierarchy(L1_ICache(size='32kB', assoc=1),
                                      L1_DCache(size='32kB', assoc=4),
                                      L2Cache(size='4MB', assoc=8))

    def create_caches_shared(self, system):
        return None

class BaseFSSwitcheroo(BaseFSSystem):
    """Uniprocessor system prepared for CPU switching"""

    def __init__(self, cpu_classes, **kwargs):
        BaseFSSystem.__init__(self, **kwargs)
        self.cpu_classes = tuple(cpu_classes)

    def create_cpus(self, cpu_clk_domain):
        cpus = [ cclass(clk_domain = cpu_clk_domain,
                        cpu_id=0,
                        switched_out=True)
                 for cclass in self.cpu_classes ]
        cpus[0].switched_out = False
        return cpus
