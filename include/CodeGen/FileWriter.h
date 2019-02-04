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
    void write(const std::string& string);
    void write(int num);
    void write(unsigned num);
    // TODO: add for more types if needed

private:
    std::ofstream m_stream;
}; // class FileWriter

} // namespace vazgen

