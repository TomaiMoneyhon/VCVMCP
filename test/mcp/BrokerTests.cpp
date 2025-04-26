#include "mcp/IMCPBroker.h"
#include "mcp/IMCPProvider_V1.h"
#include "mcp/IMCPSubscriber_V1.h"
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>
#include <thread>

namespace {

// Test provider implementation
class TestProvider : public mcp::IMCPProvider_V1 {
public:
    explicit TestProvider(const std::vector<std::string>& topics)
        : m_topics(topics) {}

    std::vector<std::string> getProvidedTopics() const override {
        return m_topics;
    }

private:
    std::vector<std::string> m_topics;
};

// Test subscriber implementation
class TestSubscriber : public mcp::IMCPSubscriber_V1 {
public:
    TestSubscriber() : m_messageCount(0) {}

    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        // Just count messages for now (messages not implemented yet)
        m_messageCount++;
    }

    int getMessageCount() const {
        return m_messageCount;
    }

private:
    int m_messageCount;
};

} // anonymous namespace

// Test fixture for broker tests
class BrokerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get a clean broker instance for each test
        broker = mcp::getMCPBroker();
        
        // Create some test providers
        provider1 = std::make_shared<TestProvider>(
            std::vector<std::string>{"test/topic1", "test/topic2"}
        );
        
        provider2 = std::make_shared<TestProvider>(
            std::vector<std::string>{"test/topic2", "test/topic3"}
        );

        // Create some test subscribers
        subscriber1 = std::make_shared<TestSubscriber>();
        subscriber2 = std::make_shared<TestSubscriber>();
    }

    std::shared_ptr<mcp::IMCPBroker> broker;
    std::shared_ptr<TestProvider> provider1;
    std::shared_ptr<TestProvider> provider2;
    std::shared_ptr<TestSubscriber> subscriber1;
    std::shared_ptr<TestSubscriber> subscriber2;
};

// Test API version is correct
TEST_F(BrokerTest, VersionIsCorrect) {
    EXPECT_EQ(1, broker->getVersion());
}

// Test registration functionality
TEST_F(BrokerTest, RegisterContext) {
    // Initial state - no topics
    EXPECT_TRUE(broker->getAvailableTopics().empty());
    
    // Register first provider's topics
    EXPECT_TRUE(broker->registerContext("test/topic1", provider1));
    EXPECT_TRUE(broker->registerContext("test/topic2", provider1));
    
    // Verify topics are registered
    auto topics = broker->getAvailableTopics();
    EXPECT_EQ(2, topics.size());
    EXPECT_TRUE(std::find(topics.begin(), topics.end(), "test/topic1") != topics.end());
    EXPECT_TRUE(std::find(topics.begin(), topics.end(), "test/topic2") != topics.end());
    
    // Verify providers are findable
    auto providers = broker->findProviders("test/topic1");
    EXPECT_EQ(1, providers.size());
    EXPECT_EQ(provider1, providers[0]);
    
    // Register second provider for an overlapping topic
    EXPECT_TRUE(broker->registerContext("test/topic2", provider2));
    EXPECT_TRUE(broker->registerContext("test/topic3", provider2));
    
    // Verify all topics are registered
    topics = broker->getAvailableTopics();
    EXPECT_EQ(3, topics.size());
    
    // Verify multiple providers for shared topic
    providers = broker->findProviders("test/topic2");
    EXPECT_EQ(2, providers.size());
    
    // Test registration edge cases
    EXPECT_FALSE(broker->registerContext("", provider1)); // Empty topic
    EXPECT_FALSE(broker->registerContext("test/topic1", nullptr)); // Null provider
    EXPECT_FALSE(broker->registerContext("test/topic1", provider1)); // Already registered
}

// Test unregistration functionality
TEST_F(BrokerTest, UnregisterContext) {
    // Setup - register providers
    broker->registerContext("test/topic1", provider1);
    broker->registerContext("test/topic2", provider1);
    broker->registerContext("test/topic2", provider2);
    broker->registerContext("test/topic3", provider2);
    
    // Verify initial state
    EXPECT_EQ(3, broker->getAvailableTopics().size());
    
    // Unregister a topic with only one provider
    EXPECT_TRUE(broker->unregisterContext("test/topic1", provider1));
    
    // Verify topic is removed
    auto topics = broker->getAvailableTopics();
    EXPECT_EQ(2, topics.size());
    EXPECT_TRUE(std::find(topics.begin(), topics.end(), "test/topic1") == topics.end());
    
    // Unregister one provider from a shared topic
    EXPECT_TRUE(broker->unregisterContext("test/topic2", provider1));
    
    // Verify topic still exists with one provider
    topics = broker->getAvailableTopics();
    EXPECT_EQ(2, topics.size());
    auto providers = broker->findProviders("test/topic2");
    EXPECT_EQ(1, providers.size());
    EXPECT_EQ(provider2, providers[0]);
    
    // Test unregistration edge cases
    EXPECT_FALSE(broker->unregisterContext("", provider1)); // Empty topic
    EXPECT_FALSE(broker->unregisterContext("test/topic2", nullptr)); // Null provider
    EXPECT_FALSE(broker->unregisterContext("test/topic1", provider1)); // Not registered
}

// Test subscription functionality
TEST_F(BrokerTest, Subscribe) {
    // Setup - register providers
    broker->registerContext("test/topic1", provider1);
    broker->registerContext("test/topic2", provider1);
    broker->registerContext("test/topic2", provider2);
    
    // Subscribe to topics
    EXPECT_TRUE(broker->subscribe("test/topic1", subscriber1));
    EXPECT_TRUE(broker->subscribe("test/topic2", subscriber1));
    EXPECT_TRUE(broker->subscribe("test/topic2", subscriber2));
    
    // Test subscription edge cases
    EXPECT_FALSE(broker->subscribe("", subscriber1)); // Empty topic
    EXPECT_FALSE(broker->subscribe("test/topic1", nullptr)); // Null subscriber
    EXPECT_FALSE(broker->subscribe("test/topic1", subscriber1)); // Already subscribed
}

// Test unsubscribe functionality
TEST_F(BrokerTest, Unsubscribe) {
    // Setup - register providers and subscribe
    broker->registerContext("test/topic1", provider1);
    broker->registerContext("test/topic2", provider1);
    
    broker->subscribe("test/topic1", subscriber1);
    broker->subscribe("test/topic2", subscriber1);
    broker->subscribe("test/topic2", subscriber2);
    
    // Unsubscribe from one topic
    EXPECT_TRUE(broker->unsubscribe("test/topic1", subscriber1));
    
    // Unsubscribe from shared topic
    EXPECT_TRUE(broker->unsubscribe("test/topic2", subscriber1));
    
    // Test unsubscribe edge cases
    EXPECT_FALSE(broker->unsubscribe("", subscriber1)); // Empty topic
    EXPECT_FALSE(broker->unsubscribe("test/topic1", nullptr)); // Null subscriber
    EXPECT_FALSE(broker->unsubscribe("test/topic1", subscriber1)); // Not subscribed
}

// Test unsubscribeAll functionality
TEST_F(BrokerTest, UnsubscribeAll) {
    // Setup - register providers and subscribe to multiple topics
    broker->registerContext("test/topic1", provider1);
    broker->registerContext("test/topic2", provider1);
    broker->registerContext("test/topic3", provider2);
    
    broker->subscribe("test/topic1", subscriber1);
    broker->subscribe("test/topic2", subscriber1);
    broker->subscribe("test/topic2", subscriber2);
    broker->subscribe("test/topic3", subscriber1);
    
    // Unsubscribe subscriber1 from all topics
    EXPECT_TRUE(broker->unsubscribeAll(subscriber1));
    
    // subscriber2 should still be subscribed to topic2
    EXPECT_TRUE(broker->unsubscribe("test/topic2", subscriber2));
    
    // Calling unsubscribeAll again should return false (no subscriptions)
    EXPECT_FALSE(broker->unsubscribeAll(subscriber1));
    
    // Test edge case
    EXPECT_FALSE(broker->unsubscribeAll(nullptr)); // Null subscriber
}

// Test weak reference behavior
TEST_F(BrokerTest, WeakReferenceHandling) {
    // Setup - register provider
    broker->registerContext("test/topic1", provider1);
    
    // Verify provider is found
    EXPECT_EQ(1, broker->findProviders("test/topic1").size());
    
    // Reset provider (releasing the shared_ptr)
    provider1.reset();
    
    // Verify provider is automatically cleared
    EXPECT_TRUE(broker->findProviders("test/topic1").empty());
    
    // Topic should also be removed when no active providers
    EXPECT_TRUE(broker->getAvailableTopics().empty());
}

// Test weak reference behavior for subscribers
TEST_F(BrokerTest, SubscriberWeakReferenceHandling) {
    // Setup - register provider and subscriber
    broker->registerContext("test/topic1", provider1);
    broker->subscribe("test/topic1", subscriber1);
    
    // Reset subscriber (releasing the shared_ptr)
    std::weak_ptr<TestSubscriber> weakSubscriber = subscriber1;
    subscriber1.reset();
    
    // The weak reference in the broker should be automatically cleaned up
    // when attempting to publish (not implemented yet in Sprint 2)
    // This test will be expanded in Sprint 3
    EXPECT_TRUE(weakSubscriber.expired());
}

// Test thread safety for concurrent registration/unregistration
TEST_F(BrokerTest, ThreadSafety) {
    const int numThreads = 10;
    const int numOperationsPerThread = 100;
    
    std::vector<std::shared_ptr<TestProvider>> providers;
    for (int i = 0; i < numThreads; i++) {
        providers.push_back(std::make_shared<TestProvider>(
            std::vector<std::string>{
                "thread/topic" + std::to_string(i),
                "shared/topic"
            }
        ));
    }
    
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(std::thread([this, i, &providers, numOperationsPerThread]() {
            auto& provider = providers[i];
            std::string topic = "thread/topic" + std::to_string(i);
            
            for (int j = 0; j < numOperationsPerThread; j++) {
                // Register
                broker->registerContext(topic, provider);
                broker->registerContext("shared/topic", provider);
                
                // Find
                broker->getAvailableTopics();
                broker->findProviders(topic);
                broker->findProviders("shared/topic");
                
                // Unregister (every other iteration)
                if (j % 2 == 0) {
                    broker->unregisterContext(topic, provider);
                    broker->unregisterContext("shared/topic", provider);
                }
            }
        }));
    }
    
    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // If we got here without crashing or deadlocking, the test is considered passed
    // Additionally, check that registry is in a reasonable state
    auto topics = broker->getAvailableTopics();
    
    // Check each provider's assigned topic
    for (int i = 0; i < numThreads; i++) {
        std::string topic = "thread/topic" + std::to_string(i);
        
        // If the topic exists, the provider should be registered
        if (std::find(topics.begin(), topics.end(), topic) != topics.end()) {
            auto foundProviders = broker->findProviders(topic);
            EXPECT_TRUE(std::find(foundProviders.begin(), foundProviders.end(), 
                                  providers[i]) != foundProviders.end());
        }
    }
}

// Test thread safety for concurrent subscriptions
TEST_F(BrokerTest, SubscriptionThreadSafety) {
    const int numThreads = 10;
    const int numOperationsPerThread = 100;
    
    // Setup - register a topic
    broker->registerContext("concurrent/topic", provider1);
    
    std::vector<std::shared_ptr<TestSubscriber>> subscribers;
    for (int i = 0; i < numThreads; i++) {
        subscribers.push_back(std::make_shared<TestSubscriber>());
    }
    
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(std::thread([this, i, &subscribers, numOperationsPerThread]() {
            auto& subscriber = subscribers[i];
            
            for (int j = 0; j < numOperationsPerThread; j++) {
                // Subscribe
                broker->subscribe("concurrent/topic", subscriber);
                
                // Unsubscribe (every other iteration)
                if (j % 2 == 0) {
                    broker->unsubscribe("concurrent/topic", subscriber);
                }
            }
            
            // Final unsubscribe
            broker->unsubscribeAll(subscriber);
        }));
    }
    
    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // If we got here without crashing or deadlocking, the test is considered passed
} 