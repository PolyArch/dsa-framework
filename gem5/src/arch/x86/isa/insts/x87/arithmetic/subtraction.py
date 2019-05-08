# Copyright (c) 2007 The Hewlett-Packard Development Company
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
# Authors: Gabe Black

microcode = '''
def macroop FSUB1_R
{
    subfp st(0), st(0), sti
};


def macroop FSUB1_M
{
    ldfp ufp1, seg, sib, disp
    subfp st(0), st(0), ufp1
};

def macroop FSUB1_P
{
    rdip t7
    ldfp ufp1, seg, riprel, disp
    subfp st(0), st(0), ufp1
};

def macroop FSUB2_R
{
    subfp sti, sti, st(0)
};

def macroop FSUB2_M
{
    ldfp ufp1, seg, sib, disp
    subfp st(0), st(0), ufp1
};

def macroop FSUB2_P
{
    rdip t7
    ldfp ufp1, seg, riprel, disp
    subfp st(0), st(0), ufp1
};

def macroop FSUBP
{
    subfp st(1), st(1), st(0), spm=1
};

def macroop FSUBP_R
{
    subfp sti, sti, st(0), spm=1
};

def macroop FSUBP_M
{
    fault "std::make_shared<UnimpInstFault>()"
};

def macroop FSUBP_P
{
   fault "std::make_shared<UnimpInstFault>()"
};

# FISUB
# FSUBR
# FSUBRP
# FISUBR
'''
