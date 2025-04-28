#include "mcp/MCPReferenceSubscriber.h"
#include <iostream>

namespace mcp {

MCPReferenceSubscriber::MCPReferenceSubscriber(int id)
    : rack::Module(id) {
    // Initialize with default topics
    m_subscribedTopics = {
        "reference/parameter1", 
        "reference/parameter2", 
        "reference/preset", 
        "reference/parameters"
    };
    
    // Initialize parameter array with zeros
    m_parameterArray.resize(5, 0.0f);
}

MCPReferenceSubscriber::~MCPReferenceSubscriber() {
    // Do not attempt to unsubscribe here - should be done in onRemove
    // This avoids using shared_from_this during destruction, which can cause bad_weak_ptr
}

void MCPReferenceSubscriber::onAdd() {
    rack::Module::onAdd();
    
    // Get broker instance
    auto broker = MCPBroker::getInstance();
    if (!broker) {
        std::cerr << "Failed to get broker instance" << std::endl;
        return;
    }
    
    // Create a shared_ptr to this
    auto selfPtr = std::dynamic_pointer_cast<IMCPSubscriber_V1>(rack::Module::shared_from_this());
    if (!selfPtr) {
        std::cerr << "Failed to get shared_ptr to subscriber" << std::endl;
        return;
    }
    
    // Subscribe to all topics
    for (const auto& topic : m_subscribedTopics) {
        if (broker->subscribe(topic, selfPtr)) {
            std::cout << "Subscriber " << getId() << " subscribed to topic: " << topic << std::endl;
        } else {
            std::cerr << "Failed to subscribe to topic: " << topic << std::endl;
        }
    }
}

void MCPReferenceSubscriber::onRemove() {
    // Get broker instance
    auto broker = MCPBroker::getInstance();
    if (!broker) {
        std::cerr << "Failed to get broker instance" << std::endl;
        return;
    }
    
    // Create a shared_ptr to this
    auto selfPtr = std::dynamic_pointer_cast<IMCPSubscriber_V1>(rack::Module::shared_from_this());
    if (!selfPtr) {
        std::cerr << "Failed to get shared_ptr to subscriber for unsubscription" << std::endl;
        // Continue with cleanup even if we can't get the shared_ptr
    }
    else {
        // Unsubscribe from all topics
        if (broker->unsubscribeAll(selfPtr)) {
            std::cout << "Subscriber " << getId() << " unsubscribed from all topics" << std::endl;
        } else {
            std::cerr << "Failed to unsubscribe from topics" << std::endl;
        }
    }
    
    rack::Module::onRemove();
}

void MCPReferenceSubscriber::process(float* outputs, int frames) {
    // This method is called from the audio thread
    auto threadType = rack::engine::getThreadType();
    if (threadType != rack::engine::AUDIO_THREAD) {
        std::cerr << "Warning: process() called from non-audio thread!" << std::endl;
    }
    
    // Process any pending messages in the queue
    ReceivedMessage message;
    bool hasNewMessages = false;
    int messagesProcessedThisCycle = 0;
    const int MAX_MESSAGES_PER_CYCLE = 10; // Limit to avoid audio glitches
    
    // Process up to MAX_MESSAGES_PER_CYCLE messages per audio cycle
    while (messagesProcessedThisCycle < MAX_MESSAGES_PER_CYCLE && m_messageQueue.pop(message)) {
        hasNewMessages = true;
        m_messagesProcessed.fetch_add(1);
        messagesProcessedThisCycle++;
        
        try {
            // Process the message based on its topic
            if (message.topic == "reference/parameter1") {
                if (message.data.isFloat()) {
                    std::lock_guard<std::mutex> lock(m_paramMutex);
                    m_parameter1 = message.data.getFloat();
                }
            } else if (message.topic == "reference/parameter2") {
                if (message.data.isFloat()) {
                    std::lock_guard<std::mutex> lock(m_paramMutex);
                    m_parameter2 = message.data.getFloat();
                }
            } else if (message.topic == "reference/preset") {
                if (message.data.isString()) {
                    std::lock_guard<std::mutex> lock(m_paramMutex);
                    m_preset = message.data.getString();
                }
            } else if (message.topic == "reference/parameters") {
                if (message.data.isVectorFloat()) {
                    std::lock_guard<std::mutex> lock(m_paramMutex);
                    m_parameterArray = message.data.getVectorFloat();
                }
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Error processing message data: " << e.what() << std::endl;
        }
    }
    
    // If we processed MAX_MESSAGES_PER_CYCLE, there might be more in the queue
    if (messagesProcessedThisCycle == MAX_MESSAGES_PER_CYCLE && !m_messageQueue.empty()) {
        std::cout << "Audio thread limited message processing, queue still has " 
                  << m_messageQueue.size() << " messages" << std::endl;
    }
    
    // If queue overflowed since last audio cycle, log it
    static int lastQueueOverflows = 0;
    int currentOverflows = m_queueOverflows.load();
    if (currentOverflows > lastQueueOverflows) {
        std::cout << "Queue overflow detected: " << (currentOverflows - lastQueueOverflows) 
                  << " messages lost" << std::endl;
        lastQueueOverflows = currentOverflows;
    }
    
    // Use the processed parameters to generate audio outputs
    // This demonstrates how the received MCP data can be used in audio processing
    const float parameter1 = m_parameter1; // Main amplitude
    const float parameter2 = m_parameter2; // Modulation amount
    
    // Generate output using parameter values
    for (int i = 0; i < frames; ++i) {
        // Simple modulated sine wave output as an example
        float time = static_cast<float>(i) / static_cast<float>(frames);
        float modulation = parameter2 * 0.5f * std::sin(time * 10.0f);
        outputs[i] = parameter1 * std::sin(time * 5.0f + modulation * 3.0f);
    }
    
    // Log processing statistics occasionally
    static int processCycles = 0;
    if (++processCycles % 1000 == 0) {
        std::cout << "Subscriber " << getId() << " stats - Messages received: " 
                  << m_totalMessagesReceived.load() << ", Processed: " 
                  << m_messagesProcessed.load() << ", Queue overflows: " 
                  << m_queueOverflows.load() << std::endl;
    }
}

float MCPReferenceSubscriber::getParameter(int index) const {
    std::lock_guard<std::mutex> lock(m_paramMutex);
    if (index == 1) {
        return m_parameter1;
    } else if (index == 2) {
        return m_parameter2;
    }
    return 0.0f;
}

const std::string& MCPReferenceSubscriber::getPreset() const {
    std::lock_guard<std::mutex> lock(m_paramMutex);
    return m_preset;
}

const std::vector<float>& MCPReferenceSubscriber::getParameterArray() const {
    std::lock_guard<std::mutex> lock(m_paramMutex);
    return m_parameterArray;
}

void MCPReferenceSubscriber::onMCPMessage(const MCPMessage_V1* message) {
    if (!message) {
        return;
    }
    
    // This method is called on a worker thread, not the audio thread!
    auto threadType = rack::engine::getThreadType();
    if (threadType == rack::engine::AUDIO_THREAD) {
        std::cerr << "Warning: onMCPMessage() called from audio thread!" << std::endl;
    }
    
    // Count received messages
    m_totalMessagesReceived.fetch_add(1);
    
    // Update per-topic message count
    m_messageCountsByTopic[message->topic]++;
    
    // Process the message based on its topic
    try {
        ReceivedMessage receivedMsg;
        receivedMsg.topic = message->topic;
        
        if (message->topic == "reference/parameter1" || message->topic == "reference/parameter2") {
            // Extract float parameter
            float value = serialization::extractMessageData<float>(message);
            receivedMsg.data = value;
        } else if (message->topic == "reference/preset") {
            // Extract preset name
            std::string value = serialization::extractMessageData<std::string>(message);
            receivedMsg.data = value;
        } else if (message->topic == "reference/parameters") {
            // Extract parameter array
            std::vector<float> value = serialization::extractMessageData<std::vector<float>>(message);
            receivedMsg.data = value;
        } else {
            // Unknown topic, ignore
            return;
        }
        
        // Push to ring buffer for audio thread to process
        if (!m_messageQueue.push(receivedMsg)) {
            // Queue is full, increment overflow counter
            m_queueOverflows.fetch_add(1);
        }
    } catch (const MCPSerializationError& e) {
        std::cerr << "Error deserializing message: " << e.what() << std::endl;
    }
}

bool MCPReferenceSubscriber::subscribeToTopic(const std::string& topic) {
    auto broker = MCPBroker::getInstance();
    if (!broker) {
        return false;
    }
    
    // Create a shared_ptr to this
    auto selfPtr = std::dynamic_pointer_cast<IMCPSubscriber_V1>(rack::Module::shared_from_this());
    if (!selfPtr) {
        std::cerr << "Failed to get shared_ptr to subscriber for subscription" << std::endl;
        return false;
    }
    
    // Check if already subscribed
    auto it = std::find(m_subscribedTopics.begin(), m_subscribedTopics.end(), topic);
    if (it != m_subscribedTopics.end()) {
        return true;  // Already subscribed
    }
    
    // Subscribe
    if (broker->subscribe(topic, selfPtr)) {
        m_subscribedTopics.push_back(topic);
        std::cout << "Subscriber " << getId() << " subscribed to topic: " << topic << std::endl;
        return true;
    }
    
    return false;
}

bool MCPReferenceSubscriber::unsubscribeFromTopic(const std::string& topic) {
    auto broker = MCPBroker::getInstance();
    if (!broker) {
        return false;
    }
    
    // Create a shared_ptr to this
    auto selfPtr = std::dynamic_pointer_cast<IMCPSubscriber_V1>(rack::Module::shared_from_this());
    if (!selfPtr) {
        std::cerr << "Failed to get shared_ptr to subscriber for unsubscription" << std::endl;
        return false;
    }
    
    // Check if subscribed
    auto it = std::find(m_subscribedTopics.begin(), m_subscribedTopics.end(), topic);
    if (it == m_subscribedTopics.end()) {
        return false;  // Not subscribed
    }
    
    // Unsubscribe
    if (broker->unsubscribe(topic, selfPtr)) {
        m_subscribedTopics.erase(it);
        std::cout << "Subscriber " << getId() << " unsubscribed from topic: " << topic << std::endl;
        return true;
    }
    
    return false;
}

} // namespace mcp 