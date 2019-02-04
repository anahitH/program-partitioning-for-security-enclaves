#pragma once

#include "CodeGen/ProtoService.h"
#include "CodeGen/ProtoMessage.h"

#include <vector>

namespace vazgen {

class ProtoFile
{
public:
    struct Import
    {
        std::string m_name;
    };

    using Imports = std::vector<Import>;
    using Services = std::vector<ProtoService>;
    using Messages = std::vector<ProtoMessage>;

public:
    ProtoFile() = default;
    ProtoFile(const std::string& version, const std::string& packageName)
        : m_version(version)
        , m_package(packageName)
    {
    }

    ProtoFile(const ProtoFile& ) = delete;
    ProtoFile(ProtoFile&& ) = delete;
    ProtoFile& operator =(const ProtoFile& ) = delete;
    ProtoFile& operator =(ProtoFile&& ) = delete;

public:
    const std::string& getVersion() const
    {
        return m_version;
    }

    const std::string& getPackage() const
    {
        return m_package;
    }

    const Imports& getImports() const
    {
        return m_imports;
    }

    const Services& getServices() const
    {
        return m_services;
    }

    const Messages& getMessages() const
    {
        return m_messages;
    }

    void setVersion(const std::string& version)
    {
        m_version = version;
    }

    void setPackage(const std::string& package)
    {
        m_package = package;
    }

    void setImports(const Imports& imports)
    {
        m_imports = imports;
    }

    void setImports(Imports&& imports)
    {
        m_imports = std::move(imports);
    }

    void setServices(const Services& services)
    {
        m_services = services;
    }

    void setServices(Services&& services)
    {
        m_services = std::move(services);
    }

    void setMessages(const Messages& messages)
    {
        m_messages = messages;
    }

    void setMessages(Messages&& messages)
    {
        m_messages = std::move(messages);
    }

    void addImport(const Import& import)
    {
        m_imports.push_back(import);
    }

    void addImport(Import&& import)
    {
        m_imports.push_back(std::move(import));
    }

    void addService(const ProtoService& service)
    {
        m_services.push_back(service);
    }

    void addService(ProtoService&& service)
    {
        m_services.push_back(std::move(service));
    }

    void addMessage(const ProtoMessage& message)
    {
        m_messages.push_back(message);
    }

    void addMessage(ProtoMessage&& message)
    {
        m_messages.push_back(std::move(message));
    }

private:
    std::string m_version;
    std::string m_package;
    std::vector<Import> m_imports;
    std::vector<ProtoService> m_services;
    std::vector<ProtoMessage> m_messages;
}; // class ProtoFile

} // namespace vazgen

