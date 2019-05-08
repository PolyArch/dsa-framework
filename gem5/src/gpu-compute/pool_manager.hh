/*
 * Copyright (c) 2015-2017 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * For use for simulation and test purposes only
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: John Kalamatianos
 */

#ifndef __POOL_MANAGER_HH__
#define __POOL_MANAGER_HH__

#include <cassert>
#include <cstdint>
#include <string>

// Pool Manager Logic
class PoolManager
{
  public:
    PoolManager(uint32_t minAlloc, uint32_t poolSize);
    uint32_t minAllocation() { return _minAllocation; }
    virtual std::string printRegion() = 0;
    virtual uint32_t regionSize(std::pair<uint32_t,uint32_t> &region) = 0;
    virtual bool canAllocate(uint32_t numRegions, uint32_t size) = 0;

    virtual uint32_t allocateRegion(const uint32_t size,
                                    uint32_t *reserved) = 0;

    virtual void freeRegion(uint32_t firstIdx, uint32_t lastIdx) = 0;
    uint32_t poolSize() { return _poolSize; }

  private:
    // minimum size that can be reserved per allocation
    uint32_t _minAllocation;
    // pool size in number of elements
    uint32_t _poolSize;
};

#endif // __POOL_MANAGER_HH__
