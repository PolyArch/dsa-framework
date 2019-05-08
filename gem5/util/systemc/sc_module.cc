/*
 * Copyright (c) 2014 ARM Limited
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
 * Copyright (c) 2006 The Regents of The University of Michigan
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
 * Copyright (c) 2013 Mark D. Hill and David A. Wood
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
 * Authors: Nathan Binkert
 *          Steve Reinhardt
 *          Andrew Bardsley
 *          Matthias Jung
 *          Christian Menard
 */

/**
 * @file
 *
 * Defines an sc_module type to wrap a gem5 simulation.  The 'evaluate'
 * thread on that module implements the gem5 event loop.
 *
 * This currently only supports a single event queue and strictly
 * cooperatively threaded SystemC threads and so there should be at
 * most one Gem5Module instantiated in any simulation.
 */

#include "base/logging.hh"
#include "base/pollevent.hh"
#include "base/trace.hh"
#include "debug/Event.hh"
#include "sc_module.hh"
#include "sim/async.hh"
#include "sim/core.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/stat_control.hh"

namespace Gem5SystemC
{

/** There are assumptions throughout Gem5SystemC file that a tick is 1ps.
 *  Make this the case */
void
setTickFrequency()
{
    ::setClockFrequency(1000000000000);
}

Module::Module(sc_core::sc_module_name name) : sc_core::sc_channel(name),
    in_simulate(false)
{
    SC_METHOD(eventLoop);
    sensitive << eventLoopEnterEvent;
    dont_initialize();

    SC_METHOD(serviceExternalEvent);
    sensitive << externalSchedulingEvent;
    dont_initialize();
}

void
Module::SCEventQueue::wakeup(Tick when)
{
    DPRINTF(Event, "waking up SCEventQueue\n");
    /* Don't bother to use 'when' for now */
    module.notify();
}

void
Module::setupEventQueues(Module &module)
{
    fatal_if(mainEventQueue.size() != 0,
        "Gem5SystemC::Module::setupEventQueues must be called"
        " before any gem5 event queues are set up");

    numMainEventQueues = 1;
    mainEventQueue.push_back(new SCEventQueue("events", module));
    curEventQueue(getEventQueue(0));
}

void
Module::catchup()
{
    EventQueue *eventq = getEventQueue(0);
    Tick systemc_time = sc_core::sc_time_stamp().value();
    Tick gem5_time = curTick();

    /* gem5 time *must* lag SystemC as SystemC is the master */
    fatal_if(gem5_time > systemc_time, "gem5 time must lag SystemC time"
        " gem5: %d SystemC: %d", gem5_time, systemc_time);

    eventq->setCurTick(systemc_time);

    if (!eventq->empty()) {
        Tick next_event_time M5_VAR_USED = eventq->nextTick();

        fatal_if(gem5_time > next_event_time,
            "Missed an event at time %d gem5: %d, SystemC: %d",
            next_event_time, gem5_time, systemc_time);
    }
}

void
Module::notify(sc_core::sc_time time_from_now)
{
    externalSchedulingEvent.notify(time_from_now);
}

void
Module::serviceAsyncEvent()
{
    EventQueue *eventq = getEventQueue(0);
    std::lock_guard<EventQueue> lock(*eventq);

    assert(async_event);

    /* Catch up gem5 time with SystemC time so that any event here won't
     * be in the past relative to the current time */
    Tick systemc_time = sc_core::sc_time_stamp().value();

    /* Move time on to match SystemC */
    catchup();

    async_event = false;
    if (async_statdump || async_statreset) {
        Stats::schedStatEvent(async_statdump, async_statreset);
        async_statdump = false;
        async_statreset = false;
    }

    if (async_exit) {
        async_exit = false;
        exitSimLoop("user interrupt received");
    }

    if (async_io) {
        async_io = false;
        pollQueue.service();
    }

    if (async_exception)
        fatal("received async_exception, shouldn't be possible");
}

void
Module::serviceExternalEvent()
{
    EventQueue *eventq = getEventQueue(0);

    if (!in_simulate && !async_event)
        warn("Gem5SystemC external event received while not in simulate");

    if (async_event)
        serviceAsyncEvent();

    if (in_simulate && !eventq->empty())
        eventLoop();
}

void
Module::eventLoop()
{
    EventQueue *eventq = getEventQueue(0);

    fatal_if(!in_simulate, "Gem5SystemC event loop entered while"
        " outside Gem5SystemC::Module::simulate");

    if (async_event)
        serviceAsyncEvent();

    while (!eventq->empty()) {
        Tick next_event_time = eventq->nextTick();

        /* Move time on to match SystemC */
        catchup();

        Tick gem5_time = curTick();

        /* Woken up early */
        if (wait_exit_time > sc_core::sc_time_stamp().value()) {
            DPRINTF(Event, "Woken up early\n");
            wait_exit_time = sc_core::sc_time_stamp().value();
        }

        if (gem5_time < next_event_time) {
            Tick wait_period = next_event_time - gem5_time;
            wait_exit_time = gem5_time + wait_period;

            DPRINTF(Event, "Waiting for %d ticks for next gem5 event\n",
                wait_period);

            /* The next event is scheduled in the future, wait until
             *  then or until externalSchedulingEvent */
            eventLoopEnterEvent.notify(sc_core::sc_time::from_value(
                sc_dt::uint64(wait_period)));

            return;
        } else if (gem5_time > next_event_time) {
            Tick systemc_time = sc_core::sc_time_stamp().value();

            /* Missed event, for some reason the above test didn't work
             *  or an event was scheduled in the past */
            fatal("Missed an event at time %d gem5: %d, SystemC: %d",
                next_event_time, gem5_time, systemc_time);
        } else {
            /* Service an event */
            exitEvent = eventq->serviceOne();

            if (exitEvent) {
                eventLoopExitEvent.notify(sc_core::SC_ZERO_TIME);
                return;
            }
        }
    }

    fatal("Ran out of events without seeing exit event");
}

GlobalSimLoopExitEvent *
Module::simulate(Tick num_cycles)
{
    inform("Entering event queue @ %d.  Starting simulation...", curTick());

    if (num_cycles < MaxTick - curTick())
        num_cycles = curTick() + num_cycles;
    else /* counter would roll over or be set to MaxTick anyhow */
        num_cycles = MaxTick;

    GlobalEvent *limit_event = new GlobalSimLoopExitEvent(num_cycles,
        "simulate() limit reached", 0, 0);

    exitEvent = NULL;

    /* Cancel any outstanding events */
    eventLoopExitEvent.cancel();
    externalSchedulingEvent.cancel();

    in_simulate = true;
    eventLoopEnterEvent.notify(sc_core::SC_ZERO_TIME);

    /* Wait for event queue to exit, guarded by exitEvent just incase
     *  it already has exited and we don't want to completely rely
     *  on notify semantics */
    if (!exitEvent)
        wait(eventLoopExitEvent);

    /* Cancel any outstanding event loop entries */
    eventLoopEnterEvent.cancel();
    in_simulate = false;

    /* Locate the global exit event */
    BaseGlobalEvent *global_event = exitEvent->globalEvent();
    assert(global_event != NULL);

    GlobalSimLoopExitEvent *global_exit_event =
        dynamic_cast<GlobalSimLoopExitEvent *>(global_event);
    assert(global_exit_event != NULL);

    if (global_exit_event != limit_event) {
        limit_event->deschedule();
        delete limit_event;
    }

    return global_exit_event;
}

}
