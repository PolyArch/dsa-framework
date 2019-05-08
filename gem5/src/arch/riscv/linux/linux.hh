/*
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
 * Authors: Korey Sewell
 */

#ifndef __ARCH_RISCV_LINUX_LINUX_HH__
#define __ARCH_RISCV_LINUX_LINUX_HH__

#include "kern/linux/linux.hh"

class RiscvLinux : public Linux
{
  public:
    static const int TGT_SIGHUP         =  1;
    static const int TGT_SIGINT         =  2;
    static const int TGT_SIGQUIT        =  3;
    static const int TGT_SIGILL         =  4;
    static const int TGT_SIGTRAP        =  5;
    static const int TGT_SIGABRT        =  6;
    static const int TGT_SIGIOT         =  6;
    static const int TGT_SIGEMT         =  7;
    static const int TGT_SIGFPE         =  8;
    static const int TGT_SIGKILL        =  9;
    static const int TGT_SIGBUS         = 10;
    static const int TGT_SIGSEGV        = 11;
    static const int TGT_SIGSYS         = 12;
    static const int TGT_SIGPIPE        = 13;
    static const int TGT_SIGALRM        = 14;
    static const int TGT_SIGTERM        = 15;
    static const int TGT_SIGURG         = 16;
    static const int TGT_SIGSTOP        = 17;
    static const int TGT_SIGTSTP        = 18;
    static const int TGT_SIGCONT        = 19;
    static const int TGT_SIGCHLD        = 20;
    static const int TGT_SIGCLD         = 20;
    static const int TGT_SIGTTIN        = 21;
    static const int TGT_SIGTTOU        = 22;
    static const int TGT_SIGPOLL        = 23;
    static const int TGT_SIGIO          = 23;
    static const int TGT_SIGXCPU        = 24;
    static const int TGT_SIGXFSZ        = 25;
    static const int TGT_SIGVTALRM      = 26;
    static const int TGT_SIGPROF        = 27;
    static const int TGT_SIGWINCH       = 28;
    static const int TGT_SIGLOST        = 29;
    static const int TGT_SIGPWR         = 29;
    static const int TGT_SIGUSR1        = 30;
    static const int TGT_SIGUSR2        = 31;

    /// This table maps the target open() flags to the corresponding
    /// host open() flags.
    static SyscallFlagTransTable openFlagTable[];

    /// Number of entries in openFlagTable[].
    static const int NUM_OPEN_FLAGS;

    //@{
    /// open(2) flag values.
    static const int TGT_O_RDONLY       = 0x000000; //!< O_RDONLY
    static const int TGT_O_WRONLY       = 0x000001; //!< O_WRONLY
    static const int TGT_O_RDWR         = 0x000002; //!< O_RDWR
    static const int TGT_O_CREAT        = 0x000040; //!< O_CREAT
    static const int TGT_O_EXCL         = 0x000080; //!< O_EXCL
    static const int TGT_O_NOCTTY       = 0x000100; //!< O_NOCTTY
    static const int TGT_O_TRUNC        = 0x000200; //!< O_TRUNC
    static const int TGT_O_APPEND       = 0x000400; //!< O_APPEND
    static const int TGT_O_NONBLOCK     = 0x000800; //!< O_NONBLOCK
    static const int TGT_O_SYNC         = 0x001000; //!< O_SYNC
    static const int TGT_FSYNC          = 0x001000; //!< FSYNC
    static const int TGT_FASYNC         = 0x008000; //!< FASYNC
    // The following are not present in riscv64-unknown-elf <fcntl.h>
    static const int TGT_O_DSYNC        = 0x010000; //!< O_DSYNC
    static const int TGT_O_CLOEXEC      = 0x040000; //!< O_CLOEXEC
    static const int TGT_O_NOINHERIT    = 0x040000; //!< O_NOINHERIT
    static const int TGT_O_DIRECT       = 0x080000; //!< O_DIRECT
    static const int TGT_O_NOFOLLOW     = 0x100000; //!< O_NOFOLLOW
    static const int TGT_O_DIRECTORY    = 0x200000; //!< O_DIRECTORY
    // The following are not defined by riscv64-unknown-elf
    static const int TGT_O_LARGEFILE    = 0x020000; //!< O_LARGEFILE
    static const int TGT_O_NOATIME      = 0x800000; //!< O_NOATIME
    static const int TGT_O_PATH         = 0x400000; //!< O_PATH
    //@}

    // Only defined in riscv-unknown-elf for proxy kernel and not linux kernel
    static const unsigned TGT_MAP_SHARED        = 0x0001;
    static const unsigned TGT_MAP_PRIVATE       = 0x0002;
    static const unsigned TGT_MAP_FIXED         = 0x0010;
    static const unsigned TGT_MAP_ANONYMOUS     = 0x0020;
    static const unsigned TGT_MAP_POPULATE      = 0x1000;
    static const unsigned TGT_MREMAP_FIXED      = 0x0020;

    static const unsigned NUM_MMAP_FLAGS;

    typedef int64_t time_t;
    typedef uint64_t dev_t;
    typedef uint64_t ino_t;
    typedef uint32_t mode_t;
    typedef uint32_t nlink_t;
    typedef uint32_t uid_t;
    typedef uint32_t gid_t;
    typedef int64_t off_t;
    typedef uint64_t blksize_t;
    typedef uint64_t blkcnt_t;

    struct timespec {
        time_t tv_sec;
        int64_t tv_nsec;
    };

    typedef struct {
        dev_t st_dev;
        ino_t st_ino;
        mode_t st_mode;
        nlink_t st_nlink;
        uid_t st_uid;
        gid_t st_gid;
        dev_t st_rdev;
        dev_t __pad1;
        off_t st_size;
        blksize_t st_blksize;
        blkcnt_t st_blocks;
        uint64_t st_atimeX;
        uint64_t st_atime_nsec;
        uint64_t st_mtimeX;
        uint64_t st_mtime_nsec;
        uint64_t st_ctimeX;
        uint64_t st_ctime_nsec;
        int32_t ___glibc_reserved[2];
    } tgt_stat64;

    typedef struct {
        int32_t val[2];
    } tgt_fsid_t;

    typedef struct {
        uint64_t f_type;
        uint64_t f_bsize;
        uint64_t f_blocks;
        uint64_t f_bfree;
        uint64_t f_bavail;
        uint64_t f_files;
        uint64_t f_ffree;
        tgt_fsid_t f_fsid;
        uint64_t f_namelen;
        uint64_t f_frsize;
        uint64_t f_flags;
        uint64_t f_spare[4];
    } tgt_statfs;

    typedef struct {
        int64_t uptime;
        uint64_t loads[3];
        uint64_t totalram;
        uint64_t freeram;
        uint64_t sharedram;
        uint64_t bufferram;
        uint64_t totalswap;
        uint64_t freeswap;
        uint16_t procs;
        uint16_t pad;
        uint64_t totalhigh;
        uint64_t freehigh;
        uint32_t mem_unit;
    } tgt_sysinfo;
};

#endif
