#include "Analysis/BasicBlockLivenessInfo.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Value.h"

namespace vazgen {

BasicBlockLivenessInfo::BasicBlockLivenessInfo(llvm::BasicBlock* block)
    : m_block(block)
{
}

llvm::BasicBlock* BasicBlockLivenessInfo::get_block() const
{
    return m_block;
}

void BasicBlockLivenessInfo::set_liveIn(const ValueSet& liveIn)
{
    m_liveIn = liveIn;
}

void BasicBlockLivenessInfo::set_liveOut(const ValueSet& liveOut)
{
    m_liveOut = liveOut;
}

void BasicBlockLivenessInfo::add_liveIn(const ValueSet& liveIn)
{
    m_liveIn.insert(liveIn.begin(), liveIn.end());
}

void BasicBlockLivenessInfo::add_liveOut(const ValueSet& liveOut)
{
    m_liveOut.insert(liveOut.begin(), liveOut.end());
}

const BasicBlockLivenessInfo::ValueSet& BasicBlockLivenessInfo::get_liveIn() const
{
    return m_liveIn;
}

const BasicBlockLivenessInfo::ValueSet& BasicBlockLivenessInfo::get_liveOut() const
{
    return m_liveOut;
}

bool BasicBlockLivenessInfo::is_liveIn(llvm::Value* val) const
{
    return m_liveIn.find(val) != m_liveIn.end();
}

bool BasicBlockLivenessInfo::is_liveOut(llvm::Value* val) const
{
    return m_liveOut.find(val) != m_liveOut.end();
}

void BasicBlockLivenessInfo::set_phiDefs(const ValueSet& phiDefs)
{
    m_phiDefs = phiDefs;
}

void BasicBlockLivenessInfo::add_phiUse(llvm::Value* val)
{
    m_phiUses.insert(val);
}

const BasicBlockLivenessInfo::ValueSet& BasicBlockLivenessInfo::get_phiDefs() const
{
    return m_phiDefs;
}

const BasicBlockLivenessInfo::ValueSet& BasicBlockLivenessInfo::get_phiUses() const
{
    return m_phiUses;
}

} // namespace vazgen

