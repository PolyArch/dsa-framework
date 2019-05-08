/*
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
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

#include <iostream>

#include "base/loader/symtab.hh"
#include "base/str.hh"

using namespace std;

void usage(const char *progname);

void
usage(const char *progname)
{
    cout << "Usage: " << progname << " <symbol file> <symbol>" << endl;

    exit(1);
}

int
main(int argc, char *argv[])
{
    SymbolTable symtab;

    if (argc != 3)
        usage(argv[0]);

    if (!symtab.load(argv[1])) {
        cout << "could not load symbol file: " << argv[1] << endl;
        exit(1);
    }

    string symbol = argv[2];
    Addr address;

    if (!to_number(symbol, address)) {
        if (!symtab.findAddress(symbol, address)) {
            cout << "could not find symbol: " << symbol << endl;
            exit(1);
        }

        cout << symbol << " -> " << "0x" << hex << address << endl;
    } else {
        if (!symtab.findSymbol(address, symbol)) {
            cout << "could not find address: " << address << endl;
            exit(1);
        }

        cout << "0x" << hex << address << " -> " << symbol<< endl;
    }

    return 0;
}
