# Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
# Copyright (c) 2009 The Hewlett-Packard Development Company
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from m5.util import makeDir

from slicc.generate import html
from slicc.symbols.StateMachine import StateMachine
from slicc.symbols.Type import Type
from slicc.util import Location

class SymbolTable(object):
    def __init__(self, slicc):
        self.slicc = slicc

        self.sym_vec = []
        self.sym_map_vec = [ {} ]
        self.machine_components = {}

        pairs = {}
        pairs["primitive"] = "yes"
        pairs["external"] = "yes"
        location = Location("init", 0, no_warning=not slicc.verbose)
        void = Type(self, "void", location, pairs)
        self.newSymbol(void)

    def __repr__(self):
        return "[SymbolTable]" # FIXME

    def codeFormatter(self, *args, **kwargs):
        return self.slicc.codeFormatter(*args, **kwargs)

    def newSymbol(self, sym):
        self.registerSym(str(sym), sym)
        self.sym_vec.append(sym)

    def registerSym(self, id, sym):
        # Check for redeclaration (in the current frame only)
        if id in self.sym_map_vec[-1]:
            sym.error("Symbol '%s' redeclared in same scope.", id)

        for sym_map in self.sym_map_vec:
            if id in sym_map:
                if type(sym_map[id]) != type(sym):
                    sym.error("Conflicting declaration of Symbol '%s'", id)

        # FIXME - warn on masking of a declaration in a previous frame
        self.sym_map_vec[-1][id] = sym

    def find(self, ident, types=None):
        for sym_map in reversed(self.sym_map_vec):
            try:
                symbol = sym_map[ident]
            except KeyError:
                continue

            if types is not None:
                if not isinstance(symbol, types):
                    continue # there could be a name clash with other symbol
                             # so rather than producing an error, keep trying

            return symbol

        return None

    def newMachComponentSym(self, symbol):
        # used to cheat-- that is, access components in other machines
        machine = self.find("current_machine", StateMachine)
        if machine:
            self.machine_components[str(machine)][str(symbol)] = symbol

    def newCurrentMachine(self, sym):
        self.registerGlobalSym(str(sym), sym)
        self.registerSym("current_machine", sym)
        self.sym_vec.append(sym)

        self.machine_components[str(sym)] = {}

    @property
    def state_machine(self):
        return self.find("current_machine", StateMachine)

    def pushFrame(self):
        self.sym_map_vec.append({})

    def popFrame(self):
        assert len(self.sym_map_vec) > 0
        self.sym_map_vec.pop()

    def registerGlobalSym(self, ident, symbol):
        # Check for redeclaration (global frame only)
        if ident in self.sym_map_vec[0]:
            symbol.error("Symbol '%s' redeclared in global scope." % ident)

        self.sym_map_vec[0][ident] = symbol

    def getAllType(self, type):
        for symbol in self.sym_vec:
            if isinstance(symbol, type):
                yield symbol

    def writeCodeFiles(self, path, includes):
        makeDir(path)

        code = self.codeFormatter()
        code('/** Auto generated C++ code started by $__file__:$__line__ */')

        for include_path in includes:
            code('#include "${{include_path}}"')

        for symbol in self.sym_vec:
            if isinstance(symbol, Type) and not symbol.isPrimitive:
                code('#include "mem/protocol/${{symbol.c_ident}}.hh"')

        code.write(path, "Types.hh")

        for symbol in self.sym_vec:
            symbol.writeCodeFiles(path, includes)

    def writeHTMLFiles(self, path):
        makeDir(path)

        machines = list(self.getAllType(StateMachine))
        if len(machines) > 1:
            name = "%s_table.html" % machines[0].ident
        else:
            name = "empty.html"

        code = self.codeFormatter()
        code('''
<html>
<head>
<title>$path</title>
</head>
<frameset rows="*,30">
    <frame name="Table" src="$name">
    <frame name="Status" src="empty.html">
</frameset>
</html>
''')
        code.write(path, "index.html")

        code = self.codeFormatter()
        code("<HTML></HTML>")
        code.write(path, "empty.html")

        for symbol in self.sym_vec:
            symbol.writeHTMLFiles(path)

__all__ = [ "SymbolTable" ]
