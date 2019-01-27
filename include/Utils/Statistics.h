#pragma once

#include <fstream>
#include <memory>
#include <vector>

namespace vazgen {

class Statistics
{
public:
    // Add more formats if needed
    enum Format {
        JSON
    };
    
    using key = std::vector<std::string>;

public:
    class Writer
    {
    public:
        Writer(std::ofstream& strm);

        virtual ~Writer()
        {}

    public:
        virtual void flush() = 0;
        virtual void write_entry(const Statistics::key& k, double value) = 0;
        virtual void write_entry(const Statistics::key& k, unsigned value) = 0;
        virtual void write_entry(const Statistics::key& k, int value) = 0;
        virtual void write_entry(const Statistics::key& k, const std::string& value) = 0;
        virtual void write_entry(const Statistics::key& k, const std::vector<std::string>& value) = 0;

    protected:
        std::ofstream& m_strm;
    }; // class Writer

public:
    Statistics() = default;
    Statistics(std::ofstream& strm, Format format);

    virtual ~Statistics()
    {}

    virtual void report() = 0;

    void flush()
    {
        m_writer->flush();
    }

protected:
    void write_entry(const key& key, double value);
    void write_entry(const key& key, unsigned value);
    void write_entry(const key& key, int value);
    void write_entry(const key& key, const std::string& value);
    void write_entry(const key& key, const std::vector<std::string>& value);

protected:
    using WriterType = std::shared_ptr<Writer>;
    WriterType m_writer;
}; // class Statistic

} // namespace vazgen

