#pragma once

#include <fstream>

namespace vazgen {

class FileWriter
{
public:
   FileWriter(const std::string& name);
   ~FileWriter();

   FileWriter(const FileWriter& writer) = delete;
   FileWriter(FileWriter&& writer) = delete;
   FileWriter& operator =(const FileWriter& ) = delete;
   FileWriter& operator =(FileWriter&& ) = delete;

public:
    void write(const std::string& string, int indent = 0);
    void write(int num, int indent = 0);
    void write(unsigned num, int indent = 0);
    void writeNewLine();
    // TODO: add for more types if needed

private:
    void writeIndent(int indent);

private:
    std::ofstream m_stream;
}; // class FileWriter

} // namespace vazgen

