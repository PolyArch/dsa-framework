/*
 * Copyright 2018 Google, Inc.
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

#ifndef __SYSTEMC_EXT_CORE_SC_TIME_HH__
#define __SYSTEMC_EXT_CORE_SC_TIME_HH__

#include <stdint.h>

#include <iostream>

#include "../dt/int/sc_nbdefs.hh"

namespace sc_core
{

enum sc_time_unit {
    SC_FS = 0,
    SC_PS,
    SC_NS,
    SC_US,
    SC_MS,
    SC_SEC
};

class sc_time
{
  public:
    sc_time();
    sc_time(double, sc_time_unit);
    sc_time(const sc_time &);

    sc_time &operator = (const sc_time &);

    sc_dt::uint64 value() const;
    double to_double() const;
    double to_seconds() const;
    const std::string to_string() const;

    bool operator == (const sc_time &) const;
    bool operator != (const sc_time &) const;
    bool operator < (const sc_time &) const;
    bool operator <= (const sc_time &) const;
    bool operator > (const sc_time &) const;
    bool operator >= (const sc_time &) const;

    sc_time &operator += (const sc_time &);
    sc_time &operator -= (const sc_time &);
    sc_time &operator *= (double);
    sc_time &operator /= (double);

    void print(std::ostream & =std::cout) const;
};

const sc_time operator + (const sc_time &, const sc_time &);
const sc_time operator - (const sc_time &, const sc_time &);

const sc_time operator * (const sc_time &, double);
const sc_time operator * (double, const sc_time &);
const sc_time operator / (const sc_time &, double);
double operator / (const sc_time &, const sc_time &);

std::ostream &operator << (std::ostream &, const sc_time &);

extern const sc_time SC_ZERO_TIME;

void sc_set_time_resolution(double, sc_time_unit);
sc_time sc_get_time_resolution();
const sc_time &sc_max_time();

} // namespace sc_core

#endif  //__SYSTEMC_EXT_CORE_SC_TIME_HH__
