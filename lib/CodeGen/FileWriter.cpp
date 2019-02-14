#include "CodeGen/FileWriter.h"

namespace vazgen {

FileWriter::FileWriter(const std::string& name)
    : m_stream(name)
    //, std::ios::app)
{
}

FileWriter::~FileWriter()
{
    m_stream.close();
}

void FileWriter::write(const std::string& string)
{
    m_stream << string << "\n";
}

void FileWriter::write(int num)
{
    m_stream << num << "\n";
}

void FileWriter::write(unsigned num)
{
    m_stream << num << "\n";
}

} // namespace vazgen

