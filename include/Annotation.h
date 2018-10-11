#pragma once

#include "llvm/IR/Function.h"

namespace vazgen {

/**
 * \class Annotation
 * \brief Represents security annotation for a function
 */
class Annotation
{
public:
    Annotation(const std::string& annotation, llvm::Function* F)
        : m_annotation(annotation)
        , m_F(F)
        , m_return(false)
    {
    }

    Annotation(const Annotation& ) = default;
    Annotation(Annotation&& ) = default;
    Annotation& operator =(const Annotation&) = default;
    Annotation& operator =(Annotation&& ) = default;

public:
    const std::string& getAnnotation() const
    {
        return m_annotation;
    }

    llvm::Function* getFunction() const
    {
        return m_F;
    }

    const std::vector<unsigned>& getAnnotatedArguments() const
    {
        return m_arguments;
    }

    bool isReturnAnnotated() const
    {
        return m_return;
    }

    void addAnnotatedArgument(int arg)
    {
        m_arguments.push_back(arg);
    }

    void setReturnAnnotation(bool annot)
    {
        m_return = annot;
    }

    bool operator ==(const Annotation& annot) const
    {
        return m_F == annot.getFunction();
    }

private:
    const std::string m_annotation;
    llvm::Function* m_F;
    std::vector<unsigned> m_arguments;
    bool m_return;
}; // class Annotation

} // namespace vazgen

namespace std {

template <>
struct hash<vazgen::Annotation>
{
    size_t operator()(const vazgen::Annotation& annot) const
    {
        return hash<llvm::Function*>()(annot.getFunction());
    }
}; // struct hash<Annotation>

} // namespace std


