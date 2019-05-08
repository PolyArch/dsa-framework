# Copyright (c) 2012-2013 ARM Limited
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
# Authors: Prakash Ramrakhyani

from m5.params import *
from m5.proxy import *
from ClockedObject import ClockedObject

class BaseTags(ClockedObject):
    type = 'BaseTags'
    abstract = True
    cxx_header = "mem/cache/tags/base.hh"
    # Get the size from the parent (cache)
    size = Param.MemorySize(Parent.size, "capacity in bytes")

    # Get the block size from the parent (system)
    block_size = Param.Int(Parent.cache_line_size, "block size in bytes")

    # Get the tag lookup latency from the parent (cache)
    tag_latency = Param.Cycles(Parent.tag_latency,
                               "The tag lookup latency for this cache")

    # Get the RAM access latency from the parent (cache)
    data_latency = Param.Cycles(Parent.data_latency,
                               "The data access latency for this cache")

    # Get the warmup percentage from the parent (cache)
    warmup_percentage = Param.Percent(Parent.warmup_percentage,
        "Percentage of tags to be touched to warm up the cache")

    sequential_access = Param.Bool(Parent.sequential_access,
        "Whether to access tags and data sequentially")

class BaseSetAssoc(BaseTags):
    type = 'BaseSetAssoc'
    cxx_header = "mem/cache/tags/base_set_assoc.hh"

    # Get the cache associativity
    assoc = Param.Int(Parent.assoc, "associativity")

    # Get replacement policy from the parent (cache)
    replacement_policy = Param.BaseReplacementPolicy(
        Parent.replacement_policy, "Replacement policy")

class SectorTags(BaseTags):
    type = 'SectorTags'
    cxx_header = "mem/cache/tags/sector_tags.hh"

    # Get the cache associativity
    assoc = Param.Int(Parent.assoc, "associativity")

    # Number of sub-sectors (data blocks) per sector
    num_blocks_per_sector = Param.Int(1, "Number of sub-sectors per sector");

    # Get replacement policy from the parent (cache)
    replacement_policy = Param.BaseReplacementPolicy(
        Parent.replacement_policy, "Replacement policy")

class FALRU(BaseTags):
    type = 'FALRU'
    cxx_class = 'FALRU'
    cxx_header = "mem/cache/tags/fa_lru.hh"

    min_tracked_cache_size = Param.MemorySize("128kB", "Minimum cache size for"
                                              " which we track statistics")
