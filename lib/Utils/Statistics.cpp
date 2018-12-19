#include "Statistics.h"

#include "nlohmann/json.hpp"
#include <iostream>
#include <iomanip>

namespace vazgen {

class JsonWriter : public Statistics::Writer
{
public:
    JsonWriter(std::ofstream& strm)
        : Statistics::Writer(strm)
    {
    }

public:
    void flush() final
    {
        m_strm << std::setw(4) << m_root;
        m_root.clear();
    }

    void write_entry(const Statistics::key& k, double value) final
    {
        write(k, value);
    }

    void write_entry(const Statistics::key& k, unsigned value) final
    {
        write(k, value);
    }

    void write_entry(const Statistics::key& k, const std::string& value) final
    {
        write(k, value);
    }

    void write_entry(const Statistics::key& k, const std::vector<std::string>& value) final
    {
        write(k, value);
    }

private:
    template<class ValueTy>
    void write(const Statistics::key& k, const ValueTy& value);

private:
    nlohmann::json m_root;
};

Statistics::Writer::Writer(std::ofstream& strm)
    : m_strm(strm)
{
}

template<class ValueTy>
void JsonWriter::write(const Statistics::key& k, const ValueTy& value)
{
    if (k.empty()) {
        return;
    }
    // TODO: fix this for any size of k
    m_root[k[0]][k[1]] = value;

    //nlohmann::json obj;
    //obj[k.back()] = value;

    //for (int i = k.size() - 2; i > 0; ++i) {
    //    nlohmann::json subObj;
    //    subObj[k[i]] = obj;
    //    obj = subObj;
    //}

    //m_root[k.front()] = obj;
}

Statistics::Statistics(std::ofstream& strm, Format format)
{
    if (format == JSON) {
        m_writer.reset(new JsonWriter(strm));
    } else {
        assert(false);
    }
}

void Statistics::write_entry(const key& key, double value)
{
    m_writer->write_entry(key, value);
}

void Statistics::write_entry(const key& key, unsigned value)
{
    m_writer->write_entry(key, value);
}

void Statistics::write_entry(const key& key, const std::string& value)
{
    m_writer->write_entry(key, value);
}

void Statistics::write_entry(const key& key, const std::vector<std::string>& value)
{
    m_writer->write_entry(key, value);
}

} // namespace vazgen

