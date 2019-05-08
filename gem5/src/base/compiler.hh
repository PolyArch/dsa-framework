/*
 * Copyright (c) 2012,2017-2018 ARM Limited
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
 */

#ifndef __BASE_COMPILER_HH__
#define __BASE_COMPILER_HH__

#include <memory>

// http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html

#if defined(__GNUC__) // clang or gcc
#  define M5_ATTR_NORETURN  __attribute__((noreturn))
#  define M5_DUMMY_RETURN
#  define M5_VAR_USED __attribute__((unused))
#  define M5_ATTR_PACKED __attribute__ ((__packed__))
#  define M5_NO_INLINE __attribute__ ((__noinline__))
#  define M5_DEPRECATED __attribute__((deprecated))
#  define M5_DEPRECATED_MSG(MSG) __attribute__((deprecated(MSG)))
#  define M5_UNREACHABLE __builtin_unreachable()
#  define M5_PUBLIC __attribute__ ((visibility ("default")))
#  define M5_LOCAL __attribute__ ((visibility ("hidden")))
#endif

#if defined(__clang__)
#  define M5_CLASS_VAR_USED M5_VAR_USED
#else
#  define M5_CLASS_VAR_USED
#endif

// This can be removed once all compilers support C++17
#if defined __has_cpp_attribute
    // Note: We must separate this if statement because GCC < 5.0 doesn't
    //       support the function-like syntax in #if statements.
    #if __has_cpp_attribute(fallthrough)
        #define M5_FALLTHROUGH [[fallthrough]]
    #else
        #define M5_FALLTHROUGH
    #endif

    #if __has_cpp_attribute(nodiscard)
        #define M5_NODISCARD [[nodiscard]]
    #else
        #define M5_NODISCARD
    #endif
#else
    // Unsupported (and no warning) on GCC < 7.
    #define M5_FALLTHROUGH

    #define M5_NODISCARD
#endif

// std::make_unique redefined for C++11 compilers
namespace m5
{

#if __cplusplus >= 201402L // C++14

using std::make_unique;

#else // C++11

/** Defining custom version of make_unique: m5::make_unique<>() */
template<typename T, typename... Args>
std::unique_ptr<T>
make_unique( Args&&... constructor_args )
{
    return std::unique_ptr<T>(
               new T( std::forward<Args>(constructor_args)... )
           );
}

#endif // __cplusplus >= 201402L

} //namespace m5

#endif // __BASE_COMPILER_HH__
