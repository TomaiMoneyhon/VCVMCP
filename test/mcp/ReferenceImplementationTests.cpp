#include <gtest/gtest.h>
#include "mcp/MCPBroker.h"
#include "mcp/MCPReferenceProvider.h"
#include "mcp/MCPReferenceSubscriber.h"
#include <memory>
#include <thread>
#include <chrono>

class ReferenceImplementationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get broker instance
        m_broker = mcp::MCPBroker::getInstance();
        ASSERT_TRUE(m_broker != nullptr);
    }

    void TearDown() override {
        // Cleanup any remaining resources
        m_broker.reset();
    }

    std::shared_ptr<mcp::MCPBroker> m_broker;
};

// Test provider and subscriber can be initialized
TEST_F(ReferenceImplementationTest, Initialization) {
    // Create provider and subscriber on the stack to avoid issues with shared_from_this()
    mcp::MCPReferenceProvider provider(1001);
    mcp::MCPReferenceSubscriber subscriber(2001);
    
    // Check initial state
    EXPECT_EQ(subscriber.getParameter(1), 0.0f);
    EXPECT_EQ(subscriber.getParameter(2), 0.0f);
    
    // Check topic list
    auto topics = provider.getProvidedTopics();
    ASSERT_GE(topics.size(), 1);
    EXPECT_EQ(topics[0], "reference/parameter1");
}

// Test provider can register and unregister topics
TEST_F(ReferenceImplementationTest, ProviderRegistration) {
    // Create shared_ptr properly to work with shared_from_this()
    auto provider = std::make_shared<mcp::MCPReferenceProvider>(1001);
    
    // Add provider to the rack (registers topics)
    provider->onAdd();
    
    // Check that the topics are registered
    auto topics = m_broker->getAvailableTopics();
    ASSERT_GE(topics.size(), 1);
    EXPECT_TRUE(std::find(topics.begin(), topics.end(), "reference/parameter1") != topics.end());
    
    // Find providers for a topic
    auto providers = m_broker->findProviders("reference/parameter1");
    ASSERT_GE(providers.size(), 1);
    
    // Remove provider (unregisters topics)
    provider->onRemove();
    
    // Wait a moment for unregistration to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // The topics should no longer be available from this provider
    providers = m_broker->findProviders("reference/parameter1");
    for (const auto& p : providers) {
        if (p == provider) {
            FAIL() << "Provider still registered after onRemove";
        }
    }
}

// Test subscriber can subscribe and unsubscribe
TEST_F(ReferenceImplementationTest, SubscriberSubscription) {
    // Create shared_ptr properly to work with shared_from_this()
    auto subscriber = std::make_shared<mcp::MCPReferenceSubscriber>(2001);
    
    // Add subscriber to the rack (subscribes to topics)
    subscriber->onAdd();
    
    // Add test subscription to a specific topic
    EXPECT_TRUE(subscriber->subscribeToTopic("test/topic"));
    
    // Unsubscribe from the test topic
    EXPECT_TRUE(subscriber->unsubscribeFromTopic("test/topic"));
    
    // Remove subscriber (unsubscribes from all topics)
    subscriber->onRemove();
    
    // No direct way to verify subscription was removed, but we can check no errors occurred
    SUCCEED();
}

// Test basic message passing
TEST_F(ReferenceImplementationTest, BasicMessagePassing) {
    // Setup provider and subscriber
    auto provider = std::make_shared<mcp::MCPReferenceProvider>(1001);
    auto subscriber = std::make_shared<mcp::MCPReferenceSubscriber>(2001);
    
    // Start subscriber first
    subscriber->onAdd();
    
    // Then start provider and enable publishing
    provider->onAdd();
    provider->startPeriodicPublishing(50); // Fast publishing for test
    
    // Let them run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Process messages in the subscriber (we need a buffer for process)
    const int BUFFER_SIZE = 256;
    float buffer[BUFFER_SIZE];
    subscriber->process(buffer, BUFFER_SIZE);
    
    // Should have received some parameter values
    EXPECT_NE(subscriber->getParameter(1), 0.0f);
    
    // Stop both
    provider->stopPeriodicPublishing();
    provider->onRemove();
    subscriber->onRemove();
}

// Test thread-safe communication
TEST_F(ReferenceImplementationTest, ThreadSafeCommunication) {
    // Setup provider and subscriber with fast publishing
    auto provider = std::make_shared<mcp::MCPReferenceProvider>(1001);
    auto subscriber = std::make_shared<mcp::MCPReferenceSubscriber>(2001);
    
    // Start both
    subscriber->onAdd();
    provider->onAdd();
    provider->startPeriodicPublishing(50); // Fast publishing every 50ms
    
    // Run a separate thread that simulates the audio processing
    std::atomic<bool> running(true);
    std::atomic<int> processedCount(0);
    
    auto processingThread = std::thread([&]() {
        // Set as audio thread
        rack::engine::setThreadType(rack::engine::AUDIO_THREAD);
        
        // Buffer for processing
        const int BUFFER_SIZE = 256;
        float buffer[BUFFER_SIZE];
        
        while (running.load()) {
            // Process messages
            subscriber->process(buffer, BUFFER_SIZE);
            processedCount.fetch_add(1);
            
            // Simulate audio thread timing (around 5.8ms for 256 samples at 44.1kHz)
            std::this_thread::sleep_for(std::chrono::milliseconds(6));
        }
    });
    
    // Let them run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Stop processing thread
    running.store(false);
    processingThread.join();
    
    // Stop provider and subscriber
    provider->stopPeriodicPublishing();
    provider->onRemove();
    subscriber->onRemove();
    
    // Should have processed some messages
    EXPECT_GT(processedCount.load(), 0);
    EXPECT_NE(subscriber->getParameter(1), 0.0f);
    
    // Should have processed approximately the right number of messages
    // This is approximate because timing can vary
    int expectedProcessCycles = 500 / 6; // Time / audio processing interval
    int tolerance = expectedProcessCycles / 2; // Allow for significant timing variation in tests
    EXPECT_NEAR(processedCount.load(), expectedProcessCycles, tolerance);
} 