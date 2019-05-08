/*
 * Copyright (c) 2015 ARM Limited
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
 *          Andreas Sandberg
 */

#include "arch/isa_traits.hh"
#include "config/the_isa.hh"
#include "mem/packet.hh"
#include "sim/byteswap.hh"

#ifndef __MEM_PACKET_ACCESS_HH__
#define __MEM_PACKET_ACCESS_HH__

template <typename T>
inline T
Packet::getRaw() const
{
    assert(flags.isSet(STATIC_DATA|DYNAMIC_DATA));
    assert(sizeof(T) <= size);
    return *(T*)data;
}

template <typename T>
inline void
Packet::setRaw(T v)
{
    assert(flags.isSet(STATIC_DATA|DYNAMIC_DATA));
    assert(sizeof(T) <= size);
    *(T*)data = v;
}


template <typename T>
inline T
Packet::getBE() const
{
    return betoh(getRaw<T>());
}

template <typename T>
inline T
Packet::getLE() const
{
    return letoh(getRaw<T>());
}

template <typename T>
inline T
Packet::get(ByteOrder endian) const
{
    switch (endian) {
      case BigEndianByteOrder:
        return getBE<T>();

      case LittleEndianByteOrder:
        return getLE<T>();

      default:
        panic("Illegal byte order in Packet::get()\n");
    };
}

template <typename T>
inline T
Packet::get() const
{
    return TheISA::gtoh(getRaw<T>());
}

template <typename T>
inline void
Packet::setBE(T v)
{
    setRaw(htobe(v));
}

template <typename T>
inline void
Packet::setLE(T v)
{
    setRaw(htole(v));
}

template <typename T>
inline void
Packet::set(T v, ByteOrder endian)
{
    switch (endian) {
      case BigEndianByteOrder:
        return setBE<T>(v);

      case LittleEndianByteOrder:
        return setLE<T>(v);

      default:
        panic("Illegal byte order in Packet::set()\n");
    };
}

template <typename T>
inline void
Packet::set(T v)
{
    setRaw(TheISA::htog(v));
}

#endif //__MEM_PACKET_ACCESS_HH__
