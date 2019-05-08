/*
 * Copyright (c) 2009 The Regents of The University of Michigan
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
 * Authors: Gabe Black
 */

#include "arch/sparc/isa.hh"

#include "arch/sparc/asi.hh"
#include "arch/sparc/decoder.hh"
#include "base/bitfield.hh"
#include "base/trace.hh"
#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "debug/MiscRegs.hh"
#include "debug/Timer.hh"
#include "params/SparcISA.hh"

namespace SparcISA
{

static PSTATE
buildPstateMask()
{
    PSTATE mask = 0;
    mask.ie = 1;
    mask.priv = 1;
    mask.am = 1;
    mask.pef = 1;
    mask.mm = 3;
    mask.tle = 1;
    mask.cle = 1;
    mask.pid1 = 1;
    return mask;
}

static const PSTATE PstateMask = buildPstateMask();

ISA::ISA(Params *p)
    : SimObject(p)
{
    tickCompare = NULL;
    sTickCompare = NULL;
    hSTickCompare = NULL;

    clear();
}

const SparcISAParams *
ISA::params() const
{
    return dynamic_cast<const Params *>(_params);
}

void
ISA::reloadRegMap()
{
    installGlobals(gl, CurrentGlobalsOffset);
    installWindow(cwp, CurrentWindowOffset);
    // Microcode registers.
    for (int i = 0; i < NumMicroIntRegs; i++)
        intRegMap[MicroIntOffset + i] = i + TotalGlobals + NWindows * 16;
    installGlobals(gl, NextGlobalsOffset);
    installWindow(cwp - 1, NextWindowOffset);
    installGlobals(gl, PreviousGlobalsOffset);
    installWindow(cwp + 1, PreviousWindowOffset);
}

void
ISA::installWindow(int cwp, int offset)
{
    assert(offset >= 0 && offset + NumWindowedRegs <= NumIntRegs);
    RegIndex *mapChunk = intRegMap + offset;
    for (int i = 0; i < NumWindowedRegs; i++)
        mapChunk[i] = TotalGlobals +
            ((i - cwp * RegsPerWindow + TotalWindowed) % (TotalWindowed));
}

void
ISA::installGlobals(int gl, int offset)
{
    assert(offset >= 0 && offset + NumGlobalRegs <= NumIntRegs);
    RegIndex *mapChunk = intRegMap + offset;
    mapChunk[0] = 0;
    for (int i = 1; i < NumGlobalRegs; i++)
        mapChunk[i] = i + gl * NumGlobalRegs;
}

void
ISA::clear()
{
    cwp = 0;
    gl = 0;
    reloadRegMap();

    // y = 0;
    // ccr = 0;
    asi = 0;
    tick = ULL(1) << 63;
    fprs = 0;
    gsr = 0;
    softint = 0;
    tick_cmpr = 0;
    stick = 0;
    stick_cmpr = 0;
    memset(tpc, 0, sizeof(tpc));
    memset(tnpc, 0, sizeof(tnpc));
    memset(tstate, 0, sizeof(tstate));
    memset(tt, 0, sizeof(tt));
    tba = 0;
    pstate = 0;
    tl = 0;
    pil = 0;
    // cansave = 0;
    // canrestore = 0;
    // cleanwin = 0;
    // otherwin = 0;
    // wstate = 0;
    // In a T1, bit 11 is apparently always 1
    hpstate = 0;
    hpstate.id = 1;
    memset(htstate, 0, sizeof(htstate));
    hintp = 0;
    htba = 0;
    hstick_cmpr = 0;
    // This is set this way in Legion for some reason
    strandStatusReg = 0x50000;
    fsr = 0;

    priContext = 0;
    secContext = 0;
    partId = 0;
    lsuCtrlReg = 0;

    memset(scratchPad, 0, sizeof(scratchPad));

    cpu_mondo_head = 0;
    cpu_mondo_tail = 0;
    dev_mondo_head = 0;
    dev_mondo_tail = 0;
    res_error_head = 0;
    res_error_tail = 0;
    nres_error_head = 0;
    nres_error_tail = 0;

    // If one of these events is active, it's not obvious to me how to get
    // rid of it cleanly. For now we'll just assert that they're not.
    if (tickCompare != NULL && sTickCompare != NULL && hSTickCompare != NULL)
        panic("Tick comparison event active when clearing the ISA object.\n");
}

MiscReg
ISA::readMiscRegNoEffect(int miscReg) const
{

  // The three miscRegs are moved up from the switch statement
  // due to more frequent calls.

  if (miscReg == MISCREG_GL)
    return gl;
  if (miscReg == MISCREG_CWP)
    return cwp;
  if (miscReg == MISCREG_TLB_DATA) {
    /* Package up all the data for the tlb:
     * 6666555555555544444444443333333333222222222211111111110000000000
     * 3210987654321098765432109876543210987654321098765432109876543210
     *   secContext   | priContext    |             |tl|partid|  |||||^hpriv
     *                                                           ||||^red
     *                                                           |||^priv
     *                                                           ||^am
     *                                                           |^lsuim
     *                                                           ^lsudm
     */
    return      (uint64_t)hpstate.hpriv |
                (uint64_t)hpstate.red << 1 |
                (uint64_t)pstate.priv << 2 |
                (uint64_t)pstate.am << 3 |
           bits((uint64_t)lsuCtrlReg,3,2) << 4 |
           bits((uint64_t)partId,7,0) << 8 |
           bits((uint64_t)tl,2,0) << 16 |
                (uint64_t)priContext << 32 |
                (uint64_t)secContext << 48;
  }

    switch (miscReg) {
      // case MISCREG_TLB_DATA:
      //  [original contents see above]
      // case MISCREG_Y:
      //  return y;
      // case MISCREG_CCR:
      //  return ccr;
      case MISCREG_ASI:
        return asi;
      case MISCREG_FPRS:
        return fprs;
      case MISCREG_TICK:
        return tick;
      case MISCREG_PCR:
        panic("PCR not implemented\n");
      case MISCREG_PIC:
        panic("PIC not implemented\n");
      case MISCREG_GSR:
        return gsr;
      case MISCREG_SOFTINT:
        return softint;
      case MISCREG_TICK_CMPR:
        return tick_cmpr;
      case MISCREG_STICK:
        return stick;
      case MISCREG_STICK_CMPR:
        return stick_cmpr;

        /** Privilged Registers */
      case MISCREG_TPC:
        return tpc[tl-1];
      case MISCREG_TNPC:
        return tnpc[tl-1];
      case MISCREG_TSTATE:
        return tstate[tl-1];
      case MISCREG_TT:
        return tt[tl-1];
      case MISCREG_PRIVTICK:
        panic("Priviliged access to tick registers not implemented\n");
      case MISCREG_TBA:
        return tba;
      case MISCREG_PSTATE:
        return (MiscReg)pstate;
      case MISCREG_TL:
        return tl;
      case MISCREG_PIL:
        return pil;
      // CWP, GL moved
      // case MISCREG_CWP:
      //   return cwp;
      // case MISCREG_CANSAVE:
      //   return cansave;
      // case MISCREG_CANRESTORE:
      //   return canrestore;
      // case MISCREG_CLEANWIN:
      //   return cleanwin;
      // case MISCREG_OTHERWIN:
      //   return otherwin;
      // case MISCREG_WSTATE:
      //   return wstate;
      // case MISCREG_GL:
      //   return gl;

        /** Hyper privileged registers */
      case MISCREG_HPSTATE:
        return (MiscReg)hpstate;
      case MISCREG_HTSTATE:
        return htstate[tl-1];
      case MISCREG_HINTP:
        return hintp;
      case MISCREG_HTBA:
        return htba;
      case MISCREG_STRAND_STS_REG:
        return strandStatusReg;
      case MISCREG_HSTICK_CMPR:
        return hstick_cmpr;

        /** Floating Point Status Register */
      case MISCREG_FSR:
        DPRINTF(MiscRegs, "FSR read as: %#x\n", fsr);
        return fsr;

      case MISCREG_MMU_P_CONTEXT:
        return priContext;
      case MISCREG_MMU_S_CONTEXT:
        return secContext;
      case MISCREG_MMU_PART_ID:
        return partId;
      case MISCREG_MMU_LSU_CTRL:
        return lsuCtrlReg;

      case MISCREG_SCRATCHPAD_R0:
        return scratchPad[0];
      case MISCREG_SCRATCHPAD_R1:
        return scratchPad[1];
      case MISCREG_SCRATCHPAD_R2:
        return scratchPad[2];
      case MISCREG_SCRATCHPAD_R3:
        return scratchPad[3];
      case MISCREG_SCRATCHPAD_R4:
        return scratchPad[4];
      case MISCREG_SCRATCHPAD_R5:
        return scratchPad[5];
      case MISCREG_SCRATCHPAD_R6:
        return scratchPad[6];
      case MISCREG_SCRATCHPAD_R7:
        return scratchPad[7];
      case MISCREG_QUEUE_CPU_MONDO_HEAD:
        return cpu_mondo_head;
      case MISCREG_QUEUE_CPU_MONDO_TAIL:
        return cpu_mondo_tail;
      case MISCREG_QUEUE_DEV_MONDO_HEAD:
        return dev_mondo_head;
      case MISCREG_QUEUE_DEV_MONDO_TAIL:
        return dev_mondo_tail;
      case MISCREG_QUEUE_RES_ERROR_HEAD:
        return res_error_head;
      case MISCREG_QUEUE_RES_ERROR_TAIL:
        return res_error_tail;
      case MISCREG_QUEUE_NRES_ERROR_HEAD:
        return nres_error_head;
      case MISCREG_QUEUE_NRES_ERROR_TAIL:
        return nres_error_tail;
      default:
        panic("Miscellaneous register %d not implemented\n", miscReg);
    }
}

MiscReg
ISA::readMiscReg(int miscReg, ThreadContext * tc)
{
    switch (miscReg) {
        // tick and stick are aliased to each other in niagra
        // well store the tick data in stick and the interrupt bit in tick
      case MISCREG_STICK:
      case MISCREG_TICK:
      case MISCREG_PRIVTICK:
        // I'm not sure why legion ignores the lowest two bits, but we'll go
        // with it
        // change from curCycle() to instCount() until we're done with legion
        DPRINTF(Timer, "Instruction Count when TICK read: %#X stick=%#X\n",
                tc->getCpuPtr()->instCount(), stick);
        return mbits(tc->getCpuPtr()->instCount() + (int64_t)stick,62,2) |
               mbits(tick,63,63);
      case MISCREG_FPRS:
        // in legion if fp is enabled du and dl are set
        return fprs | 0x3;
      case MISCREG_PCR:
      case MISCREG_PIC:
        panic("Performance Instrumentation not impl\n");
      case MISCREG_SOFTINT_CLR:
      case MISCREG_SOFTINT_SET:
        panic("Can read from softint clr/set\n");
      case MISCREG_SOFTINT:
      case MISCREG_TICK_CMPR:
      case MISCREG_STICK_CMPR:
      case MISCREG_HINTP:
      case MISCREG_HTSTATE:
      case MISCREG_HTBA:
      case MISCREG_HVER:
      case MISCREG_STRAND_STS_REG:
      case MISCREG_HSTICK_CMPR:
      case MISCREG_QUEUE_CPU_MONDO_HEAD:
      case MISCREG_QUEUE_CPU_MONDO_TAIL:
      case MISCREG_QUEUE_DEV_MONDO_HEAD:
      case MISCREG_QUEUE_DEV_MONDO_TAIL:
      case MISCREG_QUEUE_RES_ERROR_HEAD:
      case MISCREG_QUEUE_RES_ERROR_TAIL:
      case MISCREG_QUEUE_NRES_ERROR_HEAD:
      case MISCREG_QUEUE_NRES_ERROR_TAIL:
      case MISCREG_HPSTATE:
        return readFSReg(miscReg, tc);
    }
    return readMiscRegNoEffect(miscReg);
}

void
ISA::setMiscRegNoEffect(int miscReg, MiscReg val)
{
    switch (miscReg) {
//      case MISCREG_Y:
//        y = val;
//        break;
//      case MISCREG_CCR:
//        ccr = val;
//        break;
      case MISCREG_ASI:
        asi = val;
        break;
      case MISCREG_FPRS:
        fprs = val;
        break;
      case MISCREG_TICK:
        tick = val;
        break;
      case MISCREG_PCR:
        panic("PCR not implemented\n");
      case MISCREG_PIC:
        panic("PIC not implemented\n");
      case MISCREG_GSR:
        gsr = val;
        break;
      case MISCREG_SOFTINT:
        softint = val;
        break;
      case MISCREG_TICK_CMPR:
        tick_cmpr = val;
        break;
      case MISCREG_STICK:
        stick = val;
        break;
      case MISCREG_STICK_CMPR:
        stick_cmpr = val;
        break;

        /** Privilged Registers */
      case MISCREG_TPC:
        tpc[tl-1] = val;
        break;
      case MISCREG_TNPC:
        tnpc[tl-1] = val;
        break;
      case MISCREG_TSTATE:
        tstate[tl-1] = val;
        break;
      case MISCREG_TT:
        tt[tl-1] = val;
        break;
      case MISCREG_PRIVTICK:
        panic("Priviliged access to tick regesiters not implemented\n");
      case MISCREG_TBA:
        // clear lower 7 bits on writes.
        tba = val & ULL(~0x7FFF);
        break;
      case MISCREG_PSTATE:
        pstate = (val & PstateMask);
        break;
      case MISCREG_TL:
        tl = val;
        break;
      case MISCREG_PIL:
        pil = val;
        break;
      case MISCREG_CWP:
        cwp = val;
        break;
//      case MISCREG_CANSAVE:
//        cansave = val;
//        break;
//      case MISCREG_CANRESTORE:
//        canrestore = val;
//        break;
//      case MISCREG_CLEANWIN:
//        cleanwin = val;
//        break;
//      case MISCREG_OTHERWIN:
//        otherwin = val;
//        break;
//      case MISCREG_WSTATE:
//        wstate = val;
//        break;
      case MISCREG_GL:
        gl = val;
        break;

        /** Hyper privileged registers */
      case MISCREG_HPSTATE:
        hpstate = val;
        break;
      case MISCREG_HTSTATE:
        htstate[tl-1] = val;
        break;
      case MISCREG_HINTP:
        hintp = val;
        break;
      case MISCREG_HTBA:
        htba = val;
        break;
      case MISCREG_STRAND_STS_REG:
        strandStatusReg = val;
        break;
      case MISCREG_HSTICK_CMPR:
        hstick_cmpr = val;
        break;

        /** Floating Point Status Register */
      case MISCREG_FSR:
        fsr = val;
        DPRINTF(MiscRegs, "FSR written with: %#x\n", fsr);
        break;

      case MISCREG_MMU_P_CONTEXT:
        priContext = val;
        break;
      case MISCREG_MMU_S_CONTEXT:
        secContext = val;
        break;
      case MISCREG_MMU_PART_ID:
        partId = val;
        break;
      case MISCREG_MMU_LSU_CTRL:
        lsuCtrlReg = val;
        break;

      case MISCREG_SCRATCHPAD_R0:
        scratchPad[0] = val;
        break;
      case MISCREG_SCRATCHPAD_R1:
        scratchPad[1] = val;
        break;
      case MISCREG_SCRATCHPAD_R2:
        scratchPad[2] = val;
        break;
      case MISCREG_SCRATCHPAD_R3:
        scratchPad[3] = val;
        break;
      case MISCREG_SCRATCHPAD_R4:
        scratchPad[4] = val;
        break;
      case MISCREG_SCRATCHPAD_R5:
        scratchPad[5] = val;
        break;
      case MISCREG_SCRATCHPAD_R6:
        scratchPad[6] = val;
        break;
      case MISCREG_SCRATCHPAD_R7:
        scratchPad[7] = val;
        break;
      case MISCREG_QUEUE_CPU_MONDO_HEAD:
        cpu_mondo_head = val;
        break;
      case MISCREG_QUEUE_CPU_MONDO_TAIL:
        cpu_mondo_tail = val;
        break;
      case MISCREG_QUEUE_DEV_MONDO_HEAD:
        dev_mondo_head = val;
        break;
      case MISCREG_QUEUE_DEV_MONDO_TAIL:
        dev_mondo_tail = val;
        break;
      case MISCREG_QUEUE_RES_ERROR_HEAD:
        res_error_head = val;
        break;
      case MISCREG_QUEUE_RES_ERROR_TAIL:
        res_error_tail = val;
        break;
      case MISCREG_QUEUE_NRES_ERROR_HEAD:
        nres_error_head = val;
        break;
      case MISCREG_QUEUE_NRES_ERROR_TAIL:
        nres_error_tail = val;
        break;
      default:
        panic("Miscellaneous register %d not implemented\n", miscReg);
    }
}

void
ISA::setMiscReg(int miscReg, MiscReg val, ThreadContext * tc)
{
    MiscReg new_val = val;

    switch (miscReg) {
      case MISCREG_ASI:
        tc->getDecoderPtr()->setContext(val);
        break;
      case MISCREG_STICK:
      case MISCREG_TICK:
        // stick and tick are same thing on niagra
        // use stick for offset and tick for holding intrrupt bit
        stick = mbits(val,62,0) - tc->getCpuPtr()->instCount();
        tick = mbits(val,63,63);
        DPRINTF(Timer, "Writing TICK=%#X\n", val);
        break;
      case MISCREG_FPRS:
        // Configure the fpu based on the fprs
        break;
      case MISCREG_PCR:
        // Set up performance counting based on pcr value
        break;
      case MISCREG_PSTATE:
        pstate = val & PstateMask;
        return;
      case MISCREG_TL:
        {
            tl = val;
            if (hpstate.tlz && tl == 0 && !hpstate.hpriv)
                tc->getCpuPtr()->postInterrupt(0, IT_TRAP_LEVEL_ZERO, 0);
            else
                tc->getCpuPtr()->clearInterrupt(0, IT_TRAP_LEVEL_ZERO, 0);
            return;
        }
      case MISCREG_CWP:
        new_val = val >= NWindows ? NWindows - 1 : val;
        if (val >= NWindows)
            new_val = NWindows - 1;

        installWindow(new_val, CurrentWindowOffset);
        installWindow(new_val - 1, NextWindowOffset);
        installWindow(new_val + 1, PreviousWindowOffset);
        break;
      case MISCREG_GL:
        installGlobals(val, CurrentGlobalsOffset);
        installGlobals(val, NextGlobalsOffset);
        installGlobals(val, PreviousGlobalsOffset);
        break;
      case MISCREG_PIL:
      case MISCREG_SOFTINT:
      case MISCREG_SOFTINT_SET:
      case MISCREG_SOFTINT_CLR:
      case MISCREG_TICK_CMPR:
      case MISCREG_STICK_CMPR:
      case MISCREG_HINTP:
      case MISCREG_HTSTATE:
      case MISCREG_HTBA:
      case MISCREG_HVER:
      case MISCREG_STRAND_STS_REG:
      case MISCREG_HSTICK_CMPR:
      case MISCREG_QUEUE_CPU_MONDO_HEAD:
      case MISCREG_QUEUE_CPU_MONDO_TAIL:
      case MISCREG_QUEUE_DEV_MONDO_HEAD:
      case MISCREG_QUEUE_DEV_MONDO_TAIL:
      case MISCREG_QUEUE_RES_ERROR_HEAD:
      case MISCREG_QUEUE_RES_ERROR_TAIL:
      case MISCREG_QUEUE_NRES_ERROR_HEAD:
      case MISCREG_QUEUE_NRES_ERROR_TAIL:
      case MISCREG_HPSTATE:
        setFSReg(miscReg, val, tc);
        return;
    }
    setMiscRegNoEffect(miscReg, new_val);
}

void
ISA::serialize(CheckpointOut &cp) const
{
    SERIALIZE_SCALAR(asi);
    SERIALIZE_SCALAR(tick);
    SERIALIZE_SCALAR(fprs);
    SERIALIZE_SCALAR(gsr);
    SERIALIZE_SCALAR(softint);
    SERIALIZE_SCALAR(tick_cmpr);
    SERIALIZE_SCALAR(stick);
    SERIALIZE_SCALAR(stick_cmpr);
    SERIALIZE_ARRAY(tpc,MaxTL);
    SERIALIZE_ARRAY(tnpc,MaxTL);
    SERIALIZE_ARRAY(tstate,MaxTL);
    SERIALIZE_ARRAY(tt,MaxTL);
    SERIALIZE_SCALAR(tba);
    SERIALIZE_SCALAR(pstate);
    SERIALIZE_SCALAR(tl);
    SERIALIZE_SCALAR(pil);
    SERIALIZE_SCALAR(cwp);
    SERIALIZE_SCALAR(gl);
    SERIALIZE_SCALAR(hpstate);
    SERIALIZE_ARRAY(htstate,MaxTL);
    SERIALIZE_SCALAR(hintp);
    SERIALIZE_SCALAR(htba);
    SERIALIZE_SCALAR(hstick_cmpr);
    SERIALIZE_SCALAR(strandStatusReg);
    SERIALIZE_SCALAR(fsr);
    SERIALIZE_SCALAR(priContext);
    SERIALIZE_SCALAR(secContext);
    SERIALIZE_SCALAR(partId);
    SERIALIZE_SCALAR(lsuCtrlReg);
    SERIALIZE_ARRAY(scratchPad,8);
    SERIALIZE_SCALAR(cpu_mondo_head);
    SERIALIZE_SCALAR(cpu_mondo_tail);
    SERIALIZE_SCALAR(dev_mondo_head);
    SERIALIZE_SCALAR(dev_mondo_tail);
    SERIALIZE_SCALAR(res_error_head);
    SERIALIZE_SCALAR(res_error_tail);
    SERIALIZE_SCALAR(nres_error_head);
    SERIALIZE_SCALAR(nres_error_tail);
    Tick tick_cmp = 0, stick_cmp = 0, hstick_cmp = 0;
    ThreadContext *tc = NULL;
    BaseCPU *cpu = NULL;
    int tc_num = 0;
    bool tick_intr_sched = true;

    if (tickCompare)
        tc = tickCompare->getTC();
    else if (sTickCompare)
        tc = sTickCompare->getTC();
    else if (hSTickCompare)
        tc = hSTickCompare->getTC();
    else
        tick_intr_sched = false;

    SERIALIZE_SCALAR(tick_intr_sched);

    if (tc) {
        cpu = tc->getCpuPtr();
        tc_num = cpu->findContext(tc);
        if (tickCompare && tickCompare->scheduled())
            tick_cmp = tickCompare->when();
        if (sTickCompare && sTickCompare->scheduled())
            stick_cmp = sTickCompare->when();
        if (hSTickCompare && hSTickCompare->scheduled())
            hstick_cmp = hSTickCompare->when();

        SERIALIZE_OBJPTR(cpu);
        SERIALIZE_SCALAR(tc_num);
        SERIALIZE_SCALAR(tick_cmp);
        SERIALIZE_SCALAR(stick_cmp);
        SERIALIZE_SCALAR(hstick_cmp);
    }
}

void
ISA::unserialize(CheckpointIn &cp)
{
    UNSERIALIZE_SCALAR(asi);
    UNSERIALIZE_SCALAR(tick);
    UNSERIALIZE_SCALAR(fprs);
    UNSERIALIZE_SCALAR(gsr);
    UNSERIALIZE_SCALAR(softint);
    UNSERIALIZE_SCALAR(tick_cmpr);
    UNSERIALIZE_SCALAR(stick);
    UNSERIALIZE_SCALAR(stick_cmpr);
    UNSERIALIZE_ARRAY(tpc,MaxTL);
    UNSERIALIZE_ARRAY(tnpc,MaxTL);
    UNSERIALIZE_ARRAY(tstate,MaxTL);
    UNSERIALIZE_ARRAY(tt,MaxTL);
    UNSERIALIZE_SCALAR(tba);
    {
        uint16_t pstate;
        UNSERIALIZE_SCALAR(pstate);
        this->pstate = pstate;
    }
    UNSERIALIZE_SCALAR(tl);
    UNSERIALIZE_SCALAR(pil);
    UNSERIALIZE_SCALAR(cwp);
    UNSERIALIZE_SCALAR(gl);
    reloadRegMap();
    {
        uint64_t hpstate;
        UNSERIALIZE_SCALAR(hpstate);
        this->hpstate = hpstate;
    }
    UNSERIALIZE_ARRAY(htstate,MaxTL);
    UNSERIALIZE_SCALAR(hintp);
    UNSERIALIZE_SCALAR(htba);
    UNSERIALIZE_SCALAR(hstick_cmpr);
    UNSERIALIZE_SCALAR(strandStatusReg);
    UNSERIALIZE_SCALAR(fsr);
    UNSERIALIZE_SCALAR(priContext);
    UNSERIALIZE_SCALAR(secContext);
    UNSERIALIZE_SCALAR(partId);
    UNSERIALIZE_SCALAR(lsuCtrlReg);
    UNSERIALIZE_ARRAY(scratchPad,8);
    UNSERIALIZE_SCALAR(cpu_mondo_head);
    UNSERIALIZE_SCALAR(cpu_mondo_tail);
    UNSERIALIZE_SCALAR(dev_mondo_head);
    UNSERIALIZE_SCALAR(dev_mondo_tail);
    UNSERIALIZE_SCALAR(res_error_head);
    UNSERIALIZE_SCALAR(res_error_tail);
    UNSERIALIZE_SCALAR(nres_error_head);
    UNSERIALIZE_SCALAR(nres_error_tail);

    Tick tick_cmp = 0, stick_cmp = 0, hstick_cmp = 0;
    ThreadContext *tc = NULL;
    BaseCPU *cpu = NULL;
    int tc_num;
    bool tick_intr_sched;
    UNSERIALIZE_SCALAR(tick_intr_sched);
    if (tick_intr_sched) {
        UNSERIALIZE_OBJPTR(cpu);
        if (cpu) {
            UNSERIALIZE_SCALAR(tc_num);
            UNSERIALIZE_SCALAR(tick_cmp);
            UNSERIALIZE_SCALAR(stick_cmp);
            UNSERIALIZE_SCALAR(hstick_cmp);
            tc = cpu->getContext(tc_num);

            if (tick_cmp) {
                tickCompare = new TickCompareEvent(this, tc);
                schedule(tickCompare, tick_cmp);
            }
            if (stick_cmp)  {
                sTickCompare = new STickCompareEvent(this, tc);
                schedule(sTickCompare, stick_cmp);
            }
            if (hstick_cmp)  {
                hSTickCompare = new HSTickCompareEvent(this, tc);
                schedule(hSTickCompare, hstick_cmp);
            }
        }
    }

}

}

SparcISA::ISA *
SparcISAParams::create()
{
    return new SparcISA::ISA(this);
}
