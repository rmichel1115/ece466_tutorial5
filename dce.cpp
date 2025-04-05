// dce.cpp

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <set>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"

#include "llvm/ADT/Statistic.h"

#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/FileSystem.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/SourceMgr.h"

#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

#include "llvm/Support/CommandLine.h"

using namespace llvm;
static cl::opt<std::string>
        InputFilename(cl::Positional, cl::desc("<input bitcode>"), cl>
static cl::opt<std::string>
        OutputFilename(cl::Positional, cl::desc("<output bitcode>"), >
static cl::opt<bool>
        Mem2Reg("mem2reg",
                cl::desc("Perform memory to register promotion before>                cl::init(false));

static cl::opt<bool>
        Verbose("verbose",
                cl::desc("Verbosely print lots of status messages to >                cl::init(false));


static llvm::Statistic DeadInst = {"", "Dead", "DCE found dead instru>static llvm::Statistic WorkList = {"", "WorkList", "Added to work lis>
bool isDead(Instruction &I)
{
  int opcode = I.getOpcode();
  switch(opcode){
  case Instruction::Add:
  case Instruction::FNeg:
  case Instruction::FAdd:
  case Instruction::Sub:
  case Instruction::FSub:
  case Instruction::Mul:
  case Instruction::FMul:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::FDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::FRem:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
  case Instruction::Alloca:
  case Instruction::GetElementPtr:
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
  case Instruction::FPToUI:
  case Instruction::FPToSI:
  case Instruction::UIToFP:
  case Instruction::SIToFP:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast:
  case Instruction::ICmp:
  case Instruction::FCmp:
  case Instruction::PHI:
  case Instruction::Select:
  case Instruction::ExtractElement:
  case Instruction::InsertElement:
  case Instruction::ShuffleVector:
  case Instruction::ExtractValue:
  case Instruction::InsertValue:
    if ( I.use_begin() == I.use_end() )
         {
               return true;
         }
         break;

  case Instruction::Load:
    {
      LoadInst *li = dyn_cast<LoadInst>(&I);
      if (li && li->isVolatile())
           return false;
      if (I.use_begin() == I.use_end())
         return true;
      break;

    }

  default:
    // any other opcode fails
         return false;
  }

  return false;

}

void NoOptimization(Module &M)
{
std::cout << "Running NoOptimization..." << std::endl;

/*for(Module::iterator i = M.begin(); i!=M.end(); i++)
{
  Function &F = *i;
//or get a pointer to the function:

for(Function::iterator j = F.begin(); j != F.end(); j++) {
BasicBlock &BB = *j;
//BB.dump();

for(BasicBlock::iterator k = BB.begin(); k != BB.end(); k++) {
Instruction &I = *k;

//I.dump();


  if (isDead(I)) {
    //add I to a worklist to replace later
    //std::cout << "Found an instruction to delete!\n";
    DeadInst++;
    k = I.eraseFromParent();
    k--;

  }


}

}
}*/

}

void DeadCodeElimination(Module &M)
{

 std::set<Instruction*> worklist;

 for(Module::iterator i = M.begin(); i!=M.end(); i++)
{
  Function &F = *i;

for(Function::iterator j = F.begin(); j != F.end(); j++) {
BasicBlock &BB = *j;

for(BasicBlock::iterator k = BB.begin(); k != BB.end(); k++) {
{
  if(isDead(*k)) {
    worklist.insert(&*k);
    WorkList++;
//make a worklist full of dead instructions
  }
}
}
}
}
//go through worklist and delete them while checking to make sure
//anything else that becomes dead is taken care of
while (!worklist.empty() ) {
 Instruction * I = *(worklist.begin());
 worklist.erase(I);

 if ( isDead(*I) ) {
  for(unsigned op=0; op<I->getNumOperands(); op++)
{
  Value *v = I->getOperand(op);
  if (isa<Instruction>(I->getOperand(op)) ) {
  //yes, it's an instruction, consider if it's possible dead
  Instruction *J = cast<Instruction>(I->getOperand(op));
  worklist.insert(J);
  WorkList++;
}
}
I->eraseFromParent();
DeadInst++;
}

}

}
int main(int argc, char **argv) {
    cl::ParseCommandLineOptions(argc, argv, "./dce <input> <output> \>
    // LLVM idiom for constructing output file.
    std::unique_ptr<ToolOutputFile> Out;
    std::string ErrorInfo;
    std::error_code EC;
    Out.reset(new ToolOutputFile(OutputFilename, EC,
                                 sys::fs::OF_None));

    SMDiagnostic Err;
    std::unique_ptr<Module> M;
    LLVMContext *Context = new LLVMContext();
    M = parseIRFile(InputFilename, Err, *Context);

    if (M.get() == 0) {
        Err.print(argv[0], errs());
        return 1;
    }

    EnableStatistics();

    if (Mem2Reg) {
        if (Verbose)
            errs() << "Run Mem2Reg.\n";
        legacy::PassManager Passes;
        Passes.add(createPromoteMemoryToRegisterPass());
        Passes.run(*M);
    }
    if (Verbose)
        M->print(errs(), nullptr);

    /* 3. Do optimization on Module */
    //NoOptimization(*M);
    DeadCodeElimination(*M);

    bool res = verifyModule(*M, &errs());
    if (!res) {
        WriteBitcodeToFile(*M, Out->os());
        Out->keep();
    } else {
        fprintf(stderr, "Error: %s not created.\n", argv[2]);
    }

    PrintStatistics(errs());

    return 0;
}
