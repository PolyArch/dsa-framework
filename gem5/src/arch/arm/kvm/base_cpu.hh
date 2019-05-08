/*
 * Copyright (c) 2012, 2015 ARM Limited
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
 * Authors: Andreas Sandberg
 */

#ifndef __ARCH_ARM_KVM_BASE_CPU_HH__
#define __ARCH_ARM_KVM_BASE_CPU_HH__

#include <vector>

#include "cpu/kvm/base.hh"

struct BaseArmKvmCPUParams;

class BaseArmKvmCPU : public BaseKvmCPU
{
  public:
    BaseArmKvmCPU(BaseArmKvmCPUParams *params);
    virtual ~BaseArmKvmCPU();

    void startup() override;

  protected:
    Tick kvmRun(Tick ticks) override;


    /** Cached state of the IRQ line */
    bool irqAsserted;
    /** Cached state of the FIQ line */
    bool fiqAsserted;

  protected:
    typedef std::vector<uint64_t> RegIndexVector;

    /**
     * Get a list of registers supported by getOneReg() and setOneReg().
     *
     * This method returns a list of all registers supported by
     * kvm. The actual list is only requested the first time this
     * method is called. Subsequent calls return a cached copy of the
     * register list.
     *
     * @return Vector of register indexes.
     */
    const RegIndexVector &getRegList() const;

    /**
     * Tell the kernel to initialize this CPU
     *
     * The kernel needs to know what type of the CPU that we want to
     * emulate. The specified CPU type has to be compatible with the
     * host CPU. In practice, we usually call
     * KvmVM::kvmArmPreferredTarget() to discover the host CPU.
     *
     * @param target CPU type to emulate
     */
    void kvmArmVCpuInit(const struct kvm_vcpu_init &init);

  private:
    std::unique_ptr<struct kvm_reg_list> tryGetRegList(uint64_t nelem) const;

    /**
     * Get a list of registers supported by getOneReg() and setOneReg().
     *
     * @return False if the number of elements allocated in the list
     * is too small to hold the complete register list (the required
     * size is written to regs.n in this case). True on success.
     */
    bool getRegList(struct kvm_reg_list &regs) const;

    /**
     * Cached copy of the list of registers supported by KVM
     */
    mutable RegIndexVector _regIndexList;
};

#endif // __ARCH_ARM_KVM_BASE_CPU_HH__
