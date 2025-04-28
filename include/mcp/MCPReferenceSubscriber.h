#pragma once

#include "IMCPSubscriber_V1.h"
#include "MCPBroker.h"
#include "MCPMessage_V1.h"
#include "MCPSerialization.h"
#include "MCPRingBuffer.h"
#include "MCPVariant.h"
#include "rack/framework/mock.h"

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <iostream>

namespace mcp {

// Message type for internal communication
struct ReceivedMessage {
    std::string topic;
    MessageVariant data;
};

/**
 * @brief Reference implementation of an MCP subscriber module
 * 
 * This class demonstrates best practices for implementing the IMCPSubscriber_V1
 * interface in a VCV Rack module. It shows how to:
 * 
 * 1. Subscribe to topics
 * 2. Safely receive and deserialize messages
 * 3. Pass data from the worker thread to the audio thread
 * 4. Properly initialize and clean up
 */
class MCPReferenceSubscriber : public rack::Module, public IMCPSubscriber_V1 {
public:
    /**
     * @brief Constructor
     * @param id Module ID
     */
    explicit MCPReferenceSubscriber(int id = -1);
    
    /**
     * @brief Destructor
     */
    ~MCPReferenceSubscriber() override;
    
    /**
     * @brief Called when module is added to engine
     * 
     * This method handles subscription to MCP topics.
     */
    void onAdd() override;
    
    /**
     * @brief Called when module is removed from engine
     * 
     * This method handles unsubscription from MCP topics.
     */
    void onRemove() override;
    
    /**
     * @brief Process audio
     * @param outputs Pointer to output buffer
     * @param frames Number of frames to process
     * 
     * This method is called from the audio thread. It processes any messages
     * that have been received since the last call, updating internal state.
     */
    void process(float* outputs, int frames) override;
    
    /**
     * @brief Get the current parameter value
     * @param index Parameter index (1 or 2)
     * @return Current parameter value
     */
    float getParameter(int index) const;
    
    /**
     * @brief Get the current preset name
     * @return Current preset name
     */
    const std::string& getPreset() const;
    
    /**
     * @brief Get the current parameter array
     * @return Current parameter array
     */
    const std::vector<float>& getParameterArray() const;
    
    /**
     * @brief MCP message handler
     * @param message Pointer to received message
     * 
     * This method is called by the broker when a message is received.
     * It deserializes the message and passes it to the audio thread.
     */
    void onMCPMessage(const MCPMessage_V1* message) override;
    
    /**
     * @brief Subscribe to a specific topic
     * @param topic Topic to subscribe to
     * @return true if successful, false otherwise
     */
    bool subscribeToTopic(const std::string& topic);
    
    /**
     * @brief Unsubscribe from a specific topic
     * @param topic Topic to unsubscribe from
     * @return true if successful, false otherwise
     */
    bool unsubscribeFromTopic(const std::string& topic);
    
private:
    // Subscribed topics
    std::vector<std::string> m_subscribedTopics;
    
    // Thread-safe ring buffer for passing messages from worker to audio thread
    RingBuffer<ReceivedMessage> m_messageQueue{32};
    
    // Current parameter values (accessed from audio thread)
    float m_parameter1{0.0f};
    float m_parameter2{0.0f};
    std::string m_preset;
    std::vector<float> m_parameterArray;
    
    // Mutex for parameter access
    mutable std::mutex m_paramMutex;
    
    // Message counts for statistics
    std::unordered_map<std::string, int> m_messageCountsByTopic;
    std::atomic<int> m_totalMessagesReceived{0};
    std::atomic<int> m_messagesProcessed{0};
    std::atomic<int> m_queueOverflows{0};
};

} // namespace mcp 