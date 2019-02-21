#include "CodeGen/FileWriter.h"

namespace vazgen {

FileWriter::FileWriter(const std::string& name)
    : m_stream(name)
    //, std::ios::app)
{
}

FileWriter::~FileWriter()
{
    m_stream.flush();
    m_stream.close();
}

void FileWriter::write(const std::string& string, int indent)
{
    writeIndent(indent);
    m_stream << string << "\n";
}

void FileWriter::write(int num, int indent)
{
    writeIndent(indent);
    m_stream << num << "\n";
}

void FileWriter::write(unsigned num, int indent)
{
    writeIndent(indent);
    m_stream << num << "\n";
}

void FileWriter::writeNewLine()
{
    m_stream << "\n";
}

void FileWriter::writeIndent(int indent)
{
    for (int i = 0; i < indent; ++i) {
        m_stream << "   ";
    }
}

} // namespace vazgen

