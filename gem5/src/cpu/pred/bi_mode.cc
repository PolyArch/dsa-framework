/*
 * Copyright (c) 2014 The Regents of The University of Michigan
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
 * Authors: Anthony Gutierrez
 */

/* @file
 * Implementation of a bi-mode branch predictor
 */

#include "cpu/pred/bi_mode.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"

BiModeBP::BiModeBP(const BiModeBPParams *params)
    : BPredUnit(params),
      globalHistoryReg(params->numThreads, 0),
      globalHistoryBits(ceilLog2(params->globalPredictorSize)),
      choicePredictorSize(params->choicePredictorSize),
      choiceCtrBits(params->choiceCtrBits),
      globalPredictorSize(params->globalPredictorSize),
      globalCtrBits(params->globalCtrBits)
{
    if (!isPowerOf2(choicePredictorSize))
        fatal("Invalid choice predictor size.\n");
    if (!isPowerOf2(globalPredictorSize))
        fatal("Invalid global history predictor size.\n");

    choiceCounters.resize(choicePredictorSize);
    takenCounters.resize(globalPredictorSize);
    notTakenCounters.resize(globalPredictorSize);

    for (int i = 0; i < choicePredictorSize; ++i) {
        choiceCounters[i].setBits(choiceCtrBits);
    }
    for (int i = 0; i < globalPredictorSize; ++i) {
        takenCounters[i].setBits(globalCtrBits);
        notTakenCounters[i].setBits(globalCtrBits);
    }

    historyRegisterMask = mask(globalHistoryBits);
    choiceHistoryMask = choicePredictorSize - 1;
    globalHistoryMask = globalPredictorSize - 1;

    choiceThreshold = (ULL(1) << (choiceCtrBits - 1)) - 1;
    takenThreshold = (ULL(1) << (globalCtrBits - 1)) - 1;
    notTakenThreshold = (ULL(1) << (globalCtrBits - 1)) - 1;
}

/*
 * For an unconditional branch we set its history such that
 * everything is set to taken. I.e., its choice predictor
 * chooses the taken array and the taken array predicts taken.
 */
void
BiModeBP::uncondBranch(ThreadID tid, Addr pc, void * &bpHistory)
{
    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->takenUsed = true;
    history->takenPred = true;
    history->notTakenPred = true;
    history->finalPred = true;
    bpHistory = static_cast<void*>(history);
    updateGlobalHistReg(tid, true);
}

void
BiModeBP::squash(ThreadID tid, void *bpHistory)
{
    BPHistory *history = static_cast<BPHistory*>(bpHistory);
    globalHistoryReg[tid] = history->globalHistoryReg;

    delete history;
}

/*
 * Here we lookup the actual branch prediction. We use the PC to
 * identify the bias of a particular branch, which is based on the
 * prediction in the choice array. A hash of the global history
 * register and a branch's PC is used to index into both the taken
 * and not-taken predictors, which both present a prediction. The
 * choice array's prediction is used to select between the two
 * direction predictors for the final branch prediction.
 */
bool
BiModeBP::lookup(ThreadID tid, Addr branchAddr, void * &bpHistory)
{
    unsigned choiceHistoryIdx = ((branchAddr >> instShiftAmt)
                                & choiceHistoryMask);
    unsigned globalHistoryIdx = (((branchAddr >> instShiftAmt)
                                ^ globalHistoryReg[tid])
                                & globalHistoryMask);

    assert(choiceHistoryIdx < choicePredictorSize);
    assert(globalHistoryIdx < globalPredictorSize);

    bool choicePrediction = choiceCounters[choiceHistoryIdx].read()
                            > choiceThreshold;
    bool takenGHBPrediction = takenCounters[globalHistoryIdx].read()
                              > takenThreshold;
    bool notTakenGHBPrediction = notTakenCounters[globalHistoryIdx].read()
                                 > notTakenThreshold;
    bool finalPrediction;

    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->takenUsed = choicePrediction;
    history->takenPred = takenGHBPrediction;
    history->notTakenPred = notTakenGHBPrediction;

    if (choicePrediction) {
        finalPrediction = takenGHBPrediction;
    } else {
        finalPrediction = notTakenGHBPrediction;
    }

    history->finalPred = finalPrediction;
    bpHistory = static_cast<void*>(history);
    updateGlobalHistReg(tid, finalPrediction);

    return finalPrediction;
}

void
BiModeBP::btbUpdate(ThreadID tid, Addr branchAddr, void * &bpHistory)
{
    globalHistoryReg[tid] &= (historyRegisterMask & ~ULL(1));
}

/* Only the selected direction predictor will be updated with the final
 * outcome; the status of the unselected one will not be altered. The choice
 * predictor is always updated with the branch outcome, except when the
 * choice is opposite to the branch outcome but the selected counter of
 * the direction predictors makes a correct final prediction.
 */
void
BiModeBP::update(ThreadID tid, Addr branchAddr, bool taken, void *bpHistory,
                 bool squashed)
{
    assert(bpHistory);

    BPHistory *history = static_cast<BPHistory*>(bpHistory);

    // We do not update the counters speculatively on a squash.
    // We just restore the global history register.
    if (squashed) {
        globalHistoryReg[tid] = (history->globalHistoryReg << 1) | taken;
        return;
    }

    unsigned choiceHistoryIdx = ((branchAddr >> instShiftAmt)
                                & choiceHistoryMask);
    unsigned globalHistoryIdx = (((branchAddr >> instShiftAmt)
                                ^ history->globalHistoryReg)
                                & globalHistoryMask);

    assert(choiceHistoryIdx < choicePredictorSize);
    assert(globalHistoryIdx < globalPredictorSize);

    if (history->takenUsed) {
        // if the taken array's prediction was used, update it
        if (taken) {
            takenCounters[globalHistoryIdx].increment();
        } else {
            takenCounters[globalHistoryIdx].decrement();
        }
    } else {
        // if the not-taken array's prediction was used, update it
        if (taken) {
            notTakenCounters[globalHistoryIdx].increment();
        } else {
            notTakenCounters[globalHistoryIdx].decrement();
        }
    }

    if (history->finalPred == taken) {
       /* If the final prediction matches the actual branch's
        * outcome and the choice predictor matches the final
        * outcome, we update the choice predictor, otherwise it
        * is not updated. While the designers of the bi-mode
        * predictor don't explicity say why this is done, one
        * can infer that it is to preserve the choice predictor's
        * bias with respect to the branch being predicted; afterall,
        * the whole point of the bi-mode predictor is to identify the
        * atypical case when a branch deviates from its bias.
        */
        if (history->finalPred == history->takenUsed) {
            if (taken) {
                choiceCounters[choiceHistoryIdx].increment();
            } else {
                choiceCounters[choiceHistoryIdx].decrement();
            }
        }
    } else {
        // always update the choice predictor on an incorrect prediction
        if (taken) {
            choiceCounters[choiceHistoryIdx].increment();
        } else {
            choiceCounters[choiceHistoryIdx].decrement();
        }
    }

    delete history;
}

unsigned
BiModeBP::getGHR(ThreadID tid, void *bp_history) const
{
    return static_cast<BPHistory*>(bp_history)->globalHistoryReg;
}

void
BiModeBP::updateGlobalHistReg(ThreadID tid, bool taken)
{
    globalHistoryReg[tid] = taken ? (globalHistoryReg[tid] << 1) | 1 :
                               (globalHistoryReg[tid] << 1);
    globalHistoryReg[tid] &= historyRegisterMask;
}

BiModeBP*
BiModeBPParams::create()
{
    return new BiModeBP(this);
}
