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

#ifndef __SYSTEMC_EXT_CORE_SC_PORT_HH__
#define __SYSTEMC_EXT_CORE_SC_PORT_HH__

#include "sc_module.hh" // for sc_gen_unique_name
#include "sc_object.hh"

namespace sc_core
{

class sc_interface;

enum sc_port_policy
{
    SC_ONE_OR_MORE_BOUND, // Default
    SC_ZERO_OR_MORE_BOUND,
    SC_ALL_BOUND
};

class sc_port_base : public sc_object
{
  public:
    sc_port_base(const char *name, int n, sc_port_policy p) : sc_object(name)
    {}

    void warn_unimpl(const char *func) const;
};

template <class IF>
class sc_port_b : public sc_port_base
{
  public:
    void
    operator () (IF &)
    {
        warn_unimpl(__PRETTY_FUNCTION__);
    }

    void
    operator () (sc_port_b<IF> &)
    {
        warn_unimpl(__PRETTY_FUNCTION__);
    }

    virtual void
    bind(IF &)
    {
        warn_unimpl(__PRETTY_FUNCTION__);
    }

    virtual void
    bind(sc_port_b<IF> &)
    {
        warn_unimpl(__PRETTY_FUNCTION__);
    }

    int
    size() const
    {
        warn_unimpl(__PRETTY_FUNCTION__);
        return 0;
    }

    IF *
    operator -> ()
    {
        warn_unimpl(__PRETTY_FUNCTION__);
        return (IF *)nullptr;
    }

    const IF *
    operator -> () const
    {
        warn_unimpl(__PRETTY_FUNCTION__);
        return (IF *)nullptr;
    }

    IF *
    operator [] (int)
    {
        warn_unimpl(__PRETTY_FUNCTION__);
        return (IF *)nullptr;
    }

    const IF *
    operator [] (int) const
    {
        warn_unimpl(__PRETTY_FUNCTION__);
        return (IF *)nullptr;
    }

    virtual sc_interface *
    get_interface()
    {
        warn_unimpl(__PRETTY_FUNCTION__);
        return (sc_interface *)nullptr;
    }

    virtual const sc_interface *
    get_interface() const
    {
        warn_unimpl(__PRETTY_FUNCTION__);
        return (sc_interface *)nullptr;
    }

  protected:
    virtual void before_end_of_elaboration() {}
    virtual void end_of_elaboration() {}
    virtual void start_of_elaboration() {}
    virtual void end_of_simulation() {}

    explicit sc_port_b(int n, sc_port_policy p) :
            sc_port_base(sc_gen_unique_name("sc_port"), n, p)
    {}
    sc_port_b(const char *name, int n, sc_port_policy p) :
            sc_port_base(name, n, p)
    {}
    virtual ~sc_port_b() {}

  private:
    // Disabled
    sc_port_b() {}
    sc_port_b(const sc_port_b<IF> &) {}
    sc_port_b<IF> &operator = (const sc_port_b<IF> &) { return *this; }
};

template <class IF, int N=1, sc_port_policy P=SC_ONE_OR_MORE_BOUND>
class sc_port : public sc_port_b<IF>
{
  public:
    sc_port() : sc_port_b<IF>(sc_gen_unique_name("sc_port"), N, P) {}
    explicit sc_port(const char *name) : sc_port_b<IF>(name, N, P) {}
    virtual ~sc_port() {}

    virtual const char *kind() const { return "sc_port"; }

  private:
    // Disabled
    sc_port(const sc_port<IF, N, P> &) {}
    sc_port<IF, N, P> &operator = (const sc_port<IF, N, P> &) { return *this; }
};

} // namespace sc_core

#endif  //__SYSTEMC_EXT_CORE_SC_PORT_HH__
