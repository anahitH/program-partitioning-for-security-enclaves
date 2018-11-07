#include "Annotation.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"

namespace vazgen {

void Annotation::dump() const
{
    llvm::dbgs() << "--------- Annotation " << m_annotation << "\n";
    llvm::dbgs() << m_F->getName();
    for (auto arg : m_arguments) {
        llvm::dbgs() << " arg " << arg;
    }
    if (m_return) {
        llvm::dbgs() << " return";
    }
    llvm::dbgs() << "\n";
}

}

