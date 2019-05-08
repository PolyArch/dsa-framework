/*
 * Copyright (c) 1999 Mark D. Hill and David A. Wood
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
 */

#ifndef __MEM_RUBY_COMMON_ADDRESS_HH__
#define __MEM_RUBY_COMMON_ADDRESS_HH__

#include <cassert>
#include <iomanip>
#include <iostream>

#include "base/types.hh"

const uint32_t ADDRESS_WIDTH = 64; // address width in bytes

// selects bits inclusive
Addr bitSelect(Addr addr, unsigned int small, unsigned int big);
Addr bitRemove(Addr addr, unsigned int small, unsigned int big);
Addr maskLowOrderBits(Addr addr, unsigned int number);
Addr maskHighOrderBits(Addr addr, unsigned int number);
Addr shiftLowOrderBits(Addr addr, unsigned int number);
Addr getOffset(Addr addr);
Addr makeLineAddress(Addr addr);
Addr makeNextStrideAddress(Addr addr, int stride);
std::string printAddress(Addr addr);

#endif // __MEM_RUBY_COMMON_ADDRESS_HH__
