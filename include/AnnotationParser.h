#pragma once

#include "Annotation.h"
#include <unordered_map>
#include <unordered_set>

namespace vazgen {

/**
 * \class AnnotationParser
 * \brief Interface to request Annotations
 */
class AnnotationParser
{
public:
    using Annotations = std::unordered_set<Annotation>;

public:
    AnnotationParser()
        : m_parsed(false)
    {
    }

public:
    bool hasAnnotations(const std::string& annotation)
    {
        if (!m_parsed) {
            parseAnnotations();
        }
        return m_annotations.find(annotation) != m_annotations.end();
    }

    const Annotations& getAnnotations(const std::string& annotation)
    {
        if (!m_parsed) {
            parseAnnotations();
        }
        return m_annotations.find(annotation)->second;
    }
    
protected:
    virtual void parseAnnotations() = 0;

protected:
    bool m_parsed;
    std::unordered_map<std::string, Annotations> m_annotations;
}; // class AnnotationParser

} // namespace

