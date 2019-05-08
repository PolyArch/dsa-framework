/*
 * Copyright (c) 2017 The University of Virginia
 * Copyright 2015 LabWare
 * Copyright 2014 Google, Inc.
 * Copyright (c) 2007 The Regents of The University of Michigan
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
 *          Boris Shingarov
 *          Alec Roelke
 */

#ifndef __ARCH_RISCV_REMOTE_GDB_HH__
#define __ARCH_RISCV_REMOTE_GDB_HH__

#include <string>

#include "arch/riscv/registers.hh"
#include "base/remote_gdb.hh"

class System;
class ThreadContext;

namespace RiscvISA
{

class RemoteGDB : public BaseRemoteGDB
{
  protected:
    static const int ExplicitCSRs = 4;

    bool acc(Addr addr, size_t len);

    class RiscvGdbRegCache : public BaseGdbRegCache
    {
      using BaseGdbRegCache::BaseGdbRegCache;
      private:
        struct {
            IntReg gpr[NumIntArchRegs];
            IntReg pc;
            FloatRegBits fpr[NumFloatRegs];

            MiscReg csr_base;
            uint32_t fflags;
            uint32_t frm;
            uint32_t fcsr;
            MiscReg csr[NumMiscRegs - ExplicitCSRs];
        } __attribute__((__packed__)) r;
      public:
        char *data() const { return (char *)&r; }
        size_t size() const { return sizeof(r); }
        void getRegs(ThreadContext*);
        void setRegs(ThreadContext*) const;

        const std::string
        name() const
        {
            return gdb->name() + ".RiscvGdbRegCache";
        }
    };

    RiscvGdbRegCache regCache;

  public:
    RemoteGDB(System *_system, ThreadContext *tc, int _port);
    BaseGdbRegCache *gdbRegs();
};

} // namespace RiscvISA

#endif /* __ARCH_RISCV_REMOTE_GDB_H__ */
