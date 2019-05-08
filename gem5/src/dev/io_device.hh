/*
 * Copyright (c) 2012 ARM Limited
 * All rights reserved.
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
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
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
 * Authors: Ali Saidi
 *          Nathan Binkert
 */

#ifndef __DEV_IO_DEVICE_HH__
#define __DEV_IO_DEVICE_HH__

#include "mem/mem_object.hh"
#include "mem/tport.hh"
#include "params/BasicPioDevice.hh"
#include "params/PioDevice.hh"

class PioDevice;
class System;

/**
 * The PioPort class is a programmed i/o port that all devices that are
 * sensitive to an address range use. The port takes all the memory
 * access types and roles them into one read() and write() call that the device
 * must respond to. The device must also provide getAddrRanges() function
 * with which it returns the address ranges it is interested in.
 */
class PioPort : public SimpleTimingPort
{
  protected:
    /** The device that this port serves. */
    PioDevice *device;

    virtual Tick recvAtomic(PacketPtr pkt);

    virtual AddrRangeList getAddrRanges() const;

  public:

    PioPort(PioDevice *dev);
};

/**
 * This device is the base class which all devices senstive to an address range
 * inherit from. There are three pure virtual functions which all devices must
 * implement getAddrRanges(), read(), and write(). The magic do choose which
 * mode we are in, etc is handled by the PioPort so the device doesn't have to
 * bother.
 */
class PioDevice : public MemObject
{
  protected:
    System *sys;

    /** The pioPort that handles the requests for us and provides us requests
     * that it sees. */
    PioPort pioPort;

    /**
     * Every PIO device is obliged to provide an implementation that
     * returns the address ranges the device responds to.
     *
     * @return a list of non-overlapping address ranges
     */
    virtual AddrRangeList getAddrRanges() const = 0;

    /** Pure virtual function that the device must implement. Called
     * when a read command is recieved by the port.
     * @param pkt Packet describing this request
     * @return number of ticks it took to complete
     */
    virtual Tick read(PacketPtr pkt) = 0;

    /** Pure virtual function that the device must implement. Called when a
     * write command is recieved by the port.
     * @param pkt Packet describing this request
     * @return number of ticks it took to complete
     */
    virtual Tick write(PacketPtr pkt) = 0;

  public:
    typedef PioDeviceParams Params;
    PioDevice(const Params *p);
    virtual ~PioDevice();

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

    virtual void init();

    virtual BaseSlavePort &getSlavePort(const std::string &if_name,
                                        PortID idx = InvalidPortID);

    friend class PioPort;

};

class BasicPioDevice : public PioDevice
{
  protected:
    /** Address that the device listens to. */
    Addr pioAddr;

    /** Size that the device's address range. */
    Addr pioSize;

    /** Delay that the device experinces on an access. */
    Tick pioDelay;

  public:
    typedef BasicPioDeviceParams Params;
    BasicPioDevice(const Params *p, Addr size);

    const Params *
    params() const
    {
        return dynamic_cast<const Params *>(_params);
    }

    /**
     * Determine the address ranges that this device responds to.
     *
     * @return a list of non-overlapping address ranges
     */
    virtual AddrRangeList getAddrRanges() const;

};

#endif // __DEV_IO_DEVICE_HH__
