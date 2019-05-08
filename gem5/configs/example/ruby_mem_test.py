# Copyright (c) 2006-2007 The Regents of The University of Michigan
# Copyright (c) 2009 Advanced Micro Devices, Inc.
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
# Authors: Ron Dreslinski
#          Brad Beckmann

from __future__ import print_function

import m5
from m5.objects import *
from m5.defines import buildEnv
from m5.util import addToPath
import os, optparse, sys

addToPath('../')

from common import Options
from ruby import Ruby

# Get paths we might need.  It's expected this file is in m5/configs/example.
config_path = os.path.dirname(os.path.abspath(__file__))
config_root = os.path.dirname(config_path)

parser = optparse.OptionParser()
Options.addNoISAOptions(parser)

parser.add_option("--maxloads", metavar="N", default=0,
                  help="Stop after N loads")
parser.add_option("--progress", type="int", default=1000,
                  metavar="NLOADS",
                  help="Progress message interval "
                  "[default: %default]")
parser.add_option("--num-dmas", type="int", default=0, help="# of dma testers")
parser.add_option("--functional", type="int", default=0,
                  help="percentage of accesses that should be functional")
parser.add_option("--suppress-func-warnings", action="store_true",
                  help="suppress warnings when functional accesses fail")

#
# Add the ruby specific and protocol specific options
#
Ruby.define_options(parser)

execfile(os.path.join(config_root, "common", "Options.py"))

(options, args) = parser.parse_args()

#
# Set the default cache size and associativity to be very small to encourage
# races between requests and writebacks.
#
options.l1d_size="256B"
options.l1i_size="256B"
options.l2_size="512B"
options.l3_size="1kB"
options.l1d_assoc=2
options.l1i_assoc=2
options.l2_assoc=2
options.l3_assoc=2

if args:
     print("Error: script doesn't take any positional arguments")
     sys.exit(1)

block_size = 64

if options.num_cpus > block_size:
     print("Error: Number of testers %d limited to %d because of false sharing"
           % (options.num_cpus, block_size))
     sys.exit(1)

#
# Currently ruby does not support atomic or uncacheable accesses
#
cpus = [ MemTest(atomic = False,
                 max_loads = options.maxloads,
                 issue_dmas = False,
                 percent_functional = options.functional,
                 percent_uncacheable = 0,
                 progress_interval = options.progress,
                 suppress_func_warnings = options.suppress_func_warnings) \
         for i in xrange(options.num_cpus) ]

system = System(cpu = cpus,
                funcmem = SimpleMemory(in_addr_map = False),
                funcbus = IOXBar(),
                clk_domain = SrcClockDomain(clock = options.sys_clock),
                mem_ranges = [AddrRange(options.mem_size)])

if options.num_dmas > 0:
    dmas = [ MemTest(atomic = False,
                     max_loads = options.maxloads,
                     issue_dmas = True,
                     percent_functional = 0,
                     percent_uncacheable = 0,
                     progress_interval = options.progress,
                     suppress_func_warnings =
                                        not options.suppress_func_warnings) \
             for i in xrange(options.num_dmas) ]
    system.dma_devices = dmas
else:
    dmas = []

dma_ports = []
for (i, dma) in enumerate(dmas):
    dma_ports.append(dma.test)
Ruby.create_system(options, False, system, dma_ports = dma_ports)

# Create a top-level voltage domain and clock domain
system.voltage_domain = VoltageDomain(voltage = options.sys_voltage)
system.clk_domain = SrcClockDomain(clock = options.sys_clock,
                                   voltage_domain = system.voltage_domain)
# Create a seperate clock domain for Ruby
system.ruby.clk_domain = SrcClockDomain(clock = options.ruby_clock,
                                        voltage_domain = system.voltage_domain)

#
# The tester is most effective when randomization is turned on and
# artifical delay is randomly inserted on messages
#
system.ruby.randomization = True

assert(len(cpus) == len(system.ruby._cpu_ports))

for (i, cpu) in enumerate(cpus):
    #
    # Tie the cpu memtester ports to the correct system ports
    #
    cpu.test = system.ruby._cpu_ports[i].slave
    cpu.functional = system.funcbus.slave

    #
    # Since the memtester is incredibly bursty, increase the deadlock
    # threshold to 5 million cycles
    #
    system.ruby._cpu_ports[i].deadlock_threshold = 5000000

for (i, dma) in enumerate(dmas):
    #
    # Tie the dma memtester ports to the correct functional port
    # Note that the test port has already been connected to the dma_sequencer
    #
    dma.functional = system.funcbus.slave

# connect reference memory to funcbus
system.funcbus.master = system.funcmem.port

# -----------------------
# run simulation
# -----------------------

root = Root( full_system = False, system = system )
root.system.mem_mode = 'timing'

# Not much point in this being higher than the L1 latency
m5.ticks.setGlobalFrequency('1ns')

# instantiate configuration
m5.instantiate()

# simulate until program terminates
exit_event = m5.simulate(options.abs_max_tick)

print('Exiting @ tick', m5.curTick(), 'because', exit_event.getCause())
