#pragma once

#include "IMCPBroker.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace mcp {

/**
 * @brief Implementation of the MCP Broker.
 * 
 * This class implements the IMCPBroker interface and provides the central broker
 * functionality for the Model Context Protocol. It is implemented as a singleton
 * and provides thread-safe access to the topic registry.
 */
class MCPBroker : public IMCPBroker, public std::enable_shared_from_this<MCPBroker> {
public:
    /**
     * @brief Get the singleton instance of the MCPBroker.
     * 
     * This static method ensures only one instance of the broker exists.
     * Thread-safe - multiple concurrent calls will correctly create a single instance.
     * 
     * @return std::shared_ptr<MCPBroker> The singleton broker instance.
     */
    static std::shared_ptr<MCPBroker> getInstance();

    /**
     * @brief Constructor.
     * 
     * Made public to allow std::make_shared to work.
     * Should not be called directly; use getInstance() instead.
     */
    MCPBroker();

    /**
     * @brief Destructor.
     * 
     * Made public to allow std::make_shared to work properly.
     */
    ~MCPBroker() override;

    // IMCPBroker interface implementations
    bool registerContext(const std::string& topic, 
                        std::shared_ptr<IMCPProvider_V1> provider) override;
    
    bool unregisterContext(const std::string& topic, 
                          std::shared_ptr<IMCPProvider_V1> provider) override;
    
    bool subscribe(const std::string& topic,
                  std::shared_ptr<IMCPSubscriber_V1> subscriber) override;
    
    bool unsubscribe(const std::string& topic,
                    std::shared_ptr<IMCPSubscriber_V1> subscriber) override;
    
    bool unsubscribeAll(std::shared_ptr<IMCPSubscriber_V1> subscriber) override;
    
    std::vector<std::string> getAvailableTopics() const override;
    
    std::vector<std::shared_ptr<IMCPProvider_V1>> findProviders(
                                        const std::string& topic) const override;
    
    bool publish(std::shared_ptr<MCPMessage_V1> message) override;
    
    int getVersion() const override;

private:
    // Worker thread function for processing the message queue
    void processMessageQueue();

    // Helper to deliver a message to all subscribers of a topic
    void deliverMessage(std::shared_ptr<MCPMessage_V1> message);

    // Prevent copying/moving
    MCPBroker(const MCPBroker&) = delete;
    MCPBroker& operator=(const MCPBroker&) = delete;
    MCPBroker(MCPBroker&&) = delete;
    MCPBroker& operator=(MCPBroker&&) = delete;

    // Static instance pointer
    static std::shared_ptr<MCPBroker> s_instance;
    static std::mutex s_instanceMutex;
    
    // Registry data structure: topic -> list of providers
    using ProviderMap = std::unordered_map<std::string, 
                                          std::vector<std::weak_ptr<IMCPProvider_V1>>>;
    mutable std::mutex m_registryMutex;
    ProviderMap m_topicRegistry;
    
    // Subscription data structure: topic -> list of subscribers
    using SubscriberMap = std::unordered_map<std::string, 
                                           std::vector<std::weak_ptr<IMCPSubscriber_V1>>>;
    mutable std::mutex m_subscriptionMutex;
    SubscriberMap m_subscriptions;

    // Message queue for publish/subscribe
    std::queue<std::shared_ptr<MCPMessage_V1>> m_messageQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    
    // Worker thread for processing messages
    std::thread m_workerThread;
    std::atomic<bool> m_threadRunning;
};

} // namespace mcp 