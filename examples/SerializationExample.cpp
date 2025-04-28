#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

#include "mcp/MCPBroker.h"
#include "mcp/MCPSerialization.h"

// Example module implementing provider
class PresetProvider : public mcp::IMCPProvider_V1, public std::enable_shared_from_this<PresetProvider> {
public:
    PresetProvider(int moduleId)
        : m_moduleId(moduleId) {}
    
    std::vector<std::string> getProvidedTopics() const override {
        return { "synth/presets", "synth/parameters" };
    }

    void publishPresetName(const std::string& preset) {
        auto broker = mcp::MCPBroker::getInstance();
        if (!broker) {
            std::cerr << "Failed to get broker instance" << std::endl;
            return;
        }

        try {
            // Create a message with serialized data
            auto message = mcp::serialization::createMsgPackMessage(
                "synth/presets", 
                m_moduleId, 
                preset
            );

            // Publish the message
            broker->publish(message);
            std::cout << "Published preset: " << preset << std::endl;
        }
        catch (const mcp::MCPSerializationError& e) {
            std::cerr << "Serialization error: " << e.what() << std::endl;
        }
    }

    void publishParameters(const std::vector<float>& parameters) {
        auto broker = mcp::MCPBroker::getInstance();
        if (!broker) {
            std::cerr << "Failed to get broker instance" << std::endl;
            return;
        }

        try {
            // Create a message with serialized data
            auto message = mcp::serialization::createMsgPackMessage(
                "synth/parameters", 
                m_moduleId, 
                parameters
            );

            // Publish the message
            broker->publish(message);
            std::cout << "Published parameters" << std::endl;
        }
        catch (const mcp::MCPSerializationError& e) {
            std::cerr << "Serialization error: " << e.what() << std::endl;
        }
    }

private:
    int m_moduleId;
};

// Example module implementing subscriber
class PresetConsumer : public mcp::IMCPSubscriber_V1, public std::enable_shared_from_this<PresetConsumer> {
public:
    PresetConsumer(int moduleId)
        : m_moduleId(moduleId) {}

    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        std::cout << m_moduleId << " received message from " << message->senderModuleId 
                  << " on topic " << message->topic << std::endl;

        try {
            if (message->topic == "synth/presets") {
                // Deserialize the message as string
                std::string presetName = mcp::serialization::extractMessageData<std::string>(message);
                std::cout << "Received preset name: " << presetName << std::endl;
            } else if (message->topic == "synth/parameters") {
                // Deserialize the message as vector of floats
                std::vector<float> parameters = mcp::serialization::extractMessageData<std::vector<float>>(message);
                
                std::cout << "Received parameters: [";
                for (size_t i = 0; i < parameters.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << parameters[i];
                }
                std::cout << "]" << std::endl;
            }
        }
        catch (const mcp::MCPSerializationError& e) {
            std::cerr << "Deserialization error: " << e.what() << std::endl;
        }
    }

    void initialize() {
        auto broker = mcp::MCPBroker::getInstance();
        if (!broker) {
            std::cerr << "Failed to get broker instance" << std::endl;
            return;
        }

        // Subscribe to preset topics
        broker->subscribe("synth/presets", shared_from_this());
        broker->subscribe("synth/parameters", shared_from_this());
        std::cout << m_moduleId << " subscribed to synth/presets and synth/parameters" << std::endl;
    }

    void cleanup() {
        auto broker = mcp::MCPBroker::getInstance();
        if (!broker) {
            std::cerr << "Failed to get broker instance" << std::endl;
            return;
        }

        // Unsubscribe from preset topics
        broker->unsubscribe("synth/presets", shared_from_this());
        broker->unsubscribe("synth/parameters", shared_from_this());
        std::cout << m_moduleId << " unsubscribed from topics" << std::endl;
    }

private:
    int m_moduleId;
};

int main() {
    std::cout << "MCP Serialization Example" << std::endl;
    std::cout << "=========================" << std::endl;

    // Scope for controlled resource lifetime
    {
        // Get broker instance
        auto broker = mcp::MCPBroker::getInstance();
        if (!broker) {
            std::cerr << "Failed to get broker instance" << std::endl;
            return 1;
        }

        // Create provider and subscriber
        auto provider = std::make_shared<PresetProvider>(1001);  // Using numeric IDs
        auto consumer1 = std::make_shared<PresetConsumer>(2001);
        auto consumer2 = std::make_shared<PresetConsumer>(2002);

        // Register provider for each topic
        broker->registerContext("synth/presets", provider);
        broker->registerContext("synth/parameters", provider);
        std::cout << "Provider registered for topics" << std::endl;
        
        // Initialize subscribers
        consumer1->initialize();
        consumer2->initialize();

        // Create and publish some data
        provider->publishPresetName("Warm Pad");
        
        // Add a small delay to demonstrate async processing
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::vector<float> parameters = {0.5f, 0.3f, 0.8f, 0.2f, 0.9f};
        provider->publishParameters(parameters);
        
        // Another delay
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Clean up
        consumer1->cleanup();
        consumer2->cleanup();
        broker->unregisterContext("synth/presets", provider);
        broker->unregisterContext("synth/parameters", provider);
        std::cout << "Provider unregistered from topics" << std::endl;

        std::cout << "Example completed successfully" << std::endl;
        
        // Release objects in controlled order
        provider.reset();
        consumer1.reset();
        consumer2.reset();
        broker.reset();
    }
    
    // Ensure broker is properly shut down
    mcp::shutdownMCPBroker();
    
    return 0;
} 