/**
 * Copyright (c) 2018 Inria
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
 * Authors: Daniel Carvalho
 */

#include "mem/cache/replacement_policies/mru_rp.hh"

#include <cassert>
#include <memory>

#include "params/MRURP.hh"

MRURP::MRURP(const Params *p)
    : BaseReplacementPolicy(p)
{
}

void
MRURP::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
const
{
    // Reset last touch timestamp
    std::static_pointer_cast<MRUReplData>(
        replacement_data)->lastTouchTick = Tick(0);
}

void
MRURP::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // Update last touch timestamp
    std::static_pointer_cast<MRUReplData>(
        replacement_data)->lastTouchTick = curTick();
}

void
MRURP::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // Set last touch timestamp
    std::static_pointer_cast<MRUReplData>(
        replacement_data)->lastTouchTick = curTick();
}

ReplaceableEntry*
MRURP::getVictim(const ReplacementCandidates& candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);

    // Visit all candidates to find victim
    ReplaceableEntry* victim = candidates[0];
    for (const auto& candidate : candidates) {
        // Update victim entry if necessary
        if (std::static_pointer_cast<MRUReplData>(
                    candidate->replacementData)->lastTouchTick >
                std::static_pointer_cast<MRUReplData>(
                    victim->replacementData)->lastTouchTick) {
            victim = candidate;
        }
    }

    return victim;
}

std::shared_ptr<ReplacementData>
MRURP::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new MRUReplData());
}

MRURP*
MRURPParams::create()
{
    return new MRURP(this);
}
