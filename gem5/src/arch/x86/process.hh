/*
 * Copyright (c) 2007 The Hewlett-Packard Development Company
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
 * Authors: Gabe Black
 */

#ifndef __ARCH_X86_PROCESS_HH__
#define __ARCH_X86_PROCESS_HH__

#include <string>
#include <vector>

#include "arch/x86/pagetable.hh"
#include "mem/multi_level_page_table.hh"
#include "sim/aux_vector.hh"
#include "sim/process.hh"

class SyscallDesc;

namespace X86ISA
{
    enum X86AuxiliaryVectorTypes {
        M5_AT_SYSINFO = 32,
        M5_AT_SYSINFO_EHDR = 33
    };

    class X86Process : public Process
    {
      protected:
        /**
         * Declaration of architectural page table for x86.
         *
         * These page tables are stored in system memory and respect x86
         * specification.
         */

        Addr _gdtStart;
        Addr _gdtSize;

        SyscallDesc *syscallDescs;
        const int numSyscallDescs;

        X86Process(ProcessParams * params, ObjectFile *objFile,
                   SyscallDesc *_syscallDescs, int _numSyscallDescs);

        template<class IntType>
        void argsInit(int pageSize,
                      std::vector<AuxVector<IntType> > extraAuxvs);

      public:
        Addr gdtStart()
        { return _gdtStart; }

        Addr gdtSize()
        { return _gdtSize; }

        SyscallDesc* getDesc(int callnum) override;

        void setSyscallReturn(ThreadContext *tc,
                              SyscallReturn return_value) override;
        void clone(ThreadContext *old_tc, ThreadContext *new_tc,
                   Process *process, TheISA::IntReg flags) override;

        X86Process &
        operator=(const X86Process &in)
        {
            if (this == &in)
                return *this;

            _gdtStart = in._gdtStart;
            _gdtSize = in._gdtSize;
            syscallDescs = in.syscallDescs;

            return *this;
        }
    };

    class X86_64Process : public X86Process
    {
      protected:
        X86_64Process(ProcessParams *params, ObjectFile *objFile,
                      SyscallDesc *_syscallDescs, int _numSyscallDescs);

        class VSyscallPage
        {
          public:
            Addr base;
            Addr size;
            Addr vtimeOffset;
            Addr vgettimeofdayOffset;

            VSyscallPage &
            operator=(const VSyscallPage &in)
            {
                if (this == &in)
                    return *this;

                base = in.base;
                size = in.size;
                vtimeOffset = in.vtimeOffset;
                vgettimeofdayOffset = in.vgettimeofdayOffset;

                return *this;
            }
        };
        VSyscallPage vsyscallPage;

      public:
        void argsInit(int pageSize);
        void initState() override;

        X86ISA::IntReg getSyscallArg(ThreadContext *tc, int &i) override;
        /// Explicitly import the otherwise hidden getSyscallArg
        using Process::getSyscallArg;
        void setSyscallArg(ThreadContext *tc, int i,
                           X86ISA::IntReg val) override;
        void clone(ThreadContext *old_tc, ThreadContext *new_tc,
                   Process *process, TheISA::IntReg flags) override;
    };

    class I386Process : public X86Process
    {
      protected:
        I386Process(ProcessParams *params, ObjectFile *objFile,
                    SyscallDesc *_syscallDescs, int _numSyscallDescs);

        class VSyscallPage
        {
          public:
            Addr base;
            Addr size;
            Addr vsyscallOffset;
            Addr vsysexitOffset;

            VSyscallPage &
            operator=(const VSyscallPage &in)
            {
                if (this == &in)
                    return *this;

                base = in.base;
                size = in.size;
                vsyscallOffset = in.vsyscallOffset;
                vsysexitOffset = in.vsysexitOffset;

                return *this;
            }
        };
        VSyscallPage vsyscallPage;

      public:
        void argsInit(int pageSize);
        void initState() override;

        void syscall(int64_t callnum, ThreadContext *tc,
                     Fault *fault) override;
        X86ISA::IntReg getSyscallArg(ThreadContext *tc,
                                     int &i) override;
        X86ISA::IntReg getSyscallArg(ThreadContext *tc, int &i,
                                     int width) override;
        void setSyscallArg(ThreadContext *tc, int i,
                           X86ISA::IntReg val) override;
        void clone(ThreadContext *old_tc, ThreadContext *new_tc,
                   Process *process, TheISA::IntReg flags) override;
    };

}

#endif // __ARCH_X86_PROCESS_HH__
