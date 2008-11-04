#ifndef  __MYPROFILEINFO_CPP__
#define  __MYPROFILEINFO_CPP__

#include "llvm/BasicBlock.h"
#include "llvm/InstrTypes.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ProfileInfo.h"
#include "llvm/Analysis/ProfileInfoLoader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Streams.h"

#include "llvm/Analysis/MyProfileInfo.h"

using namespace llvm;

MyProfileInfo::MyProfileInfo(const std::string &filename)
{
///  if (filename.empty()) Filename = ProfileInfoFilename;
  if (filename.empty()) Filename = "llvmprof.out";
}

bool MyProfileInfo::runOnModule(Module &M) {
  ProfileInfoLoader PIL("profile-loader", Filename, M);
  EdgeCounts.clear();
  bool PrintedWarning = false;

  std::vector<std::pair<ProfileInfoLoader::Edge, unsigned> > ECs;
  PIL.getEdgeCounts(ECs);
  for (unsigned i = 0, e = ECs.size(); i != e; ++i) {
    BasicBlock *BB = ECs[i].first.first;
    unsigned SuccNum = ECs[i].first.second;
    TerminatorInst *TI = BB->getTerminator();
    if (SuccNum >= TI->getNumSuccessors()) {
      if (!PrintedWarning) {
        cerr << "WARNING: profile information is inconsistent with "
             << "the current program!\n";
        PrintedWarning = true;
      }
    } else {
      EdgeCounts[std::make_pair(BB, TI->getSuccessor(SuccNum))]+= ECs[i].second;
    }
  }

  printProfile();

  return false;
}
#endif  /*__MYPROFILEINFO_CPP__*/
