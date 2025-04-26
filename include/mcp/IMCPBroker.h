#pragma once

#include "IMCPProvider_V1.h"
#include "IMCPSubscriber_V1.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace mcp {

// Forward declaration
struct MCPMessage_V1;

/**
 * @brief Interface for the MCP Broker.
 * 
 * This interface defines the core functionality of the Model Context Protocol broker.
 * The broker is responsible for managing the registry of topics and their providers,
 * managing subscriptions, and message dispatch.
 */
class IMCPBroker {
public:
    virtual ~IMCPBroker() = default;

    /**
     * @brief Register a context topic provided by a module.
     * 
     * This function is called by provider modules to register a topic they offer.
     * Thread-safe - can be called from any thread.
     * 
     * @param topic The name of the topic being registered.
     * @param provider A shared pointer to the provider module.
     * @return bool True if registration was successful, false otherwise.
     */
    virtual bool registerContext(const std::string& topic, 
                                std::shared_ptr<IMCPProvider_V1> provider) = 0;

    /**
     * @brief Unregister a context topic provided by a module.
     * 
     * This function is called when a provider module no longer offers a topic,
     * or when the module is being removed. Thread-safe - can be called from any thread.
     * 
     * @param topic The name of the topic being unregistered.
     * @param provider A shared pointer to the provider module.
     * @return bool True if unregistration was successful, false otherwise.
     */
    virtual bool unregisterContext(const std::string& topic, 
                                  std::shared_ptr<IMCPProvider_V1> provider) = 0;

    /**
     * @brief Subscribe to a context topic.
     * 
     * This function is called by modules that want to receive updates for a specific topic.
     * Thread-safe - can be called from any thread.
     * 
     * @param topic The name of the topic to subscribe to.
     * @param subscriber A shared pointer to the subscriber module.
     * @return bool True if subscription was successful, false otherwise.
     */
    virtual bool subscribe(const std::string& topic,
                          std::shared_ptr<IMCPSubscriber_V1> subscriber) = 0;

    /**
     * @brief Unsubscribe from a context topic.
     * 
     * This function is called when a subscriber no longer wants to receive updates
     * for a specific topic. Thread-safe - can be called from any thread.
     * 
     * @param topic The name of the topic to unsubscribe from.
     * @param subscriber A shared pointer to the subscriber module.
     * @return bool True if unsubscription was successful, false otherwise.
     */
    virtual bool unsubscribe(const std::string& topic,
                            std::shared_ptr<IMCPSubscriber_V1> subscriber) = 0;

    /**
     * @brief Unsubscribe from all topics.
     * 
     * This function is called when a subscriber is being removed from the system
     * and wants to clean up all of its subscriptions. Thread-safe - can be called
     * from any thread.
     * 
     * @param subscriber A shared pointer to the subscriber module.
     * @return bool True if all unsubscriptions were successful, false otherwise.
     */
    virtual bool unsubscribeAll(std::shared_ptr<IMCPSubscriber_V1> subscriber) = 0;

    /**
     * @brief Get a list of all available topics in the registry.
     * 
     * This function allows modules to discover what topics are currently available
     * in the system. Thread-safe - can be called from any thread.
     * 
     * @return std::vector<std::string> List of available topic names.
     */
    virtual std::vector<std::string> getAvailableTopics() const = 0;

    /**
     * @brief Find providers for a specific topic.
     * 
     * This function allows modules to discover which providers are offering a specific topic.
     * Thread-safe - can be called from any thread.
     * 
     * @param topic The name of the topic to find providers for.
     * @return std::vector<std::shared_ptr<IMCPProvider_V1>> List of providers offering the topic.
     */
    virtual std::vector<std::shared_ptr<IMCPProvider_V1>> findProviders(
                                                const std::string& topic) const = 0;

    /**
     * @brief Get the API version of the broker.
     * 
     * This function returns the version number of the broker implementation.
     * Used for compatibility checking.
     * 
     * @return int The API version number (1 for V1).
     */
    virtual int getVersion() const = 0;
};

/**
 * @brief Get the global MCP Broker instance.
 * 
 * This function provides access to the singleton instance of the MCP Broker.
 * Thread-safe - will initialize the broker in a thread-safe manner if needed.
 * 
 * @return std::shared_ptr<IMCPBroker> A shared pointer to the broker instance.
 */
std::shared_ptr<IMCPBroker> getMCPBroker();

} // namespace mcp 