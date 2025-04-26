#include "mcp/IMCPBroker.h"
#include "mcp/IMCPProvider_V1.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Simple example provider class
class ExampleProvider : public mcp::IMCPProvider_V1 {
public:
    ExampleProvider(const std::string& providerName, const std::vector<std::string>& topics)
        : m_name(providerName), m_topics(topics) {}
    
    // Implementation of IMCPProvider_V1
    std::vector<std::string> getProvidedTopics() const override {
        return m_topics;
    }
    
    // Additional method for example
    const std::string& getName() const {
        return m_name;
    }

private:
    std::string m_name;
    std::vector<std::string> m_topics;
};

// Helper function to print available topics
void printAvailableTopics(const std::shared_ptr<mcp::IMCPBroker>& broker) {
    std::cout << "Available topics:" << std::endl;
    auto topics = broker->getAvailableTopics();
    if (topics.empty()) {
        std::cout << "  (none)" << std::endl;
    } else {
        for (const auto& topic : topics) {
            std::cout << "  " << topic << std::endl;
        }
    }
    std::cout << std::endl;
}

// Helper function to print providers for a topic
void printProviders(const std::shared_ptr<mcp::IMCPBroker>& broker, const std::string& topic) {
    std::cout << "Providers for topic '" << topic << "':" << std::endl;
    auto providers = broker->findProviders(topic);
    if (providers.empty()) {
        std::cout << "  (none)" << std::endl;
    } else {
        for (const auto& provider : providers) {
            auto exampleProvider = std::dynamic_pointer_cast<ExampleProvider>(provider);
            if (exampleProvider) {
                std::cout << "  " << exampleProvider->getName() << std::endl;
            } else {
                std::cout << "  (unknown provider)" << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "MCP Broker Registration Example" << std::endl;
    std::cout << "===============================" << std::endl << std::endl;
    
    // Get the broker instance
    auto broker = mcp::getMCPBroker();
    std::cout << "Got broker instance, version: " << broker->getVersion() << std::endl << std::endl;
    
    // Print initial state
    printAvailableTopics(broker);
    
    // Create some example providers
    auto provider1 = std::make_shared<ExampleProvider>(
        "Music Provider", 
        std::vector<std::string>{"music/key", "music/tempo", "music/chords"}
    );
    
    auto provider2 = std::make_shared<ExampleProvider>(
        "Sequence Provider", 
        std::vector<std::string>{"sequence/notes", "sequence/pattern", "music/key"}
    );
    
    auto provider3 = std::make_shared<ExampleProvider>(
        "Visual Provider", 
        std::vector<std::string>{"visual/color", "visual/intensity"}
    );
    
    // Register providers with the broker
    std::cout << "Registering providers..." << std::endl;
    
    for (const auto& topic : provider1->getProvidedTopics()) {
        bool success = broker->registerContext(topic, provider1);
        std::cout << "  Registered '" << topic << "' by " << provider1->getName() 
                  << ": " << (success ? "success" : "failed") << std::endl;
    }
    
    for (const auto& topic : provider2->getProvidedTopics()) {
        bool success = broker->registerContext(topic, provider2);
        std::cout << "  Registered '" << topic << "' by " << provider2->getName() 
                  << ": " << (success ? "success" : "failed") << std::endl;
    }
    
    for (const auto& topic : provider3->getProvidedTopics()) {
        bool success = broker->registerContext(topic, provider3);
        std::cout << "  Registered '" << topic << "' by " << provider3->getName() 
                  << ": " << (success ? "success" : "failed") << std::endl;
    }
    std::cout << std::endl;
    
    // Print state after registration
    printAvailableTopics(broker);
    
    // Print providers for specific topics
    printProviders(broker, "music/key");
    printProviders(broker, "visual/color");
    printProviders(broker, "unknown/topic");
    
    // Unregister a topic
    std::cout << "Unregistering 'music/key' from Music Provider..." << std::endl;
    broker->unregisterContext("music/key", provider1);
    std::cout << std::endl;
    
    // Print providers after unregistration
    printProviders(broker, "music/key");
    
    // Unregister a provider completely
    std::cout << "Unregistering all topics from Visual Provider..." << std::endl;
    for (const auto& topic : provider3->getProvidedTopics()) {
        broker->unregisterContext(topic, provider3);
    }
    std::cout << std::endl;
    
    // Print available topics after unregistration
    printAvailableTopics(broker);
    
    return 0;
} 