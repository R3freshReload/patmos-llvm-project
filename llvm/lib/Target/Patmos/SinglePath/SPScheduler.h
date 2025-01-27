//===------------ SchedulePostRAList.h - Scheduler ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef TARGET_PATMOS_SINGLEPATH_SPSCHEDULER_H_
#define TARGET_PATMOS_SINGLEPATH_SPSCHEDULER_H_

#include "llvm/CodeGen/MachineFunctionPass.h"

#include "PatmosTargetMachine.h"
#include "PatmosSPReduce.h"

#define DEBUG_TYPE "patmos-singlepath"

namespace llvm{

class SPScheduler : public MachineFunctionPass {
public:

  static char ID;

  SPScheduler(const PatmosTargetMachine &tm);

  // Override MachineFunctionPass::runOnMachineFunction
  bool runOnMachineFunction(MachineFunction &MF) override;

  /// getPassName - Return the pass' name.
  StringRef getPassName() const override {
    return "Patmos Single-Path Scheduler (machine code)";
  }

  /// getAnalysisUsage - Specify which passes this pass depends on
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    MachineFunctionPass::getAnalysisUsage(AU);
  }

private:

  const PatmosTargetMachine &TM;

  /// Calculates the latency that must be observed between these two instructions.
  /// E.g. if the first instruction loads a value into a register and the
  /// second instruction uses that value the latency is 1, and there must be at
  /// least 1 cycle between the two instructions.
  /// If the instructions are part of bundles, the whole bundle is taken into
  /// account.
  /// If boolean is true, latency is only returned for control flow instructions.
  unsigned calculateLatency(MachineBasicBlock::iterator, bool) const;

  /// Runs a list scheduler on the given block to avoid empty delay slots
  /// in non-control-flow instructions.
  void runListSchedule(MachineBasicBlock *mbb);
};

}

#endif /* TARGET_PATMOS_SINGLEPATH_SPSCHEDULER_H_ */
