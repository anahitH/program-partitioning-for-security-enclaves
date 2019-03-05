#pragma once

#include "CodeGen/ProtoService.h"
#include "CodeGen/ProtoMessage.h"

#include <vector>
#include <unordered_map>

namespace vazgen {

class ProtoFile
{
public:
    struct Import
    {
        std::string m_name;
    };

    using Imports = std::vector<Import>;
    using Services = std::unordered_map<std::string, ProtoService>;
    using Messages = std::unordered_map<std::string, ProtoMessage>;

public:
    ProtoFile() = default;
    ProtoFile(const std::string& name,
              const std::string& version,
              const std::string& packageName)
        : m_name(name)
        , m_version(version)
        , m_package(packageName)
    {
    }

    ProtoFile(const ProtoFile& ) =default;
    ProtoFile(ProtoFile&& ) = default;
    ProtoFile& operator =(const ProtoFile& ) = default;
    ProtoFile& operator =(ProtoFile&& ) = default;

public:
    const std::string& getName() const
    {
        return m_name;
    }

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

    bool hasService(const std::string& name) const
    {
        return m_services.find(name) != m_services.end();
    }

    ProtoService& getService(const std::string& name)
    {
        return m_services.find(name)->second;
    }

    const ProtoService& getService(const std::string& name) const
    {
        return m_services.find(name)->second;
    }

    const Messages& getMessages() const
    {
        return m_messages;
    }

    bool hasMessage(const std::string& name) const
    {
        return m_messages.find(name) != m_messages.end();
    }

    const ProtoMessage& getMessage(const std::string& name) const
    {
        return m_messages.find(name)->second;
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
        m_services.insert(std::make_pair(service.getName(), service));
    }

    void addService(ProtoService&& service)
    {
        m_services.insert(std::make_pair(service.getName(), std::move(service)));
    }

    void addMessage(const ProtoMessage& message)
    {
        m_messages.insert(std::make_pair(message.getName(), message));
    }

    void addMessage(ProtoMessage&& message)
    {
        std::string name = message.getName();
        m_messages.insert(std::make_pair(name, std::move(message)));
    }

private:
    std::string m_name;
    std::string m_version;
    std::string m_package;
    std::vector<Import> m_imports;
    Services m_services;
    Messages m_messages;
}; // class ProtoFile

} // namespace vazgen

