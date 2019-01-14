#pragma once

#include <unordered_set>

namespace llvm {
class BasicBlock;
class Value;
} // namespace llvm

namespace vazgen {

 /**
* \class BasicBlockLivenessInfo
* \brief class containing live variables of the given basic block.
*/
class BasicBlockLivenessInfo
{
public:
    using ValueSet = std::unordered_set<llvm::Value*>;

    BasicBlockLivenessInfo(llvm::BasicBlock* block);

public:
    llvm::BasicBlock* get_block() const;

    void set_liveIn(const ValueSet& liveIn);
    void set_liveOut(const ValueSet& liveOut);
    void add_liveIn(const ValueSet& liveIn);
    void add_liveOut(const ValueSet& liveOut);
    const ValueSet& get_liveIn() const;
    const ValueSet& get_liveOut() const;

    bool is_liveIn(llvm::Value* val) const;
    bool is_liveOut(llvm::Value* val) const;

    void set_phiDefs(const ValueSet& phiDefs);
    void add_phiUse(llvm::Value* val);
    const ValueSet& get_phiDefs() const;
    const ValueSet& get_phiUses() const;

private:
    llvm::BasicBlock* m_block;
    unsigned succ_size;
    ValueSet m_liveIn;
    ValueSet m_liveOut;
    ValueSet m_phiDefs;
    ValueSet m_phiUses;
}; // class BasicBlockLivenessInfo

} // namespace vazgen
