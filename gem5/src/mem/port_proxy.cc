/*
 * Copyright (c) 2012, 2018 ARM Limited
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

#include "mem/port_proxy.hh"

#include "base/chunk_generator.hh"

void
PortProxy::readBlobPhys(Addr addr, Request::Flags flags,
                        uint8_t *p, int size) const
{
    for (ChunkGenerator gen(addr, size, _cacheLineSize); !gen.done();
         gen.next()) {

        auto req = std::make_shared<Request>(
            gen.addr(), gen.size(), flags, Request::funcMasterId);

        Packet pkt(req, MemCmd::ReadReq);
        pkt.dataStatic(p);
        _port.sendFunctional(&pkt);
        p += gen.size();
    }
}

void
PortProxy::writeBlobPhys(Addr addr, Request::Flags flags,
                         const uint8_t *p, int size) const
{
    for (ChunkGenerator gen(addr, size, _cacheLineSize); !gen.done();
         gen.next()) {

        auto req = std::make_shared<Request>(
            gen.addr(), gen.size(), flags, Request::funcMasterId);

        Packet pkt(req, MemCmd::WriteReq);
        pkt.dataStaticConst(p);
        _port.sendFunctional(&pkt);
        p += gen.size();
    }
}

void
PortProxy::memsetBlobPhys(Addr addr, Request::Flags flags,
                          uint8_t v, int size) const
{
    // quick and dirty...
    uint8_t *buf = new uint8_t[size];

    std::memset(buf, v, size);
    PortProxy::writeBlobPhys(addr, flags, buf, size);

    delete [] buf;
}


void
SecurePortProxy::readBlob(Addr addr, uint8_t *p, int size) const
{
    readBlobPhys(addr, Request::SECURE, p, size);
}

void
SecurePortProxy::writeBlob(Addr addr, const uint8_t *p, int size) const
{
    writeBlobPhys(addr, Request::SECURE, p, size);
}

void
SecurePortProxy::memsetBlob(Addr addr, uint8_t v, int size) const
{
    memsetBlobPhys(addr, Request::SECURE, v, size);
}
