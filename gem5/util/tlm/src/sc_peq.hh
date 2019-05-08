/*
 * Copyright (c) 2015, University of Kaiserslautern
 * Copyright (c) 2016, Dresden University of Technology (TU Dresden)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Matthias Jung
 *          Christian Menard
 */

#ifndef PAYLOAD_EVENT_H_
#define PAYLOAD_EVENT_H_

// TLM includes
#include <tlm.h>

// gem5 includes
#include <sim/eventq.hh>

namespace Gem5SystemC {
/**
 * A 'Fake Payload Event Queue', similar to the TLM PEQs. This helps the
 * transactors to schedule events in gem5.
 */
template <typename OWNER>
class PayloadEvent : public Event
{
  public:
    OWNER& port;
    const std::string eventName;
    void (OWNER::*handler)(PayloadEvent<OWNER>* pe,
                           tlm::tlm_generic_payload& trans,
                           const tlm::tlm_phase& phase);

  protected:
    tlm::tlm_generic_payload* t;
    tlm::tlm_phase p;

    void process() { (port.*handler)(this, *t, p); }

  public:
    const std::string name() const { return eventName; }

    PayloadEvent(OWNER& port_,
                 void (OWNER::*handler_)(PayloadEvent<OWNER>* pe,
                                         tlm::tlm_generic_payload& trans,
                                         const tlm::tlm_phase& phase),
                 const std::string& event_name)
      : port(port_)
      , eventName(event_name)
      , handler(handler_)
    {
    }

    /// Schedule an event into gem5
    void notify(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase,
                const sc_core::sc_time& delay)
    {
        assert(!scheduled());

        t = &trans;
        p = phase;

        /**
         * Get time from SystemC as this will always be more up to date
         * than gem5's
         */
        Tick nextEventTick = sc_core::sc_time_stamp().value() + delay.value();

        port.owner.wakeupEventQueue(nextEventTick);
        port.owner.schedule(this, nextEventTick);
    }
};
}

#endif
