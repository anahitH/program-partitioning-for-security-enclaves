#pragma once

#include "Annotation.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

    std::vector<Annotation> getAllAnnotations()
    {
        if (!m_parsed) {
            parseAnnotations();
        }
        std::vector<Annotation> allAnnotations;
        for (auto& pair : m_annotations) {
            allAnnotations.insert(allAnnotations.begin(), pair.second.begin(), pair.second.end());
        }
        return allAnnotations;
    }
    
public:
    virtual void parseAnnotations() = 0;

protected:
    bool m_parsed;
    std::unordered_map<std::string, Annotations> m_annotations;
}; // class AnnotationParser

} // namespace

