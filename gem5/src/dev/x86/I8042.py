# Copyright (c) 2008 The Regents of The University of Michigan
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
#
# Authors: Gabe Black

from m5.params import *
from m5.proxy import *
from Device import BasicPioDevice
from X86IntPin import X86IntSourcePin
from PS2 import *

class I8042(BasicPioDevice):
    type = 'I8042'
    cxx_class = 'X86ISA::I8042'
    cxx_header = "dev/x86/i8042.hh"
    # This isn't actually used for anything here.
    pio_addr = 0x0
    data_port = Param.Addr('Data port address')
    command_port = Param.Addr('Command/status port address')
    mouse_int_pin = Param.X86IntSourcePin(X86IntSourcePin(),
            'Pin to signal the mouse has data')
    keyboard_int_pin = Param.X86IntSourcePin(X86IntSourcePin(),
            'Pin to signal the keyboard has data')

    keyboard = Param.PS2Device(PS2Keyboard(vnc=NULL), "PS/2 keyboard device")
    mouse = Param.PS2Device(PS2Mouse(), "PS/2 mouse device")
