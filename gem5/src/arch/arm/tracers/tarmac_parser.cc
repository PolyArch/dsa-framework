/*
 * Copyright (c) 2011,2017-2018 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
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
 * Authors: Giacomo Gabrielli
 */

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <string>

#include "arch/arm/tracers/tarmac_parser.hh"

#include "arch/arm/tlb.hh"
#include "arch/arm/insts/static_inst.hh"
#include "config/the_isa.hh"
#include "cpu/static_inst.hh"
#include "cpu/thread_context.hh"
#include "mem/fs_translating_port_proxy.hh"
#include "mem/packet.hh"
#include "sim/core.hh"
#include "sim/faults.hh"
#include "sim/sim_exit.hh"

using namespace std;
using namespace TheISA;

namespace Trace {

// TARMAC Parser static variables
const int TarmacParserRecord::MaxLineLength;

TarmacParserRecord::ParserInstEntry TarmacParserRecord::instRecord;
TarmacParserRecord::ParserRegEntry TarmacParserRecord::regRecord;
TarmacParserRecord::ParserMemEntry TarmacParserRecord::memRecord;
TarmacBaseRecord::TarmacRecordType TarmacParserRecord::currRecordType;

list<TarmacParserRecord::ParserRegEntry> TarmacParserRecord::destRegRecords;
char TarmacParserRecord::buf[TarmacParserRecord::MaxLineLength];
TarmacParserRecord::MiscRegMap TarmacParserRecord::miscRegMap = {

    { "cpsr", MISCREG_CPSR },
    { "nzcv", MISCREG_NZCV },

    // AArch32 CP14 registers
    { "dbgdidr", MISCREG_DBGDIDR },
    { "dbgdscrint", MISCREG_DBGDSCRint },
    { "dbgdccint", MISCREG_DBGDCCINT },
    { "dbgdtrtxint", MISCREG_DBGDTRTXint },
    { "dbgdtrrxint", MISCREG_DBGDTRRXint },
    { "dbgwfar", MISCREG_DBGWFAR },
    { "dbgvcr", MISCREG_DBGVCR },
    { "dbgdtrrxext", MISCREG_DBGDTRRXext },
    { "dbgdscrext", MISCREG_DBGDSCRext },
    { "dbgdtrtxext", MISCREG_DBGDTRTXext },
    { "dbgoseccr", MISCREG_DBGOSECCR },
    { "dbgbvr0", MISCREG_DBGBVR0 },
    { "dbgbvr1", MISCREG_DBGBVR1 },
    { "dbgbvr2", MISCREG_DBGBVR2 },
    { "dbgbvr3", MISCREG_DBGBVR3 },
    { "dbgbvr4", MISCREG_DBGBVR4 },
    { "dbgbvr5", MISCREG_DBGBVR5 },
    { "dbgbcr0", MISCREG_DBGBCR0 },
    { "dbgbcr1", MISCREG_DBGBCR1 },
    { "dbgbcr2", MISCREG_DBGBCR2 },
    { "dbgbcr3", MISCREG_DBGBCR3 },
    { "dbgbcr4", MISCREG_DBGBCR4 },
    { "dbgbcr5", MISCREG_DBGBCR5 },
    { "dbgwvr0", MISCREG_DBGWVR0 },
    { "dbgwvr1", MISCREG_DBGWVR1 },
    { "dbgwvr2", MISCREG_DBGWVR2 },
    { "dbgwvr3", MISCREG_DBGWVR3 },
    { "dbgwcr0", MISCREG_DBGWCR0 },
    { "dbgwcr1", MISCREG_DBGWCR1 },
    { "dbgwcr2", MISCREG_DBGWCR2 },
    { "dbgwcr3", MISCREG_DBGWCR3 },
    { "dbgdrar", MISCREG_DBGDRAR },
    { "dbgbxvr4", MISCREG_DBGBXVR4 },
    { "dbgbxvr5", MISCREG_DBGBXVR5 },
    { "dbgoslar", MISCREG_DBGOSLAR },
    { "dbgoslsr", MISCREG_DBGOSLSR },
    { "dbgosdlr", MISCREG_DBGOSDLR },
    { "dbgprcr", MISCREG_DBGPRCR },
    { "dbgdsar", MISCREG_DBGDSAR },
    { "dbgclaimset", MISCREG_DBGCLAIMSET },
    { "dbgclaimclr", MISCREG_DBGCLAIMCLR },
    { "dbgauthstatus", MISCREG_DBGAUTHSTATUS },
    { "dbgdevid2", MISCREG_DBGDEVID2 },
    { "dbgdevid1", MISCREG_DBGDEVID1 },
    { "dbgdevid0", MISCREG_DBGDEVID0 },
    { "teecr", MISCREG_TEECR },
    { "jidr", MISCREG_JIDR },
    { "teehbr", MISCREG_TEEHBR },
    { "joscr", MISCREG_JOSCR },
    { "jmcr", MISCREG_JMCR },

    // AArch32 CP15 registers
    { "midr", MISCREG_MIDR },
    { "ctr", MISCREG_CTR },
    { "tcmtr", MISCREG_TCMTR },
    { "tlbtr", MISCREG_TLBTR },
    { "mpidr", MISCREG_MPIDR },
    { "revidr", MISCREG_REVIDR },
    { "id_pfr0", MISCREG_ID_PFR0 },
    { "id_pfr1", MISCREG_ID_PFR1 },
    { "id_dfr0", MISCREG_ID_DFR0 },
    { "id_afr0", MISCREG_ID_AFR0 },
    { "id_mmfr0", MISCREG_ID_MMFR0 },
    { "id_mmfr1", MISCREG_ID_MMFR1 },
    { "id_mmfr2", MISCREG_ID_MMFR2 },
    { "id_mmfr3", MISCREG_ID_MMFR3 },
    { "id_isar0", MISCREG_ID_ISAR0 },
    { "id_isar1", MISCREG_ID_ISAR1 },
    { "id_isar2", MISCREG_ID_ISAR2 },
    { "id_isar3", MISCREG_ID_ISAR3 },
    { "id_isar4", MISCREG_ID_ISAR4 },
    { "id_isar5", MISCREG_ID_ISAR5 },
    { "ccsidr", MISCREG_CCSIDR },
    { "clidr", MISCREG_CLIDR },
    { "aidr", MISCREG_AIDR },
    { "csselr_ns", MISCREG_CSSELR_NS },
    { "csselr_s", MISCREG_CSSELR_S },
    { "vpidr", MISCREG_VPIDR },
    { "vmpidr", MISCREG_VMPIDR },
    { "sctlr_ns", MISCREG_SCTLR_NS },
    { "sctlr_s", MISCREG_SCTLR_S },
    { "actlr_ns", MISCREG_ACTLR_NS },
    { "actlr_s", MISCREG_ACTLR_S },
    { "cpacr", MISCREG_CPACR },
    { "scr", MISCREG_SCR },
    { "sder", MISCREG_SDER },
    { "nsacr", MISCREG_NSACR },
    { "hsctlr", MISCREG_HSCTLR },
    { "hactlr", MISCREG_HACTLR },
    { "hcr", MISCREG_HCR },
    { "hdcr", MISCREG_HDCR },
    { "hcptr", MISCREG_HCPTR },
    { "hstr", MISCREG_HSTR },
    { "hacr", MISCREG_HACR },
    { "ttbr0_ns", MISCREG_TTBR0_NS },
    { "ttbr0_s", MISCREG_TTBR0_S },
    { "ttbr1_ns", MISCREG_TTBR1_NS },
    { "ttbr1_s", MISCREG_TTBR1_S },
    { "ttbcr_ns", MISCREG_TTBCR_NS },
    { "ttbcr_s", MISCREG_TTBCR_S },
    { "htcr", MISCREG_HTCR },
    { "vtcr", MISCREG_VTCR },
    { "dacr_ns", MISCREG_DACR_NS },
    { "dacr_s", MISCREG_DACR_S },
    { "dfsr_ns", MISCREG_DFSR_NS },
    { "dfsr_s", MISCREG_DFSR_S },
    { "ifsr_ns", MISCREG_IFSR_NS },
    { "ifsr_s", MISCREG_IFSR_S },
    { "adfsr_ns", MISCREG_ADFSR_NS },
    { "adfsr_s", MISCREG_ADFSR_S },
    { "aifsr_ns", MISCREG_AIFSR_NS },
    { "aifsr_s", MISCREG_AIFSR_S },
    { "hadfsr", MISCREG_HADFSR },
    { "haifsr", MISCREG_HAIFSR },
    { "hsr", MISCREG_HSR },
    { "dfar_ns", MISCREG_DFAR_NS },
    { "dfar_s", MISCREG_DFAR_S },
    { "ifar_ns", MISCREG_IFAR_NS },
    { "ifar_s", MISCREG_IFAR_S },
    { "hdfar", MISCREG_HDFAR },
    { "hifar", MISCREG_HIFAR },
    { "hpfar", MISCREG_HPFAR },
    { "icialluis", MISCREG_ICIALLUIS },
    { "bpiallis", MISCREG_BPIALLIS },
    { "par_ns", MISCREG_PAR_NS },
    { "par_s", MISCREG_PAR_S },
    { "iciallu", MISCREG_ICIALLU },
    { "icimvau", MISCREG_ICIMVAU },
    { "cp15isb", MISCREG_CP15ISB },
    { "bpiall", MISCREG_BPIALL },
    { "bpimva", MISCREG_BPIMVA },
    { "dcimvac", MISCREG_DCIMVAC },
    { "dcisw", MISCREG_DCISW },
    { "ats1cpr", MISCREG_ATS1CPR },
    { "ats1cpw", MISCREG_ATS1CPW },
    { "ats1cur", MISCREG_ATS1CUR },
    { "ats1cuw", MISCREG_ATS1CUW },
    { "ats12nsopr", MISCREG_ATS12NSOPR },
    { "ats12nsopw", MISCREG_ATS12NSOPW },
    { "ats12nsour", MISCREG_ATS12NSOUR },
    { "ats12nsouw", MISCREG_ATS12NSOUW },
    { "dccmvac", MISCREG_DCCMVAC },
    { "dccsw", MISCREG_DCCSW },
    { "cp15dsb", MISCREG_CP15DSB },
    { "cp15dmb", MISCREG_CP15DMB },
    { "dccmvau", MISCREG_DCCMVAU },
    { "dccimvac", MISCREG_DCCIMVAC },
    { "dccisw", MISCREG_DCCISW },
    { "ats1hr", MISCREG_ATS1HR },
    { "ats1hw", MISCREG_ATS1HW },
    { "tlbiallis", MISCREG_TLBIALLIS },
    { "tlbimvais", MISCREG_TLBIMVAIS },
    { "tlbiasidis", MISCREG_TLBIASIDIS },
    { "tlbimvaais", MISCREG_TLBIMVAAIS },
    { "tlbimvalis", MISCREG_TLBIMVALIS },
    { "tlbimvaalis", MISCREG_TLBIMVAALIS },
    { "itlbiall", MISCREG_ITLBIALL },
    { "itlbimva", MISCREG_ITLBIMVA },
    { "itlbiasid", MISCREG_ITLBIASID },
    { "dtlbiall", MISCREG_DTLBIALL },
    { "dtlbimva", MISCREG_DTLBIMVA },
    { "dtlbiasid", MISCREG_DTLBIASID },
    { "tlbiall", MISCREG_TLBIALL },
    { "tlbimva", MISCREG_TLBIMVA },
    { "tlbiasid", MISCREG_TLBIASID },
    { "tlbimvaa", MISCREG_TLBIMVAA },
    { "tlbimval", MISCREG_TLBIMVAL },
    { "tlbimvaal", MISCREG_TLBIMVAAL },
    { "tlbiipas2is", MISCREG_TLBIIPAS2IS },
    { "tlbiipas2lis", MISCREG_TLBIIPAS2LIS },
    { "tlbiallhis", MISCREG_TLBIALLHIS },
    { "tlbimvahis", MISCREG_TLBIMVAHIS },
    { "tlbiallnsnhis", MISCREG_TLBIALLNSNHIS },
    { "tlbimvalhis", MISCREG_TLBIMVALHIS },
    { "tlbiipas2", MISCREG_TLBIIPAS2 },
    { "tlbiipas2l", MISCREG_TLBIIPAS2L },
    { "tlbiallh", MISCREG_TLBIALLH },
    { "tlbimvah", MISCREG_TLBIMVAH },
    { "tlbiallnsnh", MISCREG_TLBIALLNSNH },
    { "tlbimvalh", MISCREG_TLBIMVALH },
    { "pmcr", MISCREG_PMCR },
    { "pmcntenset", MISCREG_PMCNTENSET },
    { "pmcntenclr", MISCREG_PMCNTENCLR },
    { "pmovsr", MISCREG_PMOVSR },
    { "pmswinc", MISCREG_PMSWINC },
    { "pmselr", MISCREG_PMSELR },
    { "pmceid0", MISCREG_PMCEID0 },
    { "pmceid1", MISCREG_PMCEID1 },
    { "pmccntr", MISCREG_PMCCNTR },
    { "pmxevtyper", MISCREG_PMXEVTYPER },
    { "pmccfiltr", MISCREG_PMCCFILTR },
    { "pmxevcntr", MISCREG_PMXEVCNTR },
    { "pmuserenr", MISCREG_PMUSERENR },
    { "pmintenset", MISCREG_PMINTENSET },
    { "pmintenclr", MISCREG_PMINTENCLR },
    { "pmovsset", MISCREG_PMOVSSET },
    { "l2ctlr", MISCREG_L2CTLR },
    { "l2ectlr", MISCREG_L2ECTLR },
    { "prrr_ns", MISCREG_PRRR_NS },
    { "prrr_s", MISCREG_PRRR_S },
    { "mair0_ns", MISCREG_MAIR0_NS },
    { "mair0_s", MISCREG_MAIR0_S },
    { "nmrr_ns", MISCREG_NMRR_NS },
    { "nmrr_s", MISCREG_NMRR_S },
    { "mair1_ns", MISCREG_MAIR1_NS },
    { "mair1_s", MISCREG_MAIR1_S },
    { "amair0_ns", MISCREG_AMAIR0_NS },
    { "amair0_s", MISCREG_AMAIR0_S },
    { "amair1_ns", MISCREG_AMAIR1_NS },
    { "amair1_s", MISCREG_AMAIR1_S },
    { "hmair0", MISCREG_HMAIR0 },
    { "hmair1", MISCREG_HMAIR1 },
    { "hamair0", MISCREG_HAMAIR0 },
    { "hamair1", MISCREG_HAMAIR1 },
    { "vbar_ns", MISCREG_VBAR_NS },
    { "vbar_s", MISCREG_VBAR_S },
    { "mvbar", MISCREG_MVBAR },
    { "rmr", MISCREG_RMR },
    { "isr", MISCREG_ISR },
    { "hvbar", MISCREG_HVBAR },
    { "fcseidr", MISCREG_FCSEIDR },
    { "contextidr_ns", MISCREG_CONTEXTIDR_NS },
    { "contextidr_s", MISCREG_CONTEXTIDR_S },
    { "tpidrurw_ns", MISCREG_TPIDRURW_NS },
    { "tpidrurw_s", MISCREG_TPIDRURW_S },
    { "tpidruro_ns", MISCREG_TPIDRURO_NS },
    { "tpidruro_s", MISCREG_TPIDRURO_S },
    { "tpidrprw_ns", MISCREG_TPIDRPRW_NS },
    { "tpidrprw_s", MISCREG_TPIDRPRW_S },
    { "htpidr", MISCREG_HTPIDR },
    { "cntfrq", MISCREG_CNTFRQ },
    { "cntkctl", MISCREG_CNTKCTL },
    { "cntp_tval_ns", MISCREG_CNTP_TVAL_NS },
    { "cntp_tval_s", MISCREG_CNTP_TVAL_S },
    { "cntp_ctl_ns", MISCREG_CNTP_CTL_NS },
    { "cntp_ctl_s", MISCREG_CNTP_CTL_S },
    { "cntv_tval", MISCREG_CNTV_TVAL },
    { "cntv_ctl", MISCREG_CNTV_CTL },
    { "cnthctl", MISCREG_CNTHCTL },
    { "cnthp_tval", MISCREG_CNTHP_TVAL },
    { "cnthp_ctl", MISCREG_CNTHP_CTL },
    { "il1data0", MISCREG_IL1DATA0 },
    { "il1data1", MISCREG_IL1DATA1 },
    { "il1data2", MISCREG_IL1DATA2 },
    { "il1data3", MISCREG_IL1DATA3 },
    { "dl1data0", MISCREG_DL1DATA0 },
    { "dl1data1", MISCREG_DL1DATA1 },
    { "dl1data2", MISCREG_DL1DATA2 },
    { "dl1data3", MISCREG_DL1DATA3 },
    { "dl1data4", MISCREG_DL1DATA4 },
    { "ramindex", MISCREG_RAMINDEX },
    { "l2actlr", MISCREG_L2ACTLR },
    { "cbar", MISCREG_CBAR },
    { "httbr", MISCREG_HTTBR },
    { "vttbr", MISCREG_VTTBR },
    { "cntpct", MISCREG_CNTPCT },
    { "cntvct", MISCREG_CNTVCT },
    { "cntp_cval_ns", MISCREG_CNTP_CVAL_NS },
    { "cntp_cval_s", MISCREG_CNTP_CVAL_S },
    { "cntv_cval", MISCREG_CNTV_CVAL },
    { "cntvoff", MISCREG_CNTVOFF },
    { "cnthp_cval", MISCREG_CNTHP_CVAL },
    { "cpumerrsr", MISCREG_CPUMERRSR },
    { "l2merrsr", MISCREG_L2MERRSR },

    // AArch64 registers (Op0=2)
    { "mdccint_el1", MISCREG_MDCCINT_EL1 },
    { "osdtrrx_el1", MISCREG_OSDTRRX_EL1 },
    { "mdscr_el1", MISCREG_MDSCR_EL1 },
    { "osdtrtx_el1", MISCREG_OSDTRTX_EL1 },
    { "oseccr_el1", MISCREG_OSECCR_EL1 },
    { "dbgbvr0_el1", MISCREG_DBGBVR0_EL1 },
    { "dbgbvr1_el1", MISCREG_DBGBVR1_EL1 },
    { "dbgbvr2_el1", MISCREG_DBGBVR2_EL1 },
    { "dbgbvr3_el1", MISCREG_DBGBVR3_EL1 },
    { "dbgbvr4_el1", MISCREG_DBGBVR4_EL1 },
    { "dbgbvr5_el1", MISCREG_DBGBVR5_EL1 },
    { "dbgbcr0_el1", MISCREG_DBGBCR0_EL1 },
    { "dbgbcr1_el1", MISCREG_DBGBCR1_EL1 },
    { "dbgbcr2_el1", MISCREG_DBGBCR2_EL1 },
    { "dbgbcr3_el1", MISCREG_DBGBCR3_EL1 },
    { "dbgbcr4_el1", MISCREG_DBGBCR4_EL1 },
    { "dbgbcr5_el1", MISCREG_DBGBCR5_EL1 },
    { "dbgwvr0_el1", MISCREG_DBGWVR0_EL1 },
    { "dbgwvr1_el1", MISCREG_DBGWVR1_EL1 },
    { "dbgwvr2_el1", MISCREG_DBGWVR2_EL1 },
    { "dbgwvr3_el1", MISCREG_DBGWVR3_EL1 },
    { "dbgwcr0_el1", MISCREG_DBGWCR0_EL1 },
    { "dbgwcr1_el1", MISCREG_DBGWCR1_EL1 },
    { "dbgwcr2_el1", MISCREG_DBGWCR2_EL1 },
    { "dbgwcr3_el1", MISCREG_DBGWCR3_EL1 },
    { "mdccsr_el0", MISCREG_MDCCSR_EL0 },
    { "mddtr_el0", MISCREG_MDDTR_EL0 },
    { "mddtrtx_el0", MISCREG_MDDTRTX_EL0 },
    { "mddtrrx_el0", MISCREG_MDDTRRX_EL0 },
    { "dbgvcr32_el2", MISCREG_DBGVCR32_EL2 },
    { "mdrar_el1", MISCREG_MDRAR_EL1 },
    { "oslar_el1", MISCREG_OSLAR_EL1 },
    { "oslsr_el1", MISCREG_OSLSR_EL1 },
    { "osdlr_el1", MISCREG_OSDLR_EL1 },
    { "dbgprcr_el1", MISCREG_DBGPRCR_EL1 },
    { "dbgclaimset_el1", MISCREG_DBGCLAIMSET_EL1 },
    { "dbgclaimclr_el1", MISCREG_DBGCLAIMCLR_EL1 },
    { "dbgauthstatus_el1", MISCREG_DBGAUTHSTATUS_EL1 },
    { "teecr32_el1", MISCREG_TEECR32_EL1 },
    { "teehbr32_el1", MISCREG_TEEHBR32_EL1 },

    // AArch64 registers (Op0=1,3)
    { "midr_el1", MISCREG_MIDR_EL1 },
    { "mpidr_el1", MISCREG_MPIDR_EL1 },
    { "revidr_el1", MISCREG_REVIDR_EL1 },
    { "id_pfr0_el1", MISCREG_ID_PFR0_EL1 },
    { "id_pfr1_el1", MISCREG_ID_PFR1_EL1 },
    { "id_dfr0_el1", MISCREG_ID_DFR0_EL1 },
    { "id_afr0_el1", MISCREG_ID_AFR0_EL1 },
    { "id_mmfr0_el1", MISCREG_ID_MMFR0_EL1 },
    { "id_mmfr1_el1", MISCREG_ID_MMFR1_EL1 },
    { "id_mmfr2_el1", MISCREG_ID_MMFR2_EL1 },
    { "id_mmfr3_el1", MISCREG_ID_MMFR3_EL1 },
    { "id_isar0_el1", MISCREG_ID_ISAR0_EL1 },
    { "id_isar1_el1", MISCREG_ID_ISAR1_EL1 },
    { "id_isar2_el1", MISCREG_ID_ISAR2_EL1 },
    { "id_isar3_el1", MISCREG_ID_ISAR3_EL1 },
    { "id_isar4_el1", MISCREG_ID_ISAR4_EL1 },
    { "id_isar5_el1", MISCREG_ID_ISAR5_EL1 },
    { "mvfr0_el1", MISCREG_MVFR0_EL1 },
    { "mvfr1_el1", MISCREG_MVFR1_EL1 },
    { "mvfr2_el1", MISCREG_MVFR2_EL1 },
    { "id_aa64pfr0_el1", MISCREG_ID_AA64PFR0_EL1 },
    { "id_aa64pfr1_el1", MISCREG_ID_AA64PFR1_EL1 },
    { "id_aa64dfr0_el1", MISCREG_ID_AA64DFR0_EL1 },
    { "id_aa64dfr1_el1", MISCREG_ID_AA64DFR1_EL1 },
    { "id_aa64afr0_el1", MISCREG_ID_AA64AFR0_EL1 },
    { "id_aa64afr1_el1", MISCREG_ID_AA64AFR1_EL1 },
    { "id_aa64isar0_el1", MISCREG_ID_AA64ISAR0_EL1 },
    { "id_aa64isar1_el1", MISCREG_ID_AA64ISAR1_EL1 },
    { "id_aa64mmfr0_el1", MISCREG_ID_AA64MMFR0_EL1 },
    { "id_aa64mmfr1_el1", MISCREG_ID_AA64MMFR1_EL1 },
    { "ccsidr_el1", MISCREG_CCSIDR_EL1 },
    { "clidr_el1", MISCREG_CLIDR_EL1 },
    { "aidr_el1", MISCREG_AIDR_EL1 },
    { "csselr_el1", MISCREG_CSSELR_EL1 },
    { "ctr_el0", MISCREG_CTR_EL0 },
    { "dczid_el0", MISCREG_DCZID_EL0 },
    { "vpidr_el2", MISCREG_VPIDR_EL2 },
    { "vmpidr_el2", MISCREG_VMPIDR_EL2 },
    { "sctlr_el1", MISCREG_SCTLR_EL1 },
    { "actlr_el1", MISCREG_ACTLR_EL1 },
    { "cpacr_el1", MISCREG_CPACR_EL1 },
    { "sctlr_el2", MISCREG_SCTLR_EL2 },
    { "actlr_el2", MISCREG_ACTLR_EL2 },
    { "hcr_el2", MISCREG_HCR_EL2 },
    { "mdcr_el2", MISCREG_MDCR_EL2 },
    { "cptr_el2", MISCREG_CPTR_EL2 },
    { "hstr_el2", MISCREG_HSTR_EL2 },
    { "hacr_el2", MISCREG_HACR_EL2 },
    { "sctlr_el3", MISCREG_SCTLR_EL3 },
    { "actlr_el3", MISCREG_ACTLR_EL3 },
    { "scr_el3", MISCREG_SCR_EL3 },
    { "sder32_el3", MISCREG_SDER32_EL3 },
    { "cptr_el3", MISCREG_CPTR_EL3 },
    { "mdcr_el3", MISCREG_MDCR_EL3 },
    { "ttbr0_el1", MISCREG_TTBR0_EL1 },
    { "ttbr1_el1", MISCREG_TTBR1_EL1 },
    { "tcr_el1", MISCREG_TCR_EL1 },
    { "ttbr0_el2", MISCREG_TTBR0_EL2 },
    { "tcr_el2", MISCREG_TCR_EL2 },
    { "vttbr_el2", MISCREG_VTTBR_EL2 },
    { "vtcr_el2", MISCREG_VTCR_EL2 },
    { "ttbr0_el3", MISCREG_TTBR0_EL3 },
    { "tcr_el3", MISCREG_TCR_EL3 },
    { "dacr32_el2", MISCREG_DACR32_EL2 },
    { "spsr_el1", MISCREG_SPSR_EL1 },
    { "elr_el1", MISCREG_ELR_EL1 },
    { "sp_el0", MISCREG_SP_EL0 },
    { "spsel", MISCREG_SPSEL },
    { "currentel", MISCREG_CURRENTEL },
    { "nzcv", MISCREG_NZCV },
    { "daif", MISCREG_DAIF },
    { "fpcr", MISCREG_FPCR },
    { "fpsr", MISCREG_FPSR },
    { "dspsr_el0", MISCREG_DSPSR_EL0 },
    { "dlr_el0", MISCREG_DLR_EL0 },
    { "spsr_el2", MISCREG_SPSR_EL2 },
    { "elr_el2", MISCREG_ELR_EL2 },
    { "sp_el1", MISCREG_SP_EL1 },
    { "spsr_irq", MISCREG_SPSR_IRQ_AA64 },
    { "spsr_abt", MISCREG_SPSR_ABT_AA64 },
    { "spsr_und", MISCREG_SPSR_UND_AA64 },
    { "spsr_fiq", MISCREG_SPSR_FIQ_AA64 },
    { "spsr_el3", MISCREG_SPSR_EL3 },
    { "elr_el3", MISCREG_ELR_EL3 },
    { "sp_el2", MISCREG_SP_EL2 },
    { "afsr0_el1", MISCREG_AFSR0_EL1 },
    { "afsr1_el1", MISCREG_AFSR1_EL1 },
    { "esr_el1", MISCREG_ESR_EL1 },
    { "ifsr32_el2", MISCREG_IFSR32_EL2 },
    { "afsr0_el2", MISCREG_AFSR0_EL2 },
    { "afsr1_el2", MISCREG_AFSR1_EL2 },
    { "esr_el2", MISCREG_ESR_EL2 },
    { "fpexc32_el2", MISCREG_FPEXC32_EL2 },
    { "afsr0_el3", MISCREG_AFSR0_EL3 },
    { "afsr1_el3", MISCREG_AFSR1_EL3 },
    { "esr_el3", MISCREG_ESR_EL3 },
    { "far_el1", MISCREG_FAR_EL1 },
    { "far_el2", MISCREG_FAR_EL2 },
    { "hpfar_el2", MISCREG_HPFAR_EL2 },
    { "far_el3", MISCREG_FAR_EL3 },
    { "ic_ialluis", MISCREG_IC_IALLUIS },
    { "par_el1", MISCREG_PAR_EL1 },
    { "ic_iallu", MISCREG_IC_IALLU },
    { "dc_ivac_xt", MISCREG_DC_IVAC_Xt },
    { "dc_isw_xt", MISCREG_DC_ISW_Xt },
    { "at_s1e1r_xt", MISCREG_AT_S1E1R_Xt },
    { "at_s1e1w_xt", MISCREG_AT_S1E1W_Xt },
    { "at_s1e0r_xt", MISCREG_AT_S1E0R_Xt },
    { "at_s1e0w_xt", MISCREG_AT_S1E0W_Xt },
    { "dc_csw_xt", MISCREG_DC_CSW_Xt },
    { "dc_cisw_xt", MISCREG_DC_CISW_Xt },
    { "dc_zva_xt", MISCREG_DC_ZVA_Xt },
    { "ic_ivau_xt", MISCREG_IC_IVAU_Xt },
    { "dc_cvac_xt", MISCREG_DC_CVAC_Xt },
    { "dc_cvau_xt", MISCREG_DC_CVAU_Xt },
    { "dc_civac_xt", MISCREG_DC_CIVAC_Xt },
    { "at_s1e2r_xt", MISCREG_AT_S1E2R_Xt },
    { "at_s1e2w_xt", MISCREG_AT_S1E2W_Xt },
    { "at_s12e1r_xt", MISCREG_AT_S12E1R_Xt },
    { "at_s12e1w_xt", MISCREG_AT_S12E1W_Xt },
    { "at_s12e0r_xt", MISCREG_AT_S12E0R_Xt },
    { "at_s12e0w_xt", MISCREG_AT_S12E0W_Xt },
    { "at_s1e3r_xt", MISCREG_AT_S1E3R_Xt },
    { "at_s1e3w_xt", MISCREG_AT_S1E3W_Xt },
    { "tlbi_vmalle1is", MISCREG_TLBI_VMALLE1IS },
    { "tlbi_vae1is_xt", MISCREG_TLBI_VAE1IS_Xt },
    { "tlbi_aside1is_xt", MISCREG_TLBI_ASIDE1IS_Xt },
    { "tlbi_vaae1is_xt", MISCREG_TLBI_VAAE1IS_Xt },
    { "tlbi_vale1is_xt", MISCREG_TLBI_VALE1IS_Xt },
    { "tlbi_vaale1is_xt", MISCREG_TLBI_VAALE1IS_Xt },
    { "tlbi_vmalle1", MISCREG_TLBI_VMALLE1 },
    { "tlbi_vae1_xt", MISCREG_TLBI_VAE1_Xt },
    { "tlbi_aside1_xt", MISCREG_TLBI_ASIDE1_Xt },
    { "tlbi_vaae1_xt", MISCREG_TLBI_VAAE1_Xt },
    { "tlbi_vale1_xt", MISCREG_TLBI_VALE1_Xt },
    { "tlbi_vaale1_xt", MISCREG_TLBI_VAALE1_Xt },
    { "tlbi_ipas2e1is_xt", MISCREG_TLBI_IPAS2E1IS_Xt },
    { "tlbi_ipas2le1is_xt", MISCREG_TLBI_IPAS2LE1IS_Xt },
    { "tlbi_alle2is", MISCREG_TLBI_ALLE2IS },
    { "tlbi_vae2is_xt", MISCREG_TLBI_VAE2IS_Xt },
    { "tlbi_alle1is", MISCREG_TLBI_ALLE1IS },
    { "tlbi_vale2is_xt", MISCREG_TLBI_VALE2IS_Xt },
    { "tlbi_vmalls12e1is", MISCREG_TLBI_VMALLS12E1IS },
    { "tlbi_ipas2e1_xt", MISCREG_TLBI_IPAS2E1_Xt },
    { "tlbi_ipas2le1_xt", MISCREG_TLBI_IPAS2LE1_Xt },
    { "tlbi_alle2", MISCREG_TLBI_ALLE2 },
    { "tlbi_vae2_xt", MISCREG_TLBI_VAE2_Xt },
    { "tlbi_alle1", MISCREG_TLBI_ALLE1 },
    { "tlbi_vale2_xt", MISCREG_TLBI_VALE2_Xt },
    { "tlbi_vmalls12e1", MISCREG_TLBI_VMALLS12E1 },
    { "tlbi_alle3is", MISCREG_TLBI_ALLE3IS },
    { "tlbi_vae3is_xt", MISCREG_TLBI_VAE3IS_Xt },
    { "tlbi_vale3is_xt", MISCREG_TLBI_VALE3IS_Xt },
    { "tlbi_alle3", MISCREG_TLBI_ALLE3 },
    { "tlbi_vae3_xt", MISCREG_TLBI_VAE3_Xt },
    { "tlbi_vale3_xt", MISCREG_TLBI_VALE3_Xt },
    { "pmintenset_el1", MISCREG_PMINTENSET_EL1 },
    { "pmintenclr_el1", MISCREG_PMINTENCLR_EL1 },
    { "pmcr_el0", MISCREG_PMCR_EL0 },
    { "pmcntenset_el0", MISCREG_PMCNTENSET_EL0 },
    { "pmcntenclr_el0", MISCREG_PMCNTENCLR_EL0 },
    { "pmovsclr_el0", MISCREG_PMOVSCLR_EL0 },
    { "pmswinc_el0", MISCREG_PMSWINC_EL0 },
    { "pmselr_el0", MISCREG_PMSELR_EL0 },
    { "pmceid0_el0", MISCREG_PMCEID0_EL0 },
    { "pmceid1_el0", MISCREG_PMCEID1_EL0 },
    { "pmccntr_el0", MISCREG_PMCCNTR_EL0 },
    { "pmxevtyper_el0", MISCREG_PMXEVTYPER_EL0 },
    { "pmccfiltr_el0", MISCREG_PMCCFILTR_EL0 },
    { "pmxevcntr_el0", MISCREG_PMXEVCNTR_EL0 },
    { "pmuserenr_el0", MISCREG_PMUSERENR_EL0 },
    { "pmovsset_el0", MISCREG_PMOVSSET_EL0 },
    { "mair_el1", MISCREG_MAIR_EL1 },
    { "amair_el1", MISCREG_AMAIR_EL1 },
    { "mair_el2", MISCREG_MAIR_EL2 },
    { "amair_el2", MISCREG_AMAIR_EL2 },
    { "mair_el3", MISCREG_MAIR_EL3 },
    { "amair_el3", MISCREG_AMAIR_EL3 },
    { "l2ctlr_el1", MISCREG_L2CTLR_EL1 },
    { "l2ectlr_el1", MISCREG_L2ECTLR_EL1 },
    { "vbar_el1", MISCREG_VBAR_EL1 },
    { "rvbar_el1", MISCREG_RVBAR_EL1 },
    { "isr_el1", MISCREG_ISR_EL1 },
    { "vbar_el2", MISCREG_VBAR_EL2 },
    { "rvbar_el2", MISCREG_RVBAR_EL2 },
    { "vbar_el3", MISCREG_VBAR_EL3 },
    { "rvbar_el3", MISCREG_RVBAR_EL3 },
    { "rmr_el3", MISCREG_RMR_EL3 },
    { "contextidr_el1", MISCREG_CONTEXTIDR_EL1 },
    { "contextidr_el2", MISCREG_CONTEXTIDR_EL2 },
    { "tpidr_el1", MISCREG_TPIDR_EL1 },
    { "tpidr_el0", MISCREG_TPIDR_EL0 },
    { "tpidrro_el0", MISCREG_TPIDRRO_EL0 },
    { "tpidr_el2", MISCREG_TPIDR_EL2 },
    { "tpidr_el3", MISCREG_TPIDR_EL3 },
    { "cntkctl_el1", MISCREG_CNTKCTL_EL1 },
    { "cntfrq_el0", MISCREG_CNTFRQ_EL0 },
    { "cntpct_el0", MISCREG_CNTPCT_EL0 },
    { "cntvct_el0", MISCREG_CNTVCT_EL0 },
    { "cntp_tval_el0", MISCREG_CNTP_TVAL_EL0 },
    { "cntp_ctl_el0", MISCREG_CNTP_CTL_EL0 },
    { "cntp_cval_el0", MISCREG_CNTP_CVAL_EL0 },
    { "cntv_tval_el0", MISCREG_CNTV_TVAL_EL0 },
    { "cntv_ctl_el0", MISCREG_CNTV_CTL_EL0 },
    { "cntv_cval_el0", MISCREG_CNTV_CVAL_EL0 },
    { "pmevcntr0_el0", MISCREG_PMEVCNTR0_EL0 },
    { "pmevcntr1_el0", MISCREG_PMEVCNTR1_EL0 },
    { "pmevcntr2_el0", MISCREG_PMEVCNTR2_EL0 },
    { "pmevcntr3_el0", MISCREG_PMEVCNTR3_EL0 },
    { "pmevcntr4_el0", MISCREG_PMEVCNTR4_EL0 },
    { "pmevcntr5_el0", MISCREG_PMEVCNTR5_EL0 },
    { "pmevtyper0_el0", MISCREG_PMEVTYPER0_EL0 },
    { "pmevtyper1_el0", MISCREG_PMEVTYPER1_EL0 },
    { "pmevtyper2_el0", MISCREG_PMEVTYPER2_EL0 },
    { "pmevtyper3_el0", MISCREG_PMEVTYPER3_EL0 },
    { "pmevtyper4_el0", MISCREG_PMEVTYPER4_EL0 },
    { "pmevtyper5_el0", MISCREG_PMEVTYPER5_EL0 },
    { "cntvoff_el2", MISCREG_CNTVOFF_EL2 },
    { "cnthctl_el2", MISCREG_CNTHCTL_EL2 },
    { "cnthp_tval_el2", MISCREG_CNTHP_TVAL_EL2 },
    { "cnthp_ctl_el2", MISCREG_CNTHP_CTL_EL2 },
    { "cnthp_cval_el2", MISCREG_CNTHP_CVAL_EL2 },
    { "cntps_tval_el1", MISCREG_CNTPS_TVAL_EL1 },
    { "cntps_ctl_el1", MISCREG_CNTPS_CTL_EL1 },
    { "cntps_cval_el1", MISCREG_CNTPS_CVAL_EL1 },
    { "il1data0_el1", MISCREG_IL1DATA0_EL1 },
    { "il1data1_el1", MISCREG_IL1DATA1_EL1 },
    { "il1data2_el1", MISCREG_IL1DATA2_EL1 },
    { "il1data3_el1", MISCREG_IL1DATA3_EL1 },
    { "dl1data0_el1", MISCREG_DL1DATA0_EL1 },
    { "dl1data1_el1", MISCREG_DL1DATA1_EL1 },
    { "dl1data2_el1", MISCREG_DL1DATA2_EL1 },
    { "dl1data3_el1", MISCREG_DL1DATA3_EL1 },
    { "dl1data4_el1", MISCREG_DL1DATA4_EL1 },
    { "l2actlr_el1", MISCREG_L2ACTLR_EL1 },
    { "cpuactlr_el1", MISCREG_CPUACTLR_EL1 },
    { "cpuectlr_el1", MISCREG_CPUECTLR_EL1 },
    { "cpumerrsr_el1", MISCREG_CPUMERRSR_EL1 },
    { "l2merrsr_el1", MISCREG_L2MERRSR_EL1 },
    { "cbar_el1", MISCREG_CBAR_EL1 },
};

void
TarmacParserRecord::TarmacParserRecordEvent::process()
{
    ostream &outs = Trace::output();

    list<ParserRegEntry>::iterator it = destRegRecords.begin(),
                                   end = destRegRecords.end();

    uint64_t value_hi, value_lo;
    bool check_value_hi = false;

    for (; it != end; ++it) {
        switch (it->type) {
          case REG_R:
          case REG_X:
            value_lo = thread->readIntReg(it->index);
            break;
          case REG_S:
            if (instRecord.isetstate == ISET_A64)
                value_lo = thread->readFloatRegBits(it->index * 4);
            else
                value_lo = thread->readFloatRegBits(it->index);
            break;
          case REG_D:
            if (instRecord.isetstate == ISET_A64)
                value_lo = thread->readFloatRegBits(it->index * 4) |
                    (uint64_t) thread->readFloatRegBits(it->index * 4 + 1) <<
                    32;
            else
                value_lo = thread->readFloatRegBits(it->index * 2) |
                    (uint64_t) thread->readFloatRegBits(it->index * 2 + 1) <<
                    32;
            break;
          case REG_Q:
            check_value_hi = true;
            if (instRecord.isetstate == ISET_A64) {
                value_lo = thread->readFloatRegBits(it->index * 4) |
                    (uint64_t) thread->readFloatRegBits(it->index * 4 + 1) <<
                    32;
                value_hi = thread->readFloatRegBits(it->index * 4 + 2) |
                    (uint64_t) thread->readFloatRegBits(it->index * 4 + 3) <<
                    32;
            } else {
                value_lo = thread->readFloatRegBits(it->index * 2) |
                    (uint64_t) thread->readFloatRegBits(it->index * 2 + 1) <<
                    32;
                value_hi = thread->readFloatRegBits(it->index * 2 + 2) |
                    (uint64_t) thread->readFloatRegBits(it->index * 2 + 3) <<
                    32;
            }
            break;
          case REG_MISC:
            if (it->index == MISCREG_CPSR) {
                // Read condition codes from aliased integer regs
                CPSR cpsr = thread->readMiscRegNoEffect(it->index);
                cpsr.nz = thread->readCCReg(CCREG_NZ);
                cpsr.c = thread->readCCReg(CCREG_C);
                cpsr.v = thread->readCCReg(CCREG_V);
                cpsr.ge = thread->readCCReg(CCREG_GE);
                value_lo = cpsr;
            } else if (it->index == MISCREG_NZCV) {
                CPSR cpsr = 0;
                cpsr.nz = thread->readCCReg(CCREG_NZ);
                cpsr.c = thread->readCCReg(CCREG_C);
                cpsr.v = thread->readCCReg(CCREG_V);
                value_lo = cpsr;
            } else {
                value_lo = thread->readMiscRegNoEffect(it->index);
            }
            break;
          default:
            panic("Unknown TARMAC trace record type!");
        }

        if (value_lo != it->valueLo ||
            (check_value_hi && (value_hi != it->valueHi))) {
            if (!mismatch)
                TarmacParserRecord::printMismatchHeader(inst, pc);
            outs << "diff> [" << it->repr << "] gem5: 0x";
            if (check_value_hi)
                outs << hex << setfill('0') << setw(16) << value_hi
                     << setfill('0') << setw(16) << value_lo;
            else
                outs << hex << value_lo;
            outs << ", TARMAC: 0x";
            if (check_value_hi)
                outs << hex << setfill('0') << setw(16) << it->valueHi
                     << setfill('0') << setw(16) << it->valueLo << endl;
            else
                outs << it->valueLo << endl;
            mismatch = true;
        }

        check_value_hi = false;
    }
    destRegRecords.clear();

    if (mismatchOnPcOrOpcode && (parent.exitOnDiff ||
                                 parent.exitOnInsnDiff))
        exitSimLoop("a mismatch with the TARMAC trace has been detected "
                    "on PC or opcode", 1);
    if (mismatch && parent.exitOnDiff)
        exitSimLoop("a mismatch with the TARMAC trace has been detected "
                    "on data value", 1);
}

const char *
TarmacParserRecord::TarmacParserRecordEvent::description() const
{
    return "TARMAC parser record event";
}


void
TarmacParserRecord::printMismatchHeader(const StaticInstPtr staticInst,
                                        TheISA::PCState pc)
{
    ostream &outs = Trace::output();
    outs << "\nMismatch between gem5 and TARMAC trace @ " << dec << curTick()
         << " ticks\n"
         << "[seq_num: " << dec << instRecord.seq_num
         << ", opcode: 0x" << hex << (staticInst->machInst & 0xffffffff)
         << ", PC: 0x" << pc.pc()
         << ", disasm: " <<  staticInst->disassemble(pc.pc()) << "]"
         << endl;
}

TarmacParserRecord::TarmacParserRecord(Tick _when, ThreadContext *_thread,
                                       const StaticInstPtr _staticInst,
                                       PCState _pc,
                                       TarmacParser& _parent,
                                       const StaticInstPtr _macroStaticInst)
    : TarmacBaseRecord(_when, _thread, _staticInst,
                       _pc, _macroStaticInst),
      parsingStarted(false), mismatch(false),
      mismatchOnPcOrOpcode(false), parent(_parent)
{
    memReq = std::make_shared<Request>();
}

void
TarmacParserRecord::dump()
{
    ostream &outs = Trace::output();

    // By default TARMAC splits memory accesses into 4-byte chunks (see
    // 'loadstore-display-width' option in TARMAC plugin)
    uint32_t written_data = 0;
    unsigned mem_flags = TheISA::TLB::MustBeOne | 3 |
        TheISA::TLB::AllowUnaligned;

    ISetState isetstate;

    if (!staticInst->isMicroop() || staticInst->isLastMicroop()) {

        if (parent.macroopInProgress && !staticInst->isLastMicroop()) {
            // A microop faulted and it was not the last microop -> advance
            // TARMAC trace to next instruction
            advanceTrace();
        }

        parent.macroopInProgress = false;

        auto arm_inst = static_cast<const ArmStaticInst*>(
            staticInst.get()
        );

        while (advanceTrace()) {
            switch (currRecordType) {

              case TARMAC_INST:
                parsingStarted = true;
                if (pc.instAddr() != instRecord.addr) {
                    if (!mismatch)
                        printMismatchHeader(staticInst, pc);
                    outs << "diff> [PC] gem5: 0x" << hex << pc.instAddr()
                         << ", TARMAC: 0x" << instRecord.addr << endl;
                    mismatch = true;
                    mismatchOnPcOrOpcode = true;
                }

                if (arm_inst->encoding() != instRecord.opcode) {
                    if (!mismatch)
                        printMismatchHeader(staticInst, pc);
                    outs << "diff> [opcode] gem5: 0x" << hex
                         << arm_inst->encoding()
                         << ", TARMAC: 0x" << instRecord.opcode << endl;
                    mismatch = true;
                    mismatchOnPcOrOpcode = true;
                }

                // Set the Instruction set state.
                isetstate = pcToISetState(pc);

                if (instRecord.isetstate != isetstate &&
                    isetstate != ISET_UNSUPPORTED) {
                    if (!mismatch)
                        printMismatchHeader(staticInst, pc);
                    outs << "diff> [iset_state] gem5: "
                         << iSetStateToStr(isetstate)
                         << ", TARMAC: "
                         << iSetStateToStr(instRecord.isetstate);
                    mismatch = true;
                }

                // TODO(Giacomo): add support for predicate and mode checking
                break;

              case TARMAC_REG:
                destRegRecords.push_back(regRecord);
                break;

              case TARMAC_MEM:
                if (!readMemNoEffect(memRecord.addr, (uint8_t*) &written_data,
                                     memRecord.size, mem_flags))
                    break;
                if (written_data != memRecord.data) {
                    if (!mismatch)
                        printMismatchHeader(staticInst, pc);
                    outs << "diff> [mem(0x" << hex << memRecord.addr
                         << ")] gem5: 0x" << written_data
                         << ", TARMAC: 0x" << memRecord.data
                         << endl;
                }
                break;

              case TARMAC_UNSUPPORTED:
                break;

              default:
                panic("Unknown TARMAC trace record type!");
            }
        }
        // We are done with the current instruction, i.e. all the corresponding
        // entries in the TARMAC trace have been parsed
        if (destRegRecords.size()) {
            TarmacParserRecordEvent *event = new TarmacParserRecordEvent(
                parent, thread, staticInst, pc, mismatch,
                mismatchOnPcOrOpcode);
            mainEventQueue[0]->schedule(event, curTick());
        } else if (mismatchOnPcOrOpcode && (parent.exitOnDiff ||
                                            parent.exitOnInsnDiff)) {
            exitSimLoop("a mismatch with the TARMAC trace has been detected "
                        "on PC or opcode", 1);
        }
    } else {
        parent.macroopInProgress = true;
    }
}

bool
TarmacParserRecord::advanceTrace()
{
    ifstream& trace = parent.trace;
    trace >> hex;  // All integer values are in hex base

    if (buf[0] != 'I') {
        trace >> buf;
        if (trace.eof())
            return false;
        trace >> buf >> buf;
        if (parent.cpuId) {
            assert((buf[0] == 'c') && (buf[1] == 'p') && (buf[2] == 'u'));
            trace >> buf;
        }
    }

    if (trace.eof())
        return false;

    if (buf[0] == 'I') {
        // Instruction trace record
        if (parsingStarted)
            return false;
        currRecordType = TARMAC_INST;
        instRecord.taken = (buf[1] == 'T');
        trace >> buf;
        instRecord.seq_num = atoi(&buf[1]);
        trace >> instRecord.addr;
        char c = trace.peek();
        if (c == ':') {
            // Skip phys. address and _S/_NS suffix
            trace >> c >> buf;
        }
        trace >> instRecord.opcode;
        trace >> buf;
        switch (buf[0]) {
          case 'A':
            instRecord.isetstate = ISET_ARM;
            break;
          case 'T':
            instRecord.isetstate = ISET_THUMB;
            break;
          case 'O':
            instRecord.isetstate = ISET_A64;
            break;
          default:
            warn("Invalid TARMAC trace record (seq_num: %lld)",
                 instRecord.seq_num);
            instRecord.isetstate = ISET_UNSUPPORTED;
            currRecordType = TARMAC_UNSUPPORTED;
            break;
        }
        trace.ignore(MaxLineLength, '\n');
        buf[0] = 0;
    } else if (buf[0] == 'R') {
        // Register trace record
        currRecordType = TARMAC_REG;
        trace >> buf;
        strcpy(regRecord.repr, buf);
        if (buf[0] == 'r' && isdigit(buf[1])) {
            // R register
            regRecord.type = REG_R;
            int base_index = atoi(&buf[1]);
            char* pch = strchr(buf, '_');
            if (pch == NULL) {
                regRecord.index = INTREG_USR(base_index);
            } else {
                ++pch;
                if (strncmp(pch, "usr", 3) == 0)
                    regRecord.index = INTREG_USR(base_index);
                else if (strncmp(pch, "fiq", 3) == 0)
                    regRecord.index = INTREG_FIQ(base_index);
                else if (strncmp(pch, "irq", 3) == 0)
                    regRecord.index = INTREG_IRQ(base_index);
                else if (strncmp(pch, "svc", 3) == 0)
                    regRecord.index = INTREG_SVC(base_index);
                else if (strncmp(pch, "mon", 3) == 0)
                    regRecord.index = INTREG_MON(base_index);
                else if (strncmp(pch, "abt", 3) == 0)
                    regRecord.index = INTREG_ABT(base_index);
                else if (strncmp(pch, "und", 3) == 0)
                    regRecord.index = INTREG_UND(base_index);
                else if (strncmp(pch, "hyp", 3) == 0)
                    regRecord.index = INTREG_HYP(base_index);
            }
        // A64 register names are capitalized in AEM TARMAC, unlike A32
        } else if (buf[0] == 'X' && isdigit(buf[1])) {
            // X register (A64)
            regRecord.type = REG_X;
            regRecord.index = atoi(&buf[1]);
        } else if (buf[0] == 's' && isdigit(buf[1])) {
            // S register
            regRecord.type = REG_S;
            regRecord.index = atoi(&buf[1]);
        } else if (buf[0] == 'd' && isdigit(buf[1])) {
            // D register
            regRecord.type = REG_D;
            regRecord.index = atoi(&buf[1]);
        } else if (buf[0] == 'q' && isdigit(buf[1])) {
            // Q register
            regRecord.type = REG_Q;
            regRecord.index = atoi(&buf[1]);
        } else if (strncmp(buf, "SP_EL", 5) == 0) {
            // A64 stack pointer
            regRecord.type = REG_X;
            regRecord.index = INTREG_SP0 + atoi(&buf[5]);
        } else if (miscRegMap.count(buf)) {
            // Misc. register
            regRecord.type = REG_MISC;
            regRecord.index = miscRegMap[buf];
        } else {
            // Try match with upper case name (misc. register)
            string reg_name = buf;
            transform(reg_name.begin(), reg_name.end(), reg_name.begin(),
                      ::tolower);
            if (miscRegMap.count(reg_name.c_str())) {
                regRecord.type = REG_MISC;
                regRecord.index = miscRegMap[reg_name.c_str()];
            } else {
                warn("Unknown register in TARMAC trace (%s).\n", buf);
                currRecordType = TARMAC_UNSUPPORTED;
                trace.ignore(MaxLineLength, '\n');
                buf[0] = 0;
                return true;
            }
        }
        if (regRecord.type == REG_Q) {
            trace.ignore();
            trace.get(buf, 17);
            // buf[16] = '\0';
            regRecord.valueHi = strtoull(buf, NULL, 16);
            trace.get(buf, 17);
            // buf[16] = '\0';
            regRecord.valueLo = strtoull(buf, NULL, 16);
        } else {
            trace >> regRecord.valueLo;
            char c = trace.peek();
            if (c == ':') {
                // 64-bit value with colon in the middle
                uint64_t lsw = 0;
                trace >> c >> lsw;
                regRecord.valueLo = (regRecord.valueLo << 32) | lsw;
            }
        }
        trace.ignore(MaxLineLength, '\n');
        buf[0] = 0;
    } else if (buf[0] == 'M' && (parent.memWrCheck && buf[1] == 'W')) {
        currRecordType = TARMAC_MEM;
        memRecord.size = atoi(&buf[2]);
        trace >> memRecord.addr;
        char c = trace.peek();
        if (c == ':') {
            // Skip phys. address and _S/_NS suffix
            trace >> c >> buf;
        }
        trace >> memRecord.data;
        trace.ignore(MaxLineLength, '\n');
        buf[0] = 0;
    } else {
        currRecordType = TARMAC_UNSUPPORTED;
        trace.ignore(MaxLineLength, '\n');
        buf[0] = 0;
    }

    return true;
}

bool
TarmacParserRecord::readMemNoEffect(Addr addr, uint8_t *data, unsigned size,
                                    unsigned flags)
{
    const RequestPtr &req = memReq;
    TheISA::TLB* dtb = static_cast<TLB*>(thread->getDTBPtr());

    req->setVirt(0, addr, size, flags, thread->pcState().instAddr(),
                 Request::funcMasterId);

    // Translate to physical address
    Fault fault = dtb->translateAtomic(req, thread, BaseTLB::Read);

    // Ignore read if the address falls into the ignored range
    if (parent.ignoredAddrRange.contains(addr))
        return false;

    // Now do the access
    if (fault == NoFault &&
        !req->getFlags().isSet(Request::NO_ACCESS)) {
        if (req->isLLSC() || req->isMmappedIpr())
            // LLSCs and mem. mapped IPRs are ignored
            return false;
        // the translating proxy will perform the virtual to physical
        // translation again
        thread->getVirtProxy().readBlob(addr, data, size);
    } else {
        return false;
    }

    if (fault != NoFault) {
        return false;
    }

    return true;
}

void
TarmacParser::advanceTraceToStartPc()
{
    char buf[TarmacParserRecord::MaxLineLength];
    Addr pc;
    int saved_offset;

    trace >> hex;  // All integer values are in hex base

    while (true) {
        saved_offset = trace.tellg();
        trace >> buf >> buf >> buf;
        if (cpuId)
            trace >> buf;
        if (buf[0] == 'I') {
            trace >> buf >> pc;
            if (pc == startPc) {
                // Set file pointer to the beginning of this line
                trace.seekg(saved_offset, ios::beg);
                return;
            } else {
                trace.ignore(TarmacParserRecord::MaxLineLength, '\n');
            }
        } else {
            trace.ignore(TarmacParserRecord::MaxLineLength, '\n');
        }
        if (trace.eof())
            panic("End of TARMAC trace reached before start PC\n");
    }
}

const char*
TarmacParserRecord::iSetStateToStr(ISetState isetstate) const
{
    switch (isetstate) {
      case ISET_ARM:
        return "ARM (A32)";
      case ISET_THUMB:
        return "Thumb (A32)";
      case ISET_A64:
        return "A64";
      default:
        return "UNSUPPORTED";
    }
}

} // namespace Trace

Trace::TarmacParser *
TarmacParserParams::create()
{
    return new Trace::TarmacParser(this);
}
