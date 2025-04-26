#include "mcp/MCPMessage_V1.h"
#include "mcp/MCPSerialization.h"
#include "../../external/msgpack11/msgpack11.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>

// Test fixture for serialization tests
class SerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create sample data
        simpleString = "Hello, MCP!";
        simpleInt = 42;
        simpleDouble = 3.14159;
        stringArray = {"one", "two", "three"};
        floatArray = {1.1f, 2.2f, 3.3f, 4.4f};
    }
    
    // Simple test data
    std::string simpleString;
    int simpleInt;
    double simpleDouble;
    std::vector<std::string> stringArray;
    std::vector<float> floatArray;
};

// Test the MCPMessage_V1 structure
TEST_F(SerializationTest, MessageStructure) {
    // Create a basic message with a string payload
    std::string payload = "test payload";
    std::shared_ptr<void> data = std::make_shared<std::string>(payload);
    std::size_t dataSize = payload.size();
    
    mcp::MCPMessage_V1 msg("test/topic", 1, "text/plain", data, dataSize);
    
    // Check message properties
    EXPECT_EQ("test/topic", msg.topic);
    EXPECT_EQ(1, msg.senderModuleId);
    EXPECT_EQ("text/plain", msg.dataFormat);
    EXPECT_EQ(dataSize, msg.dataSize);
    EXPECT_EQ(data, msg.data);
}

// Test MessagePack serialization with simple string
TEST_F(SerializationTest, MsgPackString) {
    // Serialize
    std::size_t dataSize = 0;
    std::shared_ptr<void> data = mcp::serialization::serializeToMsgPack<std::string>(simpleString, dataSize);
    
    // Verify serialization worked
    EXPECT_GT(dataSize, 0);
    EXPECT_NE(nullptr, data);
    
    // Deserialize
    std::string result = mcp::serialization::deserializeFromMsgPack<std::string>(data.get(), dataSize);
    
    // Verify deserialization matches original
    EXPECT_EQ(simpleString, result);
}

// Test MessagePack serialization with integers
TEST_F(SerializationTest, MsgPackInt) {
    // Serialize
    std::size_t dataSize = 0;
    std::shared_ptr<void> data = mcp::serialization::serializeToMsgPack<int>(simpleInt, dataSize);
    
    // Verify serialization worked
    EXPECT_GT(dataSize, 0);
    EXPECT_NE(nullptr, data);
    
    // Deserialize
    int result = mcp::serialization::deserializeFromMsgPack<int>(data.get(), dataSize);
    
    // Verify deserialization matches original
    EXPECT_EQ(simpleInt, result);
}

// Test MessagePack serialization with arrays
TEST_F(SerializationTest, MsgPackArray) {
    // Serialize
    std::size_t dataSize = 0;
    std::shared_ptr<void> data = mcp::serialization::serializeToMsgPack<std::vector<std::string>>(stringArray, dataSize);
    
    // Verify serialization worked
    EXPECT_GT(dataSize, 0);
    EXPECT_NE(nullptr, data);
    
    // Deserialize
    auto result = mcp::serialization::deserializeFromMsgPack<std::vector<std::string>>(data.get(), dataSize);
    
    // Verify deserialization matches original
    EXPECT_EQ(stringArray.size(), result.size());
    for (size_t i = 0; i < stringArray.size(); i++) {
        EXPECT_EQ(stringArray[i], result[i]);
    }
}

// Test message creation helpers
TEST_F(SerializationTest, MessageCreationHelpers) {
    // Create message with MessagePack
    std::shared_ptr<mcp::MCPMessage_V1> msgPackMessage = 
        mcp::serialization::createMsgPackMessage<std::string>("test/topic", 1, simpleString);
    
    // Verify message properties
    EXPECT_EQ("test/topic", msgPackMessage->topic);
    EXPECT_EQ(1, msgPackMessage->senderModuleId);
    EXPECT_EQ(mcp::DataFormat::MSGPACK, msgPackMessage->dataFormat);
    EXPECT_GT(msgPackMessage->dataSize, 0);
    EXPECT_NE(nullptr, msgPackMessage->data);
}

// Test extractMessageData helper
TEST_F(SerializationTest, ExtractMessageData) {
    // Create message with MessagePack
    std::shared_ptr<mcp::MCPMessage_V1> msgPackMessage = 
        mcp::serialization::createMsgPackMessage<std::string>("test/topic", 1, simpleString);
    
    // Extract data from MessagePack message
    std::string msgPackResult = mcp::serialization::extractMessageData<std::string>(msgPackMessage.get());
    
    // Verify extracted data
    EXPECT_EQ(simpleString, msgPackResult);
}

// Test error handling in serialization
TEST_F(SerializationTest, SerializationErrors) {
    // Test deserialization of null data
    EXPECT_THROW(
        mcp::serialization::deserializeFromMsgPack<std::string>(nullptr, 0),
        mcp::MCPSerializationError
    );
    
    // Test extraction from null message
    EXPECT_THROW(
        mcp::serialization::extractMessageData<std::string>(nullptr),
        mcp::MCPSerializationError
    );
    
    // Create a message with invalid format
    mcp::MCPMessage_V1 invalidMessage(
        "test/topic", 1, "invalid/format", nullptr, 0
    );
    
    // Test extraction from message with invalid format
    EXPECT_THROW(
        mcp::serialization::extractMessageData<std::string>(&invalidMessage),
        mcp::MCPSerializationError
    );
    
    // Test extraction from message with no data
    EXPECT_THROW(
        mcp::serialization::extractMessageData<std::string>(&invalidMessage),
        mcp::MCPSerializationError
    );
} 