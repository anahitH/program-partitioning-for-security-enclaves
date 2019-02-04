#pragma once

#include "CodeGen/ProtoMessage.h"

#include <vector>

namespace vazgen {

class ProtoService
{
public:
    struct RPC
    {
        // TODO: add more fields if necessary
        std::string m_name;
        ProtoMessage m_input;
        ProtoMessage m_output;
    };

    using RPCs = std::vector<RPC>;

public:
    ProtoService() = default;
    ProtoService(const std::string& name)
        : m_name(name)
    {
    }

    ProtoService(const ProtoService&) = default;
    ProtoService(ProtoService&&) = default;
    ProtoService& operator =(const ProtoService& ) = default;
    ProtoService& operator =(ProtoService&& ) = default;

public:
    const std::string& getName() const
    {
        return m_name;
    }

    const RPCs& getRPCs() const
    {
        return m_rpcs;
    }

    void setName(const std::string& name)
    {
        m_name = name;
    }

    void setRPCs(const RPCs& rpcs)
    {
        m_rpcs = rpcs;
    }

    void setRPCs(RPCs&& rpcs)
    {
        m_rpcs = std::move(rpcs);
    }

    void addRPC(const RPC& rpc)
    {
        m_rpcs.push_back(rpc);
    }

    void addRPC(RPC&& rpc)
    {
        m_rpcs.push_back(std::move(rpc));
    }

private:
    std::string m_name;
    RPCs m_rpcs;
}; // class ProtoService

} // namespace vazgen

