#ifndef  __MYPROFILEINFO_H__
#define  __MYPROFILEINFO_H__

#include "llvm/BasicBlock.h"
#include "llvm/InstrTypes.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ProfileInfo.h"
#include "llvm/Analysis/ProfileInfoLoader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Streams.h"

namespace llvm
{

class MyProfileInfo : public ProfileInfo
{
  std::string Filename;
  public:
  MyProfileInfo(const std::string &filename="");
  bool runOnModule(Module &M);
};
}

#endif  /*__MYPROFILEINFO_H__*/
