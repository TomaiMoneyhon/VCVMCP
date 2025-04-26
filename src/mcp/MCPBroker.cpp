#include "mcp/MCPBroker.h"
#include <algorithm>

namespace mcp {

// Initialize static members
std::shared_ptr<MCPBroker> MCPBroker::s_instance = nullptr;
std::mutex MCPBroker::s_instanceMutex;

std::shared_ptr<MCPBroker> MCPBroker::getInstance() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) {
        s_instance = std::make_shared<MCPBroker>();
    }
    return s_instance;
}

MCPBroker::MCPBroker() {
    // Initialization if needed
}

MCPBroker::~MCPBroker() {
    // Cleanup if needed
}

bool MCPBroker::registerContext(const std::string& topic, 
                               std::shared_ptr<IMCPProvider_V1> provider) {
    if (!topic.empty() && provider) {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        
        // Check if the provider is already registered for this topic
        auto& providers = m_topicRegistry[topic];
        for (const auto& weakProvider : providers) {
            if (auto existingProvider = weakProvider.lock()) {
                if (existingProvider == provider) {
                    // Provider already registered for this topic
                    return false;
                }
            }
        }
        
        // Add the provider to the topic
        providers.push_back(provider);
        return true;
    }
    return false;
}

bool MCPBroker::unregisterContext(const std::string& topic, 
                                 std::shared_ptr<IMCPProvider_V1> provider) {
    if (!topic.empty() && provider) {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        
        // Find the topic in the registry
        auto topicIt = m_topicRegistry.find(topic);
        if (topicIt != m_topicRegistry.end()) {
            auto& providers = topicIt->second;
            
            // Find and remove the provider
            auto it = std::remove_if(providers.begin(), providers.end(), 
                [&provider](const std::weak_ptr<IMCPProvider_V1>& weakProvider) {
                    auto existingProvider = weakProvider.lock();
                    return !existingProvider || existingProvider == provider;
                });
            
            if (it != providers.end()) {
                providers.erase(it, providers.end());
                
                // Remove the topic if no providers left
                if (providers.empty()) {
                    m_topicRegistry.erase(topicIt);
                }
                
                return true;
            }
        }
    }
    return false;
}

bool MCPBroker::subscribe(const std::string& topic, 
                         std::shared_ptr<IMCPSubscriber_V1> subscriber) {
    if (!topic.empty() && subscriber) {
        std::lock_guard<std::mutex> lock(m_subscriptionMutex);
        
        // Check if the subscriber is already subscribed to this topic
        auto& subscribers = m_subscriptions[topic];
        for (const auto& weakSubscriber : subscribers) {
            if (auto existingSubscriber = weakSubscriber.lock()) {
                if (existingSubscriber == subscriber) {
                    // Subscriber already registered for this topic
                    return false;
                }
            }
        }
        
        // Add the subscriber to the topic
        subscribers.push_back(subscriber);
        return true;
    }
    return false;
}

bool MCPBroker::unsubscribe(const std::string& topic, 
                           std::shared_ptr<IMCPSubscriber_V1> subscriber) {
    if (!topic.empty() && subscriber) {
        std::lock_guard<std::mutex> lock(m_subscriptionMutex);
        
        // Find the topic in the subscriptions
        auto topicIt = m_subscriptions.find(topic);
        if (topicIt != m_subscriptions.end()) {
            auto& subscribers = topicIt->second;
            
            // Find and remove the subscriber
            auto it = std::remove_if(subscribers.begin(), subscribers.end(), 
                [&subscriber](const std::weak_ptr<IMCPSubscriber_V1>& weakSubscriber) {
                    auto existingSubscriber = weakSubscriber.lock();
                    return !existingSubscriber || existingSubscriber == subscriber;
                });
            
            if (it != subscribers.end()) {
                subscribers.erase(it, subscribers.end());
                
                // Remove the topic if no subscribers left
                if (subscribers.empty()) {
                    m_subscriptions.erase(topicIt);
                }
                
                return true;
            }
        }
    }
    return false;
}

bool MCPBroker::unsubscribeAll(std::shared_ptr<IMCPSubscriber_V1> subscriber) {
    if (!subscriber) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_subscriptionMutex);
    bool unsubscribedAny = false;
    
    // Iterate through all topics
    auto topicIt = m_subscriptions.begin();
    while (topicIt != m_subscriptions.end()) {
        auto& subscribers = topicIt->second;
        
        // Find and remove the subscriber from each topic
        auto it = std::remove_if(subscribers.begin(), subscribers.end(),
            [&subscriber](const std::weak_ptr<IMCPSubscriber_V1>& weakSubscriber) {
                auto existingSubscriber = weakSubscriber.lock();
                return !existingSubscriber || existingSubscriber == subscriber;
            });
        
        if (it != subscribers.end()) {
            unsubscribedAny = true;
            subscribers.erase(it, subscribers.end());
            
            // Remove the topic if no subscribers left
            if (subscribers.empty()) {
                topicIt = m_subscriptions.erase(topicIt);
                continue;
            }
        }
        
        ++topicIt;
    }
    
    return unsubscribedAny;
}

std::vector<std::string> MCPBroker::getAvailableTopics() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    std::vector<std::string> topics;
    
    // Collect non-empty topics with active providers
    for (const auto& entry : m_topicRegistry) {
        bool hasActiveProvider = false;
        for (const auto& weakProvider : entry.second) {
            if (weakProvider.lock()) {
                hasActiveProvider = true;
                break;
            }
        }
        
        if (hasActiveProvider) {
            topics.push_back(entry.first);
        }
    }
    
    return topics;
}

std::vector<std::shared_ptr<IMCPProvider_V1>> MCPBroker::findProviders(
                                    const std::string& topic) const {
    std::vector<std::shared_ptr<IMCPProvider_V1>> result;
    
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    // Find the topic in the registry
    auto topicIt = m_topicRegistry.find(topic);
    if (topicIt != m_topicRegistry.end()) {
        const auto& providers = topicIt->second;
        
        // Collect active providers
        for (const auto& weakProvider : providers) {
            if (auto provider = weakProvider.lock()) {
                result.push_back(provider);
            }
        }
        
        // Clean up expired weak pointers if we found any
        if (result.size() != providers.size()) {
            auto& mutableProviders = const_cast<std::vector<std::weak_ptr<IMCPProvider_V1>>&>(providers);
            mutableProviders.erase(
                std::remove_if(mutableProviders.begin(), mutableProviders.end(),
                    [](const std::weak_ptr<IMCPProvider_V1>& weakProvider) {
                        return weakProvider.expired();
                    }),
                mutableProviders.end()
            );
            
            // Remove the topic if no providers left
            if (mutableProviders.empty()) {
                auto& mutableRegistry = const_cast<ProviderMap&>(m_topicRegistry);
                mutableRegistry.erase(topic);
            }
        }
    }
    
    return result;
}

int MCPBroker::getVersion() const {
    return 1; // V1 implementation
}

// Global accessor function
std::shared_ptr<IMCPBroker> getMCPBroker() {
    return MCPBroker::getInstance();
}

} // namespace mcp 