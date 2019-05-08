/*
 * Copyright (c) 2011 ARM Limited
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
 * Authors: Andreas Hansson
 */

/**
 * @file
 * RobyPortProxy for connecting system port to Ruby
 *
 * A trivial wrapper that allows the system port to connect to Ruby
 * and use nothing but functional accesses.
 */

#ifndef __MEM_RUBY_SYSTEM_RUBYPORTPROXY_HH__
#define __MEM_RUBY_SYSTEM_RUBYPORTPROXY_HH__

#include "mem/ruby/system/RubyPort.hh"
#include "params/RubyPortProxy.hh"

class RubyPortProxy : public RubyPort
{

  public:

    /**
     * Create a new RubyPortProxy.
     *
     * @param p Parameters inherited from the RubyPort
     */
    RubyPortProxy(const RubyPortProxyParams* p);

    /**
     * Destruct a RubyPortProxy.
     */
    virtual ~RubyPortProxy();

    /**
     * Initialise a RubyPortProxy by doing nothing and avoid
     * involving the super class.
     */
    void init();

    /**
     * Pure virtual member in the super class that we are forced to
     * implement even if it is never used (since there are only
     * functional accesses).
     *
     * @param pkt The packet to serve to Ruby
     * @returns always a NULL status
     */
    RequestStatus makeRequest(PacketPtr pkt);

    /**
     * Pure virtual member in the super class that we are forced to
     * implement even if it is never used (since there are only
     * functional accesses).
     *
     * @returns always 0
     */
    int outstandingCount() const { return 0; }

    /**
     * Pure virtual member in the super class that we are forced to
     * implement even if it is never used (since there are only
     * functional accesses).
     *
     * @returns always false
     */
    bool isDeadlockEventScheduled() const { return false; }

    /**
     * Pure virtual member in the super class that we are forced to
     * implement even if it is never used (since there are only
     * functional accesses).
     */
    void descheduleDeadlockEvent() { }

};

#endif // __MEM_RUBY_SYSTEM_RUBYPORTPROXY_HH__
