/*
 * Copyright (c) 2008 The Regents of The University of Michigan
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
 * Authors: Gabe Black
 */

#include "dev/x86/speaker.hh"

#include "base/bitunion.hh"
#include "base/trace.hh"
#include "debug/PcSpeaker.hh"
#include "dev/x86/i8254.hh"
#include "mem/packet.hh"
#include "mem/packet_access.hh"

Tick
X86ISA::Speaker::read(PacketPtr pkt)
{
    assert(pkt->getAddr() == pioAddr);
    assert(pkt->getSize() == 1);
    controlVal.timer = timer->outputHigh(2) ? 1 : 0;
    DPRINTF(PcSpeaker,
            "Reading from speaker device: gate %s, speaker %s, output %s.\n",
            controlVal.gate ? "on" : "off",
            controlVal.speaker ? "on" : "off",
            controlVal.timer ? "on" : "off");
    pkt->set((uint8_t)controlVal);
    pkt->makeAtomicResponse();
    return latency;
}

Tick
X86ISA::Speaker::write(PacketPtr pkt)
{
    assert(pkt->getAddr() == pioAddr);
    assert(pkt->getSize() == 1);
    SpeakerControl val = pkt->get<uint8_t>();
    controlVal.gate = val.gate;
    //Change the gate value in the timer.
    if (!val.gate)
        warn("The gate bit of the pc speaker isn't implemented and "
                "is always on.\n");
    //This would control whether the timer output is hooked up to a physical
    //speaker. Since M5 can't make noise, it's value doesn't actually do
    //anything.
    controlVal.speaker = val.speaker;
    DPRINTF(PcSpeaker, "Writing to speaker device: gate %s, speaker %s.\n",
            controlVal.gate ? "on" : "off", controlVal.speaker ? "on" : "off");
    pkt->makeAtomicResponse();
    return latency;
}

void
X86ISA::Speaker::serialize(CheckpointOut &cp) const
{
    SERIALIZE_SCALAR(controlVal);
}

void
X86ISA::Speaker::unserialize(CheckpointIn &cp)
{
    UNSERIALIZE_SCALAR(controlVal);
}

X86ISA::Speaker *
PcSpeakerParams::create()
{
    return new X86ISA::Speaker(this);
}
