//===-- PatmosInstPrinter.cpp - Convert Patmos MCInst to assembly syntax --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an Patmos MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "asm-printer"
#include "Patmos.h"
#include "PatmosInstPrinter.h"
#include "MCTargetDesc/PatmosBaseInfo.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Format.h"
using namespace llvm;


// Include the auto-generated portion of the assembly writer.
#include "PatmosGenAsmWriter.inc"

bool PatmosInstPrinter::isBundled(const MCInst *MI) const {
  return MI->getOperand(MI->getNumOperands()-1).getImm() > 0;
}

void PatmosInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                  StringRef Annot, const MCSubtargetInfo &STI,
                                  raw_ostream &O)
{
  // Prints bundle marker '{'.
  // This is a workaround. It cannot be printed before the mnemonic by
  // tablegen, otherwise we would not be able to generate matcher tables.
  // We therefore skip printing it in the AsmString and print here before
  // the rest of the instruction.
  printInstPrefix(MI, O);

  printInstruction(MI, Address, O);

  // Last instruction in bundle must not have the bundle bit set.
  if (!isBundled(MI) && InBundle) {
    O << " }";
    InBundle = false;
  }

  printAnnotation(O, Annot);
}

void PatmosInstPrinter::printInstPrefix(const MCInst *MI, raw_ostream &O) {

  // First instruction in bundle?
  if (isBundled(MI) && !InBundle) {
    O << "{ ";
    InBundle = true;
  } else {
    O << "  ";
  }
}

void PatmosInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &O)
{
  // Note: this code is not used to generate inline-assembly. See
  // PatmosAsmPrinter for that.
  // This is used when outputting assembly instead of machine code.
  const MCOperand &Op = MI->getOperand(OpNo);
  auto opcode = MI->getOpcode();
  if (Op.isReg()) {
    // do not print register R0 in addressing modes
    if (!(
          (isLoadInst(opcode) && OpNo == 3) || 
          (isStoreInst(opcode) && OpNo == 2)
        ) || Op.getReg() != Patmos::R0
    ) {
      printRegisterName(Op.getReg(), O);
    }
  } else if (Op.isImm()) {
    int64_t Value = Op.getImm();

    // Print as bytes if requested
    bool IsShifted = false;
    if (PrintBytes == PrintAllAsBytes ||
        (PrintBytes == PrintCallAsBytes && opcode == Patmos::CALL))
    {
      const MCInstrDesc &MID = MII.get(opcode);
      unsigned ImmShift = getPatmosImmediateShift( MID.TSFlags );
      Value <<= ImmShift;
      IsShifted = true;
    }

    // Print as hex only for some instructions, and only if it is in bytes
    // We have the hex value in the disassembly output anyway, and we do not
    // want to print hex for LIin
    if (IsShifted && opcode == Patmos::CALL) {
      O << format("0x%x", Value);
    } else {
      if ( (isLoadInst(opcode) && OpNo == 4) || 
           (isStoreInst(opcode) && OpNo == 3)) 
      {
        const MCOperand &baseOp = MI->getOperand(OpNo - 1);
        if (baseOp.getReg() == Patmos::R0) {
          // r0 is not printed, so no need for special formatting
          // because of signs
          O << Value;
        } else if (Value != 0) {
          O << ((Value < 0) ? " - " : " + ") << std::abs(Value);
        }
        // If value is 0, print nothing
      } else {
        O << Value;
      }
    }
  } else {
    assert(Op.isExpr() && "unknown operand kind in printOperand");
    O << *Op.getExpr();
  }
}

void PatmosInstPrinter::printPredicateOperand(const MCInst *MI, unsigned OpNo,
                                              raw_ostream &O)
{
  unsigned reg  = MI->getOperand(OpNo  ).getReg();
  int      flag = MI->getOperand(OpNo+1).getImm();

  O << ((flag)?"!":" ");
  if (reg == Patmos::NoRegister) {
    printRegisterName(Patmos::P0, O);
  }
  else {
    printRegisterName(reg, O);
  }
}

void PatmosInstPrinter::printGuardOperand(const MCInst *MI, unsigned OpNo,
                                              raw_ostream &O)
{
  assert(OpNo <= 1 && "Guard predicate was not the first operand!");

  unsigned reg  = MI->getOperand(OpNo  ).getReg();
  int      flag = MI->getOperand(OpNo+1).getImm();

  if (reg == Patmos::NoRegister || ((reg == Patmos::P0) && !flag)) {
    printDefaultGuard(O, false);
  } else {
    O << "(" << ((flag)?"!":" ");
    printRegisterName(reg, O);
    O << ")";
  }
}

void PatmosInstPrinter::printPCRelTargetOperand(const MCInst *MI,
                                            unsigned OpNo,
                                            raw_ostream &O)
{
  const MCOperand &Op = MI->getOperand(OpNo);
  // For disassembly .. should we create a fixup for this in the disassembler,
  // or an expression??
  if (Op.isImm()) {

    // should we print out branch targets in words/.. in all cases?
    if (PrintBytes == PrintAllAsBytes) {
      const MCInstrDesc &MID = MII.get(MI->getOpcode());
      unsigned ImmShift = getPatmosImmediateShift( MID.TSFlags );
      O << (Op.getImm() << ImmShift);
    } else {
      O << Op.getImm();
    }

    return;
  }

  assert(Op.isExpr() && "unknown operand kind in printCacheRelTargetOperand");

  O << *Op.getExpr();
}

void PatmosInstPrinter::printRegisterName(unsigned RegNo, raw_ostream &O) {
  O << "$";
  O << getRegisterName(RegNo);
}

void PatmosInstPrinter::printDefaultGuard(raw_ostream &O, bool NoGuard) {
  O << "      "; // instead of ( $p0)
}
