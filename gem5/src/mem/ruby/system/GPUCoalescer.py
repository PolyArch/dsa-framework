# Copyright (c) 2015 Advanced Micro Devices, Inc.
# All rights reserved.
#
# For use for simulation and test purposes only
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Steve Reinhardt
#          Brad Beckmann

from m5.params import *
from m5.proxy import *
from Sequencer import *

class RubyGPUCoalescer(RubyPort):
   type = 'RubyGPUCoalescer'
   cxx_class = 'GPUCoalescer'
   cxx_header = "mem/ruby/system/GPUCoalescer.hh"

   # max_outstanding_requests = (wave front slots) x (wave front size)
   max_outstanding_requests = Param.Int(40*64,
                                "max requests (incl. prefetches) outstanding")
   assume_rfo = Param.Bool(True, "assume protocol implementes Read for "
                           "Ownership coherence");

   icache = Param.RubyCache("")
   dcache = Param.RubyCache("")
   deadlock_threshold = Param.Cycles(500000,
       "max outstanding cycles for a request before " \
       "deadlock/livelock declared")
   garnet_standalone = Param.Bool(False, "")
   dcache_hit_latency = Param.Cycles(1, "Data cache hit latency")
