#include "llvm/Pass.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

namespace {
    struct ArgCheck : public FunctionPass {
        static char ID;
        ArgCheck(): FunctionPass(ID) {}
    
        std::string typeOf(Type *vtype) {
            // If it is interger type. (int, short, long, char)
            if (vtype->isPointerTy()) {
                return typeOf(vtype->getPointerElementType()) + '*';
            } else if (vtype->isIntegerTy()) {
                switch (vtype->getIntegerBitWidth()) {
                    case 8 : {
                        return "char";
                        break;
                    }
                    case 16 : {
                        return "short";
                        break;
                    }
                    default : {
                        return "int";
                    }
                }
            } else {
                std::string s;
                raw_string_ostream result(s);
                vtype->print(result);
                return result.str();
            }
        }

        bool runOnFunction(Function &F) override {
            for (Function::iterator b = F.begin(), be = F.end(); b != be; b++) {
                for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; i++) {
                    if (CallInst *callInst = dyn_cast<CallInst>(&*i)) {
                        Value *v_ptr = callInst->getCalledValue();
                        if (Function *f_ptr = dyn_cast<Function>(&*v_ptr->stripPointerCasts())) {
                            if (!f_ptr->isDeclaration()) {
                                errs() << "Function <";
                                errs().write_escaped(f_ptr->getName()) << "> call on line '";
                                errs() << i->getDebugLoc()->getLine() << "': ";
                                if (callInst->getNumArgOperands() == f_ptr->arg_size()) {
                                    bool err = false;
                                    for (unsigned i = 0; i < callInst->getNumArgOperands(); i++) {
                                        auto callee_arg = typeOf(callInst->getArgOperand(i)->getType());
                                        auto caller_arg = typeOf(f_ptr->getFunctionType()->getParamType(i));
                                        if (callee_arg == caller_arg) {
                                            continue;
                                        } else {
                                            errs() << "argument type mismatch. Expected '";
                                            errs() << caller_arg;
                                            errs() << "' but argument is of type '";
                                            errs() << callee_arg;
                                            errs() << "'.\n";
                                            err = true;
                                            break;
                                        }
                                    }
                                    if (err == false) {
                                        errs() << "Success!\n";
                                    }
                                } else {
                                        errs() << "expected '" << f_ptr->arg_size() << "' arguments but '" << callInst->getNumArgOperands() << "' are/is present.\n";
                                }
                            }
                        } else {
                            errs() << "Function ptr not reacheable\n" ;
                        }
                    }
                }
            }
            return true;
        }
    };
}

char ArgCheck::ID = 0;
static RegisterPass<ArgCheck> X("argcheck", "Arg Check Pass", false, false);
