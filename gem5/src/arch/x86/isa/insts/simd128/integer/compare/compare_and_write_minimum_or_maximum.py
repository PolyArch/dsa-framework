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
def macroop PMINUB_XMM_XMM {
    mmini xmml, xmml, xmmlm, size=1, ext=0
    mmini xmmh, xmmh, xmmhm, size=1, ext=0
};

def macroop PMINUB_XMM_M {
    ldfp ufp1, seg, sib, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, sib, "DISPLACEMENT + 8", dataSize=8
    mmini xmml, xmml, ufp1, size=1, ext=0
    mmini xmmh, xmmh, ufp2, size=1, ext=0
};

def macroop PMINUB_XMM_P {
    rdip t7
    ldfp ufp1, seg, riprel, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, riprel, "DISPLACEMENT + 8", dataSize=8
    mmini xmml, xmml, ufp1, size=1, ext=0
    mmini xmmh, xmmh, ufp2, size=1, ext=0
};

def macroop PMINSW_XMM_XMM {
    mmini xmml, xmml, xmmlm, size=2, ext=Signed
    mmini xmmh, xmmh, xmmhm, size=2, ext=Signed
};

def macroop PMINSW_XMM_M {
    ldfp ufp1, seg, sib, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, sib, "DISPLACEMENT + 8", dataSize=8
    mmini xmml, xmml, ufp1, size=2, ext=Signed
    mmini xmmh, xmmh, ufp2, size=2, ext=Signed
};

def macroop PMINSW_XMM_P {
    rdip t7
    ldfp ufp1, seg, riprel, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, riprel, "DISPLACEMENT + 8", dataSize=8
    mmini xmml, xmml, ufp1, size=2, ext=Signed
    mmini xmmh, xmmh, ufp2, size=2, ext=Signed
};

def macroop PMAXUB_XMM_XMM {
    mmaxi xmml, xmml, xmmlm, size=1, ext=0
    mmaxi xmmh, xmmh, xmmhm, size=1, ext=0
};

def macroop PMAXUB_XMM_M {
    ldfp ufp1, seg, sib, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, sib, "DISPLACEMENT + 8", dataSize=8
    mmaxi xmml, xmml, ufp1, size=1, ext=0
    mmaxi xmmh, xmmh, ufp2, size=1, ext=0
};

def macroop PMAXUB_XMM_P {
    rdip t7
    ldfp ufp1, seg, riprel, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, riprel, "DISPLACEMENT + 8", dataSize=8
    mmaxi xmml, xmml, ufp1, size=1, ext=0
    mmaxi xmmh, xmmh, ufp2, size=1, ext=0
};

def macroop PMAXSW_XMM_XMM {
    mmaxi xmml, xmml, xmmlm, size=2, ext=Signed
    mmaxi xmmh, xmmh, xmmhm, size=2, ext=Signed
};

def macroop PMAXSW_XMM_M {
    ldfp ufp1, seg, sib, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, sib, "DISPLACEMENT + 8", dataSize=8
    mmaxi xmml, xmml, ufp1, size=2, ext=Signed
    mmaxi xmmh, xmmh, ufp2, size=2, ext=Signed
};

def macroop PMAXSW_XMM_P {
    rdip t7
    ldfp ufp1, seg, riprel, "DISPLACEMENT", dataSize=8
    ldfp ufp2, seg, riprel, "DISPLACEMENT + 8", dataSize=8
    mmaxi xmml, xmml, ufp1, size=2, ext=Signed
    mmaxi xmmh, xmmh, ufp2, size=2, ext=Signed
};
'''
