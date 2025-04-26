#pragma once

#include <string>
#include <vector>
#include <memory>

namespace mcp {

/**
 * @brief Interface for modules that provide contextual information.
 * 
 * This interface is implemented by modules that want to provide contextual
 * information to other modules through the MCP system. Provider modules
 * register topics they offer with the broker and publish updates when data changes.
 * 
 * Version 1 of the interface.
 */
class IMCPProvider_V1 {
public:
    virtual ~IMCPProvider_V1() = default;

    /**
     * @brief Get the list of topics provided by this module.
     * 
     * @return std::vector<std::string> List of topic names provided by this module.
     */
    virtual std::vector<std::string> getProvidedTopics() const = 0;
};

} // namespace mcp 