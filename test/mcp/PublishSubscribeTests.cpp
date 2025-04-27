#include "mcp/MCPBroker.h"
#include "mcp/MCPMessage_V1.h"
#include "mcp/MCPSerialization.h"
#include <gtest/gtest.h>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <future>
#include <atomic>

namespace mcp {
namespace test {

// Test subscriber that keeps track of received messages
class TestSubscriber : public IMCPSubscriber_V1 {
public:
    TestSubscriber() : m_messageReceived(false) {}
    
    void onMCPMessage(const MCPMessage_V1* message) override {
        // Store received message
        if (message) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_receivedMessages.push_back(std::make_shared<MCPMessage_V1>(
                message->topic,
                message->senderModuleId,
                message->dataFormat,
                message->data,
                message->dataSize
            ));
            m_messageReceived = true;
            m_cv.notify_all();
        }
    }
    
    // Wait for a message to be received with timeout
    bool waitForMessage(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, timeout, [this] { return hasReceivedMessage(); });
    }
    
    // Check if a message has been received
    bool hasReceivedMessage() const {
        return m_messageReceived;
    }
    
    // Get the number of received messages
    size_t getMessageCount() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_receivedMessages.size();
    }
    
    // Get the received messages
    std::vector<std::shared_ptr<MCPMessage_V1>> getReceivedMessages() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_receivedMessages;
    }
    
    // Reset the received messages
    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_receivedMessages.clear();
        m_messageReceived = false;
    }
    
private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<std::shared_ptr<MCPMessage_V1>> m_receivedMessages;
    std::atomic<bool> m_messageReceived;
};

// Test subscriber that throws an exception in onMCPMessage
class ThrowingSubscriber : public IMCPSubscriber_V1 {
public:
    void onMCPMessage(const MCPMessage_V1* message) override {
        throw std::runtime_error("Test exception in subscriber");
    }
};

// Test fixture
class PublishSubscribeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get a fresh broker instance for each test
        broker = getMCPBroker();
        
        // Create test subscribers
        subscriber1 = std::make_shared<TestSubscriber>();
        subscriber2 = std::make_shared<TestSubscriber>();
        throwingSubscriber = std::make_shared<ThrowingSubscriber>();
        
        // Create test topics
        testTopic1 = "test/topic1";
        testTopic2 = "test/topic2";
    }
    
    void TearDown() override {
        // Clean up subscriptions
        broker->unsubscribeAll(subscriber1);
        broker->unsubscribeAll(subscriber2);
        broker->unsubscribeAll(throwingSubscriber);
    }
    
    std::shared_ptr<IMCPBroker> broker;
    std::shared_ptr<TestSubscriber> subscriber1;
    std::shared_ptr<TestSubscriber> subscriber2;
    std::shared_ptr<ThrowingSubscriber> throwingSubscriber;
    std::string testTopic1;
    std::string testTopic2;
};

// Test basic publish/subscribe functionality
TEST_F(PublishSubscribeTest, BasicPublishSubscribe) {
    // Subscribe to a topic
    ASSERT_TRUE(broker->subscribe(testTopic1, subscriber1));
    
    // Create and publish a message
    std::string testData = "Hello, MCP!";
    std::size_t dataSize = 0;
    auto msgData = serialization::serializeToMsgPack(testData, dataSize);
    
    auto message = std::make_shared<MCPMessage_V1>(
        testTopic1, 1, DataFormat::MSGPACK, msgData, dataSize
    );
    
    ASSERT_TRUE(broker->publish(message));
    
    // Wait for the message to be delivered
    ASSERT_TRUE(subscriber1->waitForMessage());
    
    // Verify that the message was received
    ASSERT_EQ(1, subscriber1->getMessageCount());
    auto receivedMessages = subscriber1->getReceivedMessages();
    ASSERT_EQ(testTopic1, receivedMessages[0]->topic);
    ASSERT_EQ(1, receivedMessages[0]->senderModuleId);
    ASSERT_EQ(DataFormat::MSGPACK, receivedMessages[0]->dataFormat);
    
    // Verify message content
    std::string extractedData = serialization::deserializeFromMsgPack<std::string>(
        receivedMessages[0]->data.get(), receivedMessages[0]->dataSize
    );
    ASSERT_EQ(testData, extractedData);
}

// Test multiple subscribers receiving the same message
TEST_F(PublishSubscribeTest, MultipleSubscribers) {
    // Subscribe both subscribers to the same topic
    ASSERT_TRUE(broker->subscribe(testTopic1, subscriber1));
    ASSERT_TRUE(broker->subscribe(testTopic1, subscriber2));
    
    // Create and publish a message
    auto message = serialization::createMsgPackMessage(
        testTopic1, 1, std::string("Message for multiple subscribers")
    );
    
    ASSERT_TRUE(broker->publish(message));
    
    // Wait for both subscribers to receive the message
    ASSERT_TRUE(subscriber1->waitForMessage());
    ASSERT_TRUE(subscriber2->waitForMessage());
    
    // Verify that both subscribers received the message
    ASSERT_EQ(1, subscriber1->getMessageCount());
    ASSERT_EQ(1, subscriber2->getMessageCount());
}

// Test that messages are only delivered to subscribers of the specific topic
TEST_F(PublishSubscribeTest, TopicFiltering) {
    // Subscribe to different topics
    ASSERT_TRUE(broker->subscribe(testTopic1, subscriber1));
    ASSERT_TRUE(broker->subscribe(testTopic2, subscriber2));
    
    // Publish to topic1
    auto message1 = serialization::createMsgPackMessage(
        testTopic1, 1, std::string("Message for topic1")
    );
    
    ASSERT_TRUE(broker->publish(message1));
    
    // Wait for subscriber1 to receive the message
    ASSERT_TRUE(subscriber1->waitForMessage());
    
    // Verify that only subscriber1 received the message
    ASSERT_EQ(1, subscriber1->getMessageCount());
    ASSERT_EQ(0, subscriber2->getMessageCount());
    
    // Reset subscriber1
    subscriber1->reset();
    
    // Publish to topic2
    auto message2 = serialization::createMsgPackMessage(
        testTopic2, 1, std::string("Message for topic2")
    );
    
    ASSERT_TRUE(broker->publish(message2));
    
    // Wait for subscriber2 to receive the message
    ASSERT_TRUE(subscriber2->waitForMessage());
    
    // Verify that only subscriber2 received the message
    ASSERT_EQ(0, subscriber1->getMessageCount());
    ASSERT_EQ(1, subscriber2->getMessageCount());
}

// Test unsubscribing
TEST_F(PublishSubscribeTest, Unsubscribe) {
    // Subscribe to a topic
    ASSERT_TRUE(broker->subscribe(testTopic1, subscriber1));
    
    // Publish first message
    auto message1 = serialization::createMsgPackMessage(
        testTopic1, 1, std::string("First message")
    );
    
    ASSERT_TRUE(broker->publish(message1));
    
    // Wait for subscriber to receive the message
    ASSERT_TRUE(subscriber1->waitForMessage());
    ASSERT_EQ(1, subscriber1->getMessageCount());
    
    // Unsubscribe
    ASSERT_TRUE(broker->unsubscribe(testTopic1, subscriber1));
    
    // Reset subscriber
    subscriber1->reset();
    
    // Publish second message
    auto message2 = serialization::createMsgPackMessage(
        testTopic1, 1, std::string("Second message")
    );
    
    ASSERT_TRUE(broker->publish(message2));
    
    // Verify that the subscriber did not receive the second message
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give time for delivery if it would happen
    ASSERT_EQ(0, subscriber1->getMessageCount());
}

// Test that exceptions in subscribers don't affect other subscribers
TEST_F(PublishSubscribeTest, ExceptionHandling) {
    // Subscribe both normal and throwing subscribers
    ASSERT_TRUE(broker->subscribe(testTopic1, subscriber1));
    ASSERT_TRUE(broker->subscribe(testTopic1, throwingSubscriber));
    ASSERT_TRUE(broker->subscribe(testTopic1, subscriber2));
    
    // Create and publish a message
    auto message = serialization::createMsgPackMessage(
        testTopic1, 1, std::string("Message with exception handling")
    );
    
    ASSERT_TRUE(broker->publish(message));
    
    // Wait for both normal subscribers to receive the message
    ASSERT_TRUE(subscriber1->waitForMessage());
    ASSERT_TRUE(subscriber2->waitForMessage());
    
    // Verify that both normal subscribers received the message despite the exception
    ASSERT_EQ(1, subscriber1->getMessageCount());
    ASSERT_EQ(1, subscriber2->getMessageCount());
}

// Test concurrent publishing
TEST_F(PublishSubscribeTest, ConcurrentPublishing) {
    const int numThreads = 4;
    const int messagesPerThread = 25;
    
    // Subscribe to the test topic
    ASSERT_TRUE(broker->subscribe(testTopic1, subscriber1));
    
    // Start multiple threads for publishing
    std::vector<std::future<void>> futures;
    for (int t = 0; t < numThreads; ++t) {
        futures.push_back(std::async(std::launch::async, [this, t, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                auto message = serialization::createMsgPackMessage(
                    testTopic1, t, std::string("Message " + std::to_string(t) + "-" + std::to_string(i))
                );
                broker->publish(message);
            }
        }));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    // Wait for all messages to be delivered (might need a longer timeout)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verify that all messages were received
    ASSERT_EQ(numThreads * messagesPerThread, subscriber1->getMessageCount());
}

} // namespace test
} // namespace mcp 