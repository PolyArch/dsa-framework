# Copyright (c) 2017 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
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
#
# Authors: Andreas Sandberg

from abc import *

class PyBindExport(object):
    __metaclass__ = ABCMeta

    @abstractmethod
    def export(self, code, cname):
        pass

class PyBindProperty(PyBindExport):
    def __init__(self, name, cxx_name=None, writable=True):
        self.name = name
        self.cxx_name = cxx_name if cxx_name else name
        self.writable = writable

    def export(self, code, cname):
        export = "def_readwrite" if self.writable else "def_readonly"
        code('.${export}("${{self.name}}", &${cname}::${{self.cxx_name}})')

class PyBindMethod(PyBindExport):
    def __init__(self, name, cxx_name=None, args=None):
        self.name = name
        self.cxx_name = cxx_name if cxx_name else name
        self.args = args

    def _conv_arg(self, value):
        if isinstance(value, bool):
            return "true" if value else "false"
        elif isinstance(value, (float, int)):
            return repr(value)
        else:
            raise TypeError("Unsupported PyBind default value type")

    def export(self, code, cname):
        if self.args:
            def get_arg_decl(arg):
                if isinstance(arg, tuple):
                    name, default = arg
                    return 'py::arg("%s") = %s' % (
                        name, self._conv_arg(default))
                else:
                    return 'py::arg("%s")' % arg

            code('.def("${{self.name}}", &${cname}::${{self.name}}, ')
            code('    ' + \
                 ', '.join([ get_arg_decl(a) for a in self.args ]) + ')')
        else:
            code('.def("${{self.name}}", &${cname}::${{self.cxx_name}})')
