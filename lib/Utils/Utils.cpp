#include "Utils/Utils.h"

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

namespace vazgen {

void Utils::saveModule(llvm::Module* M, const std::string& name)
{
    std::error_code EC;
    llvm::raw_fd_ostream OS(name, EC, llvm::sys::fs::OpenFlags::F_None);
    llvm::WriteBitcodeToFile(M, OS);
    OS.flush();
}

} // namespace vazgen

