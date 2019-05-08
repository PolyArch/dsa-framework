/*
 * Copyright (c) 2017-2018 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
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
 * Authors: Giacomo Travaglini
 */

#include "tarmac_tracer.hh"

#include <string>

#include "arch/arm/system.hh"
#include "cpu/base.hh"

namespace Trace {

std::string
TarmacContext::tarmacCpuName() const
{
    auto id = thread->getCpuPtr()->cpuId();
    return "cpu" + std::to_string(id);
}

TarmacTracer::TarmacTracer(const Params *p)
  : InstTracer(p),
    startTick(p->start_tick),
    endTick(p->end_tick)
{
    // Wrong parameter setting: The trace end happens before the
    // trace start.
    panic_if(startTick > endTick,
             "Tarmac start point: %lu is bigger than "
             "Tarmac end point: %lu\n", startTick, endTick);

    // By default cpu tracers in gem5 are not tracing faults
    // (exceptions).
    // This is not in compliance with the Tarmac specification:
    // instructions like SVC, SMC, HVC have to be traced.
    // Tarmac Tracer is then automatically enabling this behaviour.
    setDebugFlag("ExecFaulting");
}

InstRecord *
TarmacTracer::getInstRecord(Tick when, ThreadContext *tc,
                           const StaticInstPtr staticInst,
                           TheISA::PCState pc,
                           const StaticInstPtr macroStaticInst)
{
    // Check if we need to start tracing since we have passed the
    // tick start point.
    if (when < startTick || when > endTick)
        return nullptr;

    if (ArmSystem::highestELIs64(tc)) {
        // TarmacTracerV8
        return new TarmacTracerRecordV8(when, tc, staticInst, pc, *this,
                                        macroStaticInst);
    } else {
        // TarmacTracer
        return new TarmacTracerRecord(when, tc, staticInst, pc, *this,
                                      macroStaticInst);
    }
}

} // namespace Trace

Trace::TarmacTracer *
TarmacTracerParams::create()
{
    return new Trace::TarmacTracer(this);
}
