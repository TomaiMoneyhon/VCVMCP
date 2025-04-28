#pragma once

#include "IMCPProvider_V1.h"
#include "MCPBroker.h"
#include "MCPMessage_V1.h"
#include "MCPSerialization.h"
#include "rack/framework/mock.h"

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>
#include <iostream>

namespace mcp {

/**
 * @brief Reference implementation of an MCP provider module
 * 
 * This class demonstrates best practices for implementing the IMCPProvider_V1
 * interface in a VCV Rack module. It shows how to:
 * 
 * 1. Register and unregister topics with the broker
 * 2. Serialize various data types
 * 3. Create and publish messages
 * 4. Implement thread-safe publishing
 */
class MCPReferenceProvider : public rack::Module, public IMCPProvider_V1 {
public:
    /**
     * @brief Constructor
     * @param id Module ID
     */
    explicit MCPReferenceProvider(int id = -1);
    
    /**
     * @brief Destructor
     */
    ~MCPReferenceProvider() override;
    
    /**
     * @brief Get the topics provided by this module
     * @return Vector of topic strings
     */
    std::vector<std::string> getProvidedTopics() const override;
    
    /**
     * @brief Called when module is added to engine
     * 
     * This method handles registration with the MCP broker.
     */
    void onAdd() override;
    
    /**
     * @brief Called when module is removed from engine
     * 
     * This method handles unregistration from the MCP broker.
     */
    void onRemove() override;
    
    /**
     * @brief Process audio
     * @param outputs Pointer to output buffer
     * @param frames Number of frames to process
     * 
     * This method is called from the audio thread.
     */
    void process(float* outputs, int frames) override;
    
    /**
     * @brief Start periodic publishing
     * @param intervalMs Interval between publishes in milliseconds
     */
    void startPeriodicPublishing(int intervalMs = 1000);
    
    /**
     * @brief Stop periodic publishing
     */
    void stopPeriodicPublishing();
    
    /**
     * @brief Publish a message immediately
     * @param topic Topic to publish to
     * @param value Data to publish
     */
    template<typename T>
    void publishMessage(const std::string& topic, const T& value) {
        try {
            auto message = serialization::createMsgPackMessage(topic, getId(), value);
            auto broker = MCPBroker::getInstance();
            if (broker) {
                broker->publish(message);
            }
        }
        catch (const MCPSerializationError& e) {
            // Log error but don't throw from here
            std::cerr << "Error publishing message: " << e.what() << std::endl;
        }
    }

private:
    // Provider topics
    const std::vector<std::string> m_topics;
    
    // Publishing thread
    std::thread m_publishThread;
    std::atomic<bool> m_publishRunning{false};
    std::mutex m_publishMutex;
    std::condition_variable m_publishCondition;
    int m_publishInterval{1000};
    
    // Internal state (data to publish)
    float m_parameter1{0.0f};
    float m_parameter2{0.5f};
    std::string m_preset{"Default"};
    std::vector<float> m_parameterArray{0.5f, 0.3f, 0.8f, 0.2f, 0.6f};
    
    // Synthetic parameter update (for demo purposes)
    void updateParameters();
    
    // Publishing thread function
    void publishThreadFunc();
};

} // namespace mcp 