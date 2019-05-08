/*
 * Copyright (c) 2010, 2012-2013, 2017-2018 ARM Limited
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
 * Copyright (c) 2007-2008 The Florida State University
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
 * Authors: Stephen Hines
 */

#ifndef __ARCH_ARM_TYPES_HH__
#define __ARCH_ARM_TYPES_HH__

#include "arch/generic/types.hh"
#include "base/bitunion.hh"
#include "base/logging.hh"
#include "base/types.hh"
#include "debug/Decoder.hh"

namespace ArmISA
{
    typedef uint32_t MachInst;

    BitUnion8(ITSTATE)
        /* Note that the split (cond, mask) below is not as in ARM ARM.
         * But it is more convenient for simulation. The condition
         * is always the concatenation of the top 3 bits and the next bit,
         * which applies when one of the bottom 4 bits is set.
         * Refer to predecoder.cc for the use case.
         */
        Bitfield<7, 4> cond;
        Bitfield<3, 0> mask;
        // Bitfields for moving to/from CPSR
        Bitfield<7, 2> top6;
        Bitfield<1, 0> bottom2;
    EndBitUnion(ITSTATE)

    BitUnion64(ExtMachInst)
        // Decoder state
        Bitfield<63, 62> decoderFault; // See DecoderFault
        Bitfield<61> illegalExecution;

        // ITSTATE bits
        Bitfield<55, 48> itstate;
        Bitfield<55, 52> itstateCond;
        Bitfield<51, 48> itstateMask;

        // FPSCR fields
        Bitfield<41, 40> fpscrStride;
        Bitfield<39, 37> fpscrLen;

        // Bitfields to select mode.
        Bitfield<36>     thumb;
        Bitfield<35>     bigThumb;
        Bitfield<34>     aarch64;

        // Made up bitfields that make life easier.
        Bitfield<33>     sevenAndFour;
        Bitfield<32>     isMisc;

        uint32_t         instBits;

        // All the different types of opcode fields.
        Bitfield<27, 25> encoding;
        Bitfield<25>     useImm;
        Bitfield<24, 21> opcode;
        Bitfield<24, 20> mediaOpcode;
        Bitfield<24>     opcode24;
        Bitfield<24, 23> opcode24_23;
        Bitfield<23, 20> opcode23_20;
        Bitfield<23, 21> opcode23_21;
        Bitfield<20>     opcode20;
        Bitfield<22>     opcode22;
        Bitfield<19, 16> opcode19_16;
        Bitfield<19>     opcode19;
        Bitfield<18>     opcode18;
        Bitfield<15, 12> opcode15_12;
        Bitfield<15>     opcode15;
        Bitfield<7,  4>  miscOpcode;
        Bitfield<7,5>    opc2;
        Bitfield<7>      opcode7;
        Bitfield<6>      opcode6;
        Bitfield<4>      opcode4;

        Bitfield<31, 28> condCode;
        Bitfield<20>     sField;
        Bitfield<19, 16> rn;
        Bitfield<15, 12> rd;
        Bitfield<15, 12> rt;
        Bitfield<11, 7>  shiftSize;
        Bitfield<6,  5>  shift;
        Bitfield<3,  0>  rm;

        Bitfield<11, 8>  rs;

        SubBitUnion(puswl, 24, 20)
            Bitfield<24> prepost;
            Bitfield<23> up;
            Bitfield<22> psruser;
            Bitfield<21> writeback;
            Bitfield<20> loadOp;
        EndSubBitUnion(puswl)

        Bitfield<24, 20> pubwl;

        Bitfield<7, 0> imm;

        Bitfield<11, 8>  rotate;

        Bitfield<11, 0>  immed11_0;
        Bitfield<7,  0>  immed7_0;

        Bitfield<11, 8>  immedHi11_8;
        Bitfield<3,  0>  immedLo3_0;

        Bitfield<15, 0>  regList;

        Bitfield<23, 0>  offset;

        Bitfield<23, 0>  immed23_0;

        Bitfield<11, 8>  cpNum;
        Bitfield<18, 16> fn;
        Bitfield<14, 12> fd;
        Bitfield<3>      fpRegImm;
        Bitfield<3,  0>  fm;
        Bitfield<2,  0>  fpImm;
        Bitfield<24, 20> punwl;

        Bitfield<15,  8>  m5Func;

        // 16 bit thumb bitfields
        Bitfield<15, 13> topcode15_13;
        Bitfield<13, 11> topcode13_11;
        Bitfield<12, 11> topcode12_11;
        Bitfield<12, 10> topcode12_10;
        Bitfield<11, 9>  topcode11_9;
        Bitfield<11, 8>  topcode11_8;
        Bitfield<10, 9>  topcode10_9;
        Bitfield<10, 8>  topcode10_8;
        Bitfield<9,  6>  topcode9_6;
        Bitfield<7>      topcode7;
        Bitfield<7, 6>   topcode7_6;
        Bitfield<7, 5>   topcode7_5;
        Bitfield<7, 4>   topcode7_4;
        Bitfield<3, 0>   topcode3_0;

        // 32 bit thumb bitfields
        Bitfield<28, 27> htopcode12_11;
        Bitfield<26, 25> htopcode10_9;
        Bitfield<25>     htopcode9;
        Bitfield<25, 24> htopcode9_8;
        Bitfield<25, 21> htopcode9_5;
        Bitfield<25, 20> htopcode9_4;
        Bitfield<24>     htopcode8;
        Bitfield<24, 23> htopcode8_7;
        Bitfield<24, 22> htopcode8_6;
        Bitfield<24, 21> htopcode8_5;
        Bitfield<23>     htopcode7;
        Bitfield<23, 21> htopcode7_5;
        Bitfield<22>     htopcode6;
        Bitfield<22, 21> htopcode6_5;
        Bitfield<21, 20> htopcode5_4;
        Bitfield<20>     htopcode4;

        Bitfield<19, 16> htrn;
        Bitfield<20>     hts;

        Bitfield<15>     ltopcode15;
        Bitfield<11, 8>  ltopcode11_8;
        Bitfield<7,  6>  ltopcode7_6;
        Bitfield<7,  4>  ltopcode7_4;
        Bitfield<4>      ltopcode4;

        Bitfield<11, 8>  ltrd;
        Bitfield<11, 8>  ltcoproc;
    EndBitUnion(ExtMachInst)

    class PCState : public GenericISA::UPCState<MachInst>
    {
      protected:

        typedef GenericISA::UPCState<MachInst> Base;

        enum FlagBits {
            ThumbBit = (1 << 0),
            JazelleBit = (1 << 1),
            AArch64Bit = (1 << 2)
        };

        uint8_t flags;
        uint8_t nextFlags;
        uint8_t _itstate;
        uint8_t _nextItstate;
        uint8_t _size;
        bool _illegalExec;
      public:
        PCState() : flags(0), nextFlags(0), _itstate(0), _nextItstate(0),
                    _size(0), _illegalExec(false)
        {}

        void
        set(Addr val)
        {
            Base::set(val);
            npc(val + (thumb() ? 2 : 4));
        }

        PCState(Addr val) : flags(0), nextFlags(0), _itstate(0),
                            _nextItstate(0), _size(0), _illegalExec(false)
        { set(val); }

        bool
        illegalExec() const
        {
            return _illegalExec;
        }

        void
        illegalExec(bool val)
        {
            _illegalExec = val;
        }

        bool
        thumb() const
        {
            return flags & ThumbBit;
        }

        void
        thumb(bool val)
        {
            if (val)
                flags |= ThumbBit;
            else
                flags &= ~ThumbBit;
        }

        bool
        nextThumb() const
        {
            return nextFlags & ThumbBit;
        }

        void
        nextThumb(bool val)
        {
            if (val)
                nextFlags |= ThumbBit;
            else
                nextFlags &= ~ThumbBit;
        }

        void size(uint8_t s) { _size = s; }
        uint8_t size() const { return _size; }

        bool
        branching() const
        {
            return ((this->pc() + this->size()) != this->npc());
        }


        bool
        jazelle() const
        {
            return flags & JazelleBit;
        }

        void
        jazelle(bool val)
        {
            if (val)
                flags |= JazelleBit;
            else
                flags &= ~JazelleBit;
        }

        bool
        nextJazelle() const
        {
            return nextFlags & JazelleBit;
        }

        void
        nextJazelle(bool val)
        {
            if (val)
                nextFlags |= JazelleBit;
            else
                nextFlags &= ~JazelleBit;
        }

        bool
        aarch64() const
        {
            return flags & AArch64Bit;
        }

        void
        aarch64(bool val)
        {
            if (val)
                flags |= AArch64Bit;
            else
                flags &= ~AArch64Bit;
        }

        bool
        nextAArch64() const
        {
            return nextFlags & AArch64Bit;
        }

        void
        nextAArch64(bool val)
        {
            if (val)
                nextFlags |= AArch64Bit;
            else
                nextFlags &= ~AArch64Bit;
        }


        uint8_t
        itstate() const
        {
            return _itstate;
        }

        void
        itstate(uint8_t value)
        {
            _itstate = value;
        }

        uint8_t
        nextItstate() const
        {
            return _nextItstate;
        }

        void
        nextItstate(uint8_t value)
        {
            _nextItstate = value;
        }

        void
        advance()
        {
            Base::advance();
            flags = nextFlags;
            npc(pc() + (thumb() ? 2 : 4));

            if (_nextItstate) {
                _itstate = _nextItstate;
                _nextItstate = 0;
            } else if (_itstate) {
                ITSTATE it = _itstate;
                uint8_t cond_mask = it.mask;
                uint8_t thumb_cond = it.cond;
                DPRINTF(Decoder, "Advancing ITSTATE from %#x,%#x.\n",
                        thumb_cond, cond_mask);
                cond_mask <<= 1;
                uint8_t new_bit = bits(cond_mask, 4);
                cond_mask &= mask(4);
                if (cond_mask == 0)
                    thumb_cond = 0;
                else
                    replaceBits(thumb_cond, 0, new_bit);
                DPRINTF(Decoder, "Advancing ITSTATE to %#x,%#x.\n",
                        thumb_cond, cond_mask);
                it.mask = cond_mask;
                it.cond = thumb_cond;
                _itstate = it;
            }
        }

        void
        uEnd()
        {
            advance();
            upc(0);
            nupc(1);
        }

        Addr
        instPC() const
        {
            return pc() + (thumb() ? 4 : 8);
        }

        void
        instNPC(Addr val)
        {
            // @todo: review this when AArch32/64 interprocessing is
            // supported
            if (aarch64())
                npc(val);  // AArch64 doesn't force PC alignment, a PC
                           // Alignment Fault can be raised instead
            else
                npc(val &~ mask(nextThumb() ? 1 : 2));
        }

        Addr
        instNPC() const
        {
            return npc();
        }

        // Perform an interworking branch.
        void
        instIWNPC(Addr val)
        {
            bool thumbEE = (thumb() && jazelle());

            Addr newPC = val;
            if (thumbEE) {
                if (bits(newPC, 0)) {
                    newPC = newPC & ~mask(1);
                }  // else we have a bad interworking address; do not call
                   // panic() since the instruction could be executed
                   // speculatively
            } else {
                if (bits(newPC, 0)) {
                    nextThumb(true);
                    newPC = newPC & ~mask(1);
                } else if (!bits(newPC, 1)) {
                    nextThumb(false);
                } else {
                    // This state is UNPREDICTABLE in the ARM architecture
                    // The easy thing to do is just mask off the bit and
                    // stay in the current mode, so we'll do that.
                    newPC &= ~mask(2);
                }
            }
            npc(newPC);
        }

        // Perform an interworking branch in ARM mode, a regular branch
        // otherwise.
        void
        instAIWNPC(Addr val)
        {
            if (!thumb() && !jazelle())
                instIWNPC(val);
            else
                instNPC(val);
        }

        bool
        operator == (const PCState &opc) const
        {
            return Base::operator == (opc) &&
                flags == opc.flags && nextFlags == opc.nextFlags &&
                _itstate == opc._itstate &&
                _nextItstate == opc._nextItstate &&
                _illegalExec == opc._illegalExec;
        }

        bool
        operator != (const PCState &opc) const
        {
            return !(*this == opc);
        }

        void
        serialize(CheckpointOut &cp) const override
        {
            Base::serialize(cp);
            SERIALIZE_SCALAR(flags);
            SERIALIZE_SCALAR(_size);
            SERIALIZE_SCALAR(nextFlags);
            SERIALIZE_SCALAR(_itstate);
            SERIALIZE_SCALAR(_nextItstate);
            SERIALIZE_SCALAR(_illegalExec);
        }

        void
        unserialize(CheckpointIn &cp) override
        {
            Base::unserialize(cp);
            UNSERIALIZE_SCALAR(flags);
            UNSERIALIZE_SCALAR(_size);
            UNSERIALIZE_SCALAR(nextFlags);
            UNSERIALIZE_SCALAR(_itstate);
            UNSERIALIZE_SCALAR(_nextItstate);
            UNSERIALIZE_SCALAR(_illegalExec);
        }
    };

    // Shift types for ARM instructions
    enum ArmShiftType {
        LSL = 0,
        LSR,
        ASR,
        ROR
    };

    // Extension types for ARM instructions
    enum ArmExtendType {
        UXTB = 0,
        UXTH = 1,
        UXTW = 2,
        UXTX = 3,
        SXTB = 4,
        SXTH = 5,
        SXTW = 6,
        SXTX = 7
    };

    typedef int RegContextParam;
    typedef int RegContextVal;

    //used in FP convert & round function
    enum ConvertType{
        SINGLE_TO_DOUBLE,
        SINGLE_TO_WORD,
        SINGLE_TO_LONG,

        DOUBLE_TO_SINGLE,
        DOUBLE_TO_WORD,
        DOUBLE_TO_LONG,

        LONG_TO_SINGLE,
        LONG_TO_DOUBLE,
        LONG_TO_WORD,
        LONG_TO_PS,

        WORD_TO_SINGLE,
        WORD_TO_DOUBLE,
        WORD_TO_LONG,
        WORD_TO_PS,

        PL_TO_SINGLE,
        PU_TO_SINGLE
    };

    //used in FP convert & round function
    enum RoundMode{
        RND_ZERO,
        RND_DOWN,
        RND_UP,
        RND_NEAREST
    };

    enum ExceptionLevel {
        EL0 = 0,
        EL1,
        EL2,
        EL3
    };

    enum OperatingMode {
        MODE_EL0T = 0x0,
        MODE_EL1T = 0x4,
        MODE_EL1H = 0x5,
        MODE_EL2T = 0x8,
        MODE_EL2H = 0x9,
        MODE_EL3T = 0xC,
        MODE_EL3H = 0xD,
        MODE_USER = 16,
        MODE_FIQ = 17,
        MODE_IRQ = 18,
        MODE_SVC = 19,
        MODE_MON = 22,
        MODE_ABORT = 23,
        MODE_HYP = 26,
        MODE_UNDEFINED = 27,
        MODE_SYSTEM = 31,
        MODE_MAXMODE = MODE_SYSTEM
    };

    enum ExceptionClass {
        EC_INVALID                 = -1,
        EC_UNKNOWN                 = 0x0,
        EC_TRAPPED_WFI_WFE         = 0x1,
        EC_TRAPPED_CP15_MCR_MRC    = 0x3,
        EC_TRAPPED_CP15_MCRR_MRRC  = 0x4,
        EC_TRAPPED_CP14_MCR_MRC    = 0x5,
        EC_TRAPPED_CP14_LDC_STC    = 0x6,
        EC_TRAPPED_HCPTR           = 0x7,
        EC_TRAPPED_SIMD_FP         = 0x7,   // AArch64 alias
        EC_TRAPPED_CP10_MRC_VMRS   = 0x8,
        EC_TRAPPED_BXJ             = 0xA,
        EC_TRAPPED_CP14_MCRR_MRRC  = 0xC,
        EC_ILLEGAL_INST            = 0xE,
        EC_SVC_TO_HYP              = 0x11,
        EC_SVC                     = 0x11,  // AArch64 alias
        EC_HVC                     = 0x12,
        EC_SMC_TO_HYP              = 0x13,
        EC_SMC                     = 0x13,  // AArch64 alias
        EC_SVC_64                  = 0x15,
        EC_HVC_64                  = 0x16,
        EC_SMC_64                  = 0x17,
        EC_TRAPPED_MSR_MRS_64      = 0x18,
        EC_PREFETCH_ABORT_TO_HYP   = 0x20,
        EC_PREFETCH_ABORT_LOWER_EL = 0x20,  // AArch64 alias
        EC_PREFETCH_ABORT_FROM_HYP = 0x21,
        EC_PREFETCH_ABORT_CURR_EL  = 0x21,  // AArch64 alias
        EC_PC_ALIGNMENT            = 0x22,
        EC_DATA_ABORT_TO_HYP       = 0x24,
        EC_DATA_ABORT_LOWER_EL     = 0x24,  // AArch64 alias
        EC_DATA_ABORT_FROM_HYP     = 0x25,
        EC_DATA_ABORT_CURR_EL      = 0x25,  // AArch64 alias
        EC_STACK_PTR_ALIGNMENT     = 0x26,
        EC_FP_EXCEPTION            = 0x28,
        EC_FP_EXCEPTION_64         = 0x2C,
        EC_SERROR                  = 0x2F,
        EC_SOFTWARE_BREAKPOINT     = 0x38,
        EC_SOFTWARE_BREAKPOINT_64  = 0x3C,
    };

    /**
     * Instruction decoder fault codes in ExtMachInst.
     */
    enum DecoderFault : std::uint8_t {
        OK = 0x0, ///< No fault
        UNALIGNED = 0x1, ///< Unaligned instruction fault

        PANIC = 0x3, ///< Internal gem5 error
    };

    BitUnion8(OperatingMode64)
        Bitfield<0> spX;
        Bitfield<3, 2> el;
        Bitfield<4> width;
    EndBitUnion(OperatingMode64)

    static bool inline
    opModeIs64(OperatingMode mode)
    {
        return ((OperatingMode64)(uint8_t)mode).width == 0;
    }

    static bool inline
    opModeIsH(OperatingMode mode)
    {
        return (mode == MODE_EL1H || mode == MODE_EL2H || mode == MODE_EL3H);
    }

    static bool inline
    opModeIsT(OperatingMode mode)
    {
        return (mode == MODE_EL0T || mode == MODE_EL1T || mode == MODE_EL2T ||
                mode == MODE_EL3T);
    }

    static ExceptionLevel inline
    opModeToEL(OperatingMode mode)
    {
        bool aarch32 = ((mode >> 4) & 1) ? true : false;
        if (aarch32) {
            switch (mode) {
              case MODE_USER:
                return EL0;
              case MODE_FIQ:
              case MODE_IRQ:
              case MODE_SVC:
              case MODE_ABORT:
              case MODE_UNDEFINED:
              case MODE_SYSTEM:
                return EL1;
              case MODE_HYP:
                return EL2;
              case MODE_MON:
                return EL3;
              default:
                panic("Invalid operating mode: %d", mode);
                break;
            }
        } else {
            // aarch64
            return (ExceptionLevel) ((mode >> 2) & 3);
        }
    }

    static inline bool
    unknownMode(OperatingMode mode)
    {
        switch (mode) {
          case MODE_EL0T:
          case MODE_EL1T:
          case MODE_EL1H:
          case MODE_EL2T:
          case MODE_EL2H:
          case MODE_EL3T:
          case MODE_EL3H:
          case MODE_USER:
          case MODE_FIQ:
          case MODE_IRQ:
          case MODE_SVC:
          case MODE_MON:
          case MODE_ABORT:
          case MODE_HYP:
          case MODE_UNDEFINED:
          case MODE_SYSTEM:
            return false;
          default:
            return true;
        }
    }

    static inline bool
    unknownMode32(OperatingMode mode)
    {
        switch (mode) {
          case MODE_USER:
          case MODE_FIQ:
          case MODE_IRQ:
          case MODE_SVC:
          case MODE_MON:
          case MODE_ABORT:
          case MODE_HYP:
          case MODE_UNDEFINED:
          case MODE_SYSTEM:
            return false;
          default:
            return true;
        }
    }

} // namespace ArmISA

#endif
