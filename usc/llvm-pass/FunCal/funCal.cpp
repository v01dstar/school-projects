#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
    struct FunCal : public ModulePass {
        static char ID;
        FunCal() : ModulePass(ID) {}

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

        bool runOnModule(Module &M) override{
            errs() << "List of function calls:\n";
            StringMap<int> ref_map;
            StringMap<std::string> arg_map;
            for (Module::iterator f = M.begin(), fe = M.end(); f != fe; f++) {
                if (!f->isDeclaration()) {
                    ref_map.insert(std::make_pair(f->getName(), 0));
                    std::string ty = "(";
                    for (unsigned i = 0; i< f->arg_size(); i++) {
                        ty += typeOf(f->getFunctionType()->getParamType(i));
                        if (i != f->arg_size() - 1) {
                            ty += ',';
                        }
                    }
                    ty += ")";
                    arg_map.insert(std::make_pair(f->getName(), ty));
                }
            }
            for (Module::iterator f = M.begin(), fe = M.end(); f != fe; f++) {
                for (inst_iterator I = inst_begin(f), E = inst_end(f); I != E; I++) {
                    if (CallInst *callInst = dyn_cast<CallInst>(&*I)) {
                        if (Function *f_ptr = dyn_cast<Function>(&*callInst->getCalledValue()->stripPointerCasts())) {
                            std::string name = f_ptr->getName();
                            if (ref_map.count(name)) {
                                int newv = ref_map.lookup(name)+1;
                                ref_map.erase(name);
                                ref_map.insert(std::make_pair(name, newv));
                            }
                        }
                    }
                }
            }
            for (auto fu = ref_map.begin(), fue = ref_map.end(); fu != fue; fu++) {
                errs() << fu->getKey();
                errs() << arg_map.lookup(fu->getKey()) << ": ";
                errs() << fu->getValue() << "\n";
            }
            return true;
        }
    };
}

char FunCal::ID = 0;
static RegisterPass<FunCal> X("funcal", "Function call calculation", false, false);
