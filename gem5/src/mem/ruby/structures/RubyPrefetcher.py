# Copyright (c) 2012 Mark D. Hill and David A. Wood
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
# Authors: Nilay Vaish

from m5.SimObject import SimObject
from System import System
from m5.params import *
from m5.proxy import *

class Prefetcher(SimObject):
    type = 'Prefetcher'
    cxx_class = 'Prefetcher'
    cxx_header = "mem/ruby/structures/Prefetcher.hh"

    num_streams = Param.UInt32(4,
        "Number of prefetch streams to be allocated")
    pf_per_stream = Param.UInt32(1, "Number of prefetches per stream")
    unit_filter  = Param.UInt32(8,
        "Number of entries in the unit filter array")
    nonunit_filter = Param.UInt32(8,
        "Number of entries in the non-unit filter array")
    train_misses = Param.UInt32(4, "")
    num_startup_pfs = Param.UInt32(1, "")
    cross_page = Param.Bool(False, """True if prefetched address can be on a
            page different from the observed address""")
    sys = Param.System(Parent.any, "System this prefetcher belongs to")
