/*
 * Copyright (c) 2012-2014 ARM Limited
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
 * Authors: Andrew Bardsley
 */

/**
 * @file
 *
 * ExternalSlave is a memory object representing a binding from
 * a gem5 master to a slave port in a system external to gem5.
 *
 * During initialisation, a `handler' for the port type specified in the
 * port's port_type parameter is found from the registered port handlers
 * provided with registerHandler.  Once a handler is found, it is passed the
 * port_data parameter of the port which can be used to identify the external
 * port which is to be bound to.  A port handler will usually construct a
 * bridge object in the external system to accomodate the port-to-port
 * mapping but this bridge is not exposed to gem5 other than be the
 * presentation of the SlavePort which can be bound.
 *
 * The external port must provide a gem5 SlavePort interface (with the
 * exception of getAddrRanges which is provided by the ExternalSlave
 * object).
 */

#ifndef __MEM_EXTERNAL_SLAVE_HH__
#define __MEM_EXTERNAL_SLAVE_HH__

#include "mem/mem_object.hh"
#include "params/ExternalSlave.hh"

class ExternalSlave : public MemObject
{
  public:
    /** Derive from this class to create an external port interface */
    class Port : public SlavePort
    {
      protected:
        ExternalSlave &owner;

      public:
        Port(const std::string &name_,
            ExternalSlave &owner_) :
            SlavePort(name_, &owner_), owner(owner_)
        { }

        ~Port() { }

        /** Any or all of recv... can be overloaded to provide the port's
         *  functionality */

        AddrRangeList getAddrRanges() const;
    };

    /* Handlers are specific to *types* of port not specific port
     * instantiations.  A handler will typically build a bridge to the
     * external port from gem5 and provide gem5 with a SlavePort that can be
     * bound to for each call to Handler::getExternalPort.*/
    class Handler
    {
      public:
        /** Create or find an external port which can be bound.  Returns
         *  NULL on failure */
        virtual Port *getExternalPort(
            const std::string &name, ExternalSlave &owner,
            const std::string &port_data) = 0;
    };

  protected:
    /** The peer port for the gem5 port "port" */
    Port *externalPort;

    /** Name of the bound port.  This will be name() + ".port" */
    std::string portName;

    /** Key to select a port handler */
    std::string portType;

    /** Handler-specific port configuration */
    std::string portData;

    /** The Range of addresses supported by the devices on the external
     *  side of this port */
    AddrRangeList addrRanges;

    /** Registered handlers.  Handlers are chosen using the port_type
     *  parameter on ExternalSlaves.  port_types form a global namespace
     *  across the simulation and so handlers are registered into a global
     *  structure */
    static std::map<std::string, Handler *> portHandlers;

  public:
    ExternalSlave(ExternalSlaveParams *params);

    /** SlavePort interface.  Responds only to port "port" */
    BaseSlavePort &getSlavePort(const std::string &if_name,
        PortID idx = InvalidPortID);

    /** Register a handler which can provide ports with port_type ==
     *  handler_name */
    static void registerHandler(const std::string &handler_name,
        Handler *handler);

    void init();
};


#endif //__MEM_EXTERNAL_SLAVE_HH__
