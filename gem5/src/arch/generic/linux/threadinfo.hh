/*
 * Copyright (c) 2004 The Regents of The University of Michigan
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
 *          Dam Sunwoo
 */

#ifndef __ARCH_GENERIC_LINUX_THREADINFO_HH__
#define __ARCH_GENERIC_LINUX_THREADINFO_HH__

#include "cpu/thread_context.hh"
#include "sim/system.hh"
#include "sim/vptr.hh"

namespace Linux {

class ThreadInfo
{
  private:
    ThreadContext *tc;
    System *sys;
    Addr pcbb;

    template <typename T>
    bool
    get_data(const char *symbol, T &data)
    {
        Addr addr = 0;
        if (!sys->kernelSymtab->findAddress(symbol, addr)) {
            warn_once("Unable to find kernel symbol %s\n", symbol);
            warn_once("Kernel not compiled with task_struct info; can't get "
                      "currently executing task/process/thread name/ids!\n");
            return false;
        }

        CopyOut(tc, &data, addr, sizeof(T));

        data = TheISA::gtoh(data);

        return true;
    }

  public:
    ThreadInfo(ThreadContext *_tc, Addr _pcbb = 0)
        : tc(_tc), sys(tc->getSystemPtr()), pcbb(_pcbb)
    {

    }
    ~ThreadInfo()
    {}

    inline Addr
    curThreadInfo()
    {
        if (!TheISA::CurThreadInfoImplemented)
            panic("curThreadInfo() not implemented for this ISA");

        Addr addr = pcbb;
        Addr sp;

        if (!addr)
            addr = tc->readMiscRegNoEffect(TheISA::CurThreadInfoReg);

        PortProxy &p = tc->getPhysProxy();
        p.readBlob(addr, (uint8_t *)&sp, sizeof(Addr));

        return sp & ~ULL(0x3fff);
    }

    inline Addr
    curTaskInfo(Addr thread_info = 0)
    {
        // Note that in Linux 4.10 the thread_info struct will no longer have a
        // pointer to the task_struct for arm64. See:
        // https://patchwork.kernel.org/patch/9333699/
        int32_t offset;
        if (!get_data("thread_info_task", offset))
            return 0;

        if (!thread_info)
            thread_info = curThreadInfo();

        Addr addr;
        CopyOut(tc, &addr, thread_info + offset, sizeof(addr));

        return addr;
    }

    int32_t
    curTaskPIDFromTaskStruct(Addr task_struct) {
        int32_t offset;
        if (!get_data("task_struct_pid", offset))
            return -1;

        int32_t pid;
        CopyOut(tc, &pid, task_struct + offset, sizeof(pid));

        return pid;
    }

    int32_t
    curTaskPID(Addr thread_info = 0)
    {
        return curTaskPIDFromTaskStruct(curTaskInfo(thread_info));
    }

    int32_t
    curTaskTGIDFromTaskStruct(Addr task_struct)
    {
        int32_t offset;
        if (!get_data("task_struct_tgid", offset))
            return -1;

        int32_t tgid;
        CopyOut(tc, &tgid, task_struct + offset, sizeof(tgid));

        return tgid;
    }

    int32_t
    curTaskTGID(Addr thread_info = 0)
    {
        return curTaskTGIDFromTaskStruct(curTaskInfo(thread_info));
    }

    int64_t
    curTaskStartFromTaskStruct(Addr task_struct)
    {
        int32_t offset;
        if (!get_data("task_struct_start_time", offset))
            return -1;

        int64_t data;
        // start_time is actually of type timespec, but if we just
        // grab the first long, we'll get the seconds out of it
        CopyOut(tc, &data, task_struct + offset, sizeof(data));

        return data;
    }

    int64_t
    curTaskStart(Addr thread_info = 0)
    {
        return curTaskStartFromTaskStruct(curTaskInfo(thread_info));
    }

    std::string
    curTaskNameFromTaskStruct(Addr task_struct)
    {
        int32_t offset;
        int32_t size;

        if (!get_data("task_struct_comm", offset))
            return "FailureIn_curTaskName";

        if (!get_data("task_struct_comm_size", size))
            return "FailureIn_curTaskName";

        char buffer[size + 1];
        CopyStringOut(tc, buffer, task_struct + offset, size);

        return buffer;
    }

    std::string
    curTaskName(Addr thread_info = 0)
    {
        return curTaskNameFromTaskStruct(curTaskInfo(thread_info));
    }

    int32_t
    curTaskMmFromTaskStruct(Addr task_struct)
    {
        int32_t offset;
        if (!get_data("task_struct_mm", offset))
            return -1;

        int32_t mm_ptr;
        CopyOut(tc, &mm_ptr, task_struct + offset, sizeof(mm_ptr));

        return mm_ptr;
    }

    int32_t
    curTaskMm(Addr thread_info = 0)
    {
        return curTaskMmFromTaskStruct(curTaskInfo(thread_info));
    }
};

} // namespace Linux

#endif // __ARCH_GENERIC_LINUX_THREADINFO_HH__
