/*
 * Copyright (c) 2004-2005 The Regents of The University of Michigan
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
 */

#ifndef __BASE_STATS_TEXT_HH__
#define __BASE_STATS_TEXT_HH__

#include <iosfwd>
#include <string>

#include "base/stats/output.hh"
#include "base/stats/types.hh"
#include "base/output.hh"

namespace Stats {

class Text : public Output
{
  protected:
    bool mystream;
    std::ostream *stream;

  protected:
    bool noOutput(const Info &info);

  public:
    bool descriptions;

  public:
    Text();
    Text(std::ostream &stream);
    Text(const std::string &file);
    ~Text();

    void open(std::ostream &stream);
    void open(const std::string &file);

    // Implement Visit
    virtual void visit(const ScalarInfo &info);
    virtual void visit(const VectorInfo &info);
    virtual void visit(const DistInfo &info);
    virtual void visit(const VectorDistInfo &info);
    virtual void visit(const Vector2dInfo &info);
    virtual void visit(const FormulaInfo &info);
    virtual void visit(const SparseHistInfo &info);

    // Implement Output
    virtual bool valid() const;
    virtual void begin();
    virtual void end();
};

std::string ValueToString(Result value, int precision);

Output *initText(const std::string &filename, bool desc);

} // namespace Stats

#endif // __BASE_STATS_TEXT_HH__
