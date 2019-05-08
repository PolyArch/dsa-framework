/*
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
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
 */

#ifndef __ARCH_SPARC_SOLARIS_SOLARIS_HH__
#define __ARCH_SPARC_SOLARIS_SOLARIS_HH__

#include "kern/solaris/solaris.hh"

class SparcSolaris : public Solaris
{
  public:

    static SyscallFlagTransTable openFlagTable[];

    static const int TGT_O_RDONLY       = 0x00000000;   //!< O_RDONLY
    static const int TGT_O_WRONLY       = 0x00000001;   //!< O_WRONLY
    static const int TGT_O_RDWR         = 0x00000002;   //!< O_RDWR
    static const int TGT_O_NDELAY       = 0x00000004;   //!< O_NONBLOCK
    static const int TGT_O_APPEND       = 0x00000008;   //!< O_APPEND
    static const int TGT_O_SYNC         = 0x00000010;   //!< O_SYNC
    static const int TGT_O_DSYNC        = 0x00000040;   //!< O_SYNC
    static const int TGT_O_RSYNC        = 0x00008000;   //!< O_SYNC
    static const int TGT_O_NONBLOCK     = 0x00000080;   //!< O_NONBLOCK
    static const int TGT_O_PRIV         = 0x00001000;   //??
    static const int TGT_O_LARGEFILE    = 0x00002000;   //??
    static const int TGT_O_CREAT        = 0x00000100;   //!< O_CREAT
    static const int TGT_O_TRUNC        = 0x00000200;   //!< O_TRUNC
    static const int TGT_O_EXCL         = 0x00000400;   //!< O_EXCL
    static const int TGT_O_NOCTTY       = 0x00000800;   //!< O_NOCTTY
    static const int TGT_O_XATTR        = 0x00004000;   //??

    static const int NUM_OPEN_FLAGS;

    /// For mmap().
    static SyscallFlagTransTable mmapFlagTable[];

    static const unsigned TGT_MAP_SHARED        = 0x00001;
    static const unsigned TGT_MAP_PRIVATE       = 0x00002;
    static const unsigned TGT_MAP_32BIT         = 0x00040;
    static const unsigned TGT_MAP_ANON          = 0x00020;
    static const unsigned TGT_MAP_DENYWRITE     = 0x00800;
    static const unsigned TGT_MAP_EXECUTABLE    = 0x01000;
    static const unsigned TGT_MAP_FILE          = 0x00000;
    static const unsigned TGT_MAP_GROWSDOWN     = 0x00100;
    static const unsigned TGT_MAP_HUGETLB       = 0x40000;
    static const unsigned TGT_MAP_LOCKED        = 0x02000;
    static const unsigned TGT_MAP_NONBLOCK      = 0x10000;
    static const unsigned TGT_MAP_NORESERVE     = 0x04000;
    static const unsigned TGT_MAP_POPULATE      = 0x08000;
    static const unsigned TGT_MAP_STACK         = 0x20000;
    static const unsigned TGT_MAP_ANONYMOUS     = 0x00020;
    static const unsigned TGT_MAP_FIXED         = 0x00010;

    static const unsigned NUM_MMAP_FLAGS;
};

#endif
