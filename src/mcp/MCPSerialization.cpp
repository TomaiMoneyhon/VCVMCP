#include "mcp/MCPSerialization.h"
#include "mcp/MCPMessage_V1.h"

// Include msgpack11
#include "../external/msgpack11/msgpack11.hpp"

#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <stdexcept>

namespace mcp {
namespace serialization {

namespace {
    // Helper function to convert byte vector to shared_ptr<void>
    std::shared_ptr<void> bytesToSharedPtr(const std::vector<uint8_t>& bytes, std::size_t& dataSize) {
        dataSize = bytes.size();
        if (dataSize == 0) {
            return nullptr;
        }
        
        auto data = std::shared_ptr<void>(new uint8_t[dataSize], [](void* p) { delete[] static_cast<uint8_t*>(p); });
        std::memcpy(data.get(), bytes.data(), dataSize);
        return data;
    }
    
    // Helper function to create byte vector from raw data
    std::vector<uint8_t> rawDataToBytes(const void* data, std::size_t dataSize) {
        if (!data || dataSize == 0) {
            return {};
        }
        
        const uint8_t* byteData = static_cast<const uint8_t*>(data);
        return std::vector<uint8_t>(byteData, byteData + dataSize);
    }
}

// MessagePack Serialization
template<typename T>
std::shared_ptr<void> serializeToMsgPack(const T& obj, std::size_t& dataSize) {
    try {
        msgpack11::MsgPack msgpack = convertToMsgPack(obj);
        std::string packed = msgpack.dump();
        dataSize = packed.size();
        
        auto data = std::shared_ptr<void>(new char[dataSize], [](void* p) { delete[] static_cast<char*>(p); });
        std::memcpy(data.get(), packed.data(), dataSize);
        return data;
    } catch (const std::exception& e) {
        throw MCPSerializationError(std::string("MessagePack serialization failed: ") + e.what());
    }
}

// Convert various types to MsgPack (string specialization)
template<>
msgpack11::MsgPack convertToMsgPack<std::string>(const std::string& value) {
    return msgpack11::MsgPack(value);
}

// Convert various types to MsgPack (int specialization)
template<>
msgpack11::MsgPack convertToMsgPack<int>(const int& value) {
    return msgpack11::MsgPack(value);
}

// Convert various types to MsgPack (double specialization)
template<>
msgpack11::MsgPack convertToMsgPack<double>(const double& value) {
    return msgpack11::MsgPack(value);
}

// Convert various types to MsgPack (vector<string> specialization)
template<>
msgpack11::MsgPack convertToMsgPack<std::vector<std::string>>(const std::vector<std::string>& value) {
    msgpack11::MsgPack::array array;
    for (const auto& item : value) {
        array.push_back(msgpack11::MsgPack(item));
    }
    return msgpack11::MsgPack(array);
}

// Convert various types to MsgPack (vector<float> specialization)
template<>
msgpack11::MsgPack convertToMsgPack<std::vector<float>>(const std::vector<float>& value) {
    msgpack11::MsgPack::array array;
    for (const auto& item : value) {
        array.push_back(msgpack11::MsgPack(static_cast<double>(item)));
    }
    return msgpack11::MsgPack(array);
}

// MessagePack Deserialization
template<typename T>
T deserializeFromMsgPack(const void* data, std::size_t dataSize) {
    try {
        if (data == nullptr || dataSize == 0) {
            throw MCPSerializationError("MessagePack deserialization failed: Empty data");
        }
        
        // Create string from the data
        std::string serialized(static_cast<const char*>(data), dataSize);
        
        // Parse MessagePack
        std::string err;
        msgpack11::MsgPack msgpack = msgpack11::MsgPack::parse(serialized, err);
        
        if (!err.empty()) {
            throw MCPSerializationError("MessagePack deserialization failed: " + err);
        }
        
        // Convert from MsgPack to the requested type
        return convertFromMsgPack<T>(msgpack);
    } catch (const std::exception& e) {
        throw MCPSerializationError(std::string("MessagePack deserialization failed: ") + e.what());
    }
}

// Convert from MsgPack to various types (string specialization)
template<>
std::string convertFromMsgPack<std::string>(const msgpack11::MsgPack& msgpack) {
    if (!msgpack.is_string()) {
        throw MCPSerializationError("Expected string type in MessagePack data");
    }
    return msgpack.string_value();
}

// Convert from MsgPack to various types (int specialization)
template<>
int convertFromMsgPack<int>(const msgpack11::MsgPack& msgpack) {
    if (!msgpack.is_int()) {
        throw MCPSerializationError("Expected number type in MessagePack data");
    }
    return msgpack.int_value();
}

// Convert from MsgPack to various types (double specialization)
template<>
double convertFromMsgPack<double>(const msgpack11::MsgPack& msgpack) {
    if (!msgpack.is_number()) {
        throw MCPSerializationError("Expected number type in MessagePack data");
    }
    return msgpack.number_value();
}

// Convert from MsgPack to various types (vector<string> specialization)
template<>
std::vector<std::string> convertFromMsgPack<std::vector<std::string>>(const msgpack11::MsgPack& msgpack) {
    if (!msgpack.is_array()) {
        throw MCPSerializationError("Expected array type in MessagePack data");
    }
    
    std::vector<std::string> result;
    for (const auto& item : msgpack.array_items()) {
        if (!item.is_string()) {
            throw MCPSerializationError("Expected string type in array");
        }
        result.push_back(item.string_value());
    }
    return result;
}

// Convert from MsgPack to various types (vector<float> specialization)
template<>
std::vector<float> convertFromMsgPack<std::vector<float>>(const msgpack11::MsgPack& msgpack) {
    if (!msgpack.is_array()) {
        throw MCPSerializationError("Expected array type in MessagePack data");
    }
    
    std::vector<float> result;
    for (const auto& item : msgpack.array_items()) {
        if (!item.is_number()) {
            throw MCPSerializationError("Expected number type in array");
        }
        result.push_back(static_cast<float>(item.number_value()));
    }
    return result;
}

// JSON Serialization - placeholder for now
template<typename T>
std::shared_ptr<void> serializeToJSON(const T& value, std::size_t& dataSize) {
    // Create a placeholder implementation
    std::string json_str = "{}";  // Empty JSON object
    dataSize = json_str.size();
        
    char* buffer = new char[dataSize];
    std::memcpy(buffer, json_str.data(), dataSize);
        
    return std::shared_ptr<void>(buffer, [](void* ptr) {
        delete[] static_cast<char*>(ptr);
    });
}

// JSON Deserialization - placeholder for now
template<typename T>
T deserializeFromJSON(const void* data, std::size_t dataSize) {
    // Just a placeholder implementation
    throw MCPSerializationError("JSON deserialization not implemented yet");
}

// Helper to create a MessagePack message
template<typename T>
std::shared_ptr<MCPMessage_V1> createMsgPackMessage(
    const std::string& topic,
    int senderModuleId,
    const T& value) {
    
    std::size_t dataSize = 0;
    std::shared_ptr<void> data = serializeToMsgPack(value, dataSize);
    
    return std::make_shared<MCPMessage_V1>(
        topic,
        senderModuleId,
        DataFormat::MSGPACK,
        data,
        dataSize
    );
}

// Helper to create a JSON message - simplified version
template<typename T>
std::shared_ptr<MCPMessage_V1> createJSONMessage(
    const std::string& topic,
    int senderModuleId,
    const T& value) {
    
    std::size_t dataSize = 0;
    std::shared_ptr<void> data = serializeToJSON(value, dataSize);
    
    return std::make_shared<MCPMessage_V1>(
        topic,
        senderModuleId,
        DataFormat::JSON,
        data,
        dataSize
    );
}

// Extract data from a message based on its format
template<typename T>
T extractMessageData(const MCPMessage_V1* message) {
    if (message == nullptr) {
        throw MCPSerializationError("Cannot extract data from null message");
    }
    
    if (message->data == nullptr || message->dataSize == 0) {
        throw MCPSerializationError("Message has no data");
    }
    
    if (message->dataFormat == DataFormat::MSGPACK) {
        return deserializeFromMsgPack<T>(message->data.get(), message->dataSize);
    } else if (message->dataFormat == DataFormat::JSON) {
        throw MCPSerializationError("JSON deserialization not implemented yet");
    } else {
        throw MCPSerializationError("Unsupported data format");
    }
}

// Explicit template instantiations for common types

// For std::string
template std::shared_ptr<void> serializeToMsgPack<std::string>(const std::string& value, std::size_t& dataSize);
template std::string deserializeFromMsgPack<std::string>(const void* data, std::size_t dataSize);
template std::shared_ptr<void> serializeToJSON<std::string>(const std::string& value, std::size_t& dataSize);
template std::string deserializeFromJSON<std::string>(const void* data, std::size_t dataSize);
template std::shared_ptr<MCPMessage_V1> createMsgPackMessage<std::string>(const std::string& topic, int senderModuleId, const std::string& value);
template std::shared_ptr<MCPMessage_V1> createJSONMessage<std::string>(const std::string& topic, int senderModuleId, const std::string& value);
template std::string extractMessageData<std::string>(const MCPMessage_V1* message);

// For int
template std::shared_ptr<void> serializeToMsgPack<int>(const int& value, std::size_t& dataSize);
template int deserializeFromMsgPack<int>(const void* data, std::size_t dataSize);
template std::shared_ptr<void> serializeToJSON<int>(const int& value, std::size_t& dataSize);
template int deserializeFromJSON<int>(const void* data, std::size_t dataSize);
template std::shared_ptr<MCPMessage_V1> createMsgPackMessage<int>(const std::string& topic, int senderModuleId, const int& value);
template std::shared_ptr<MCPMessage_V1> createJSONMessage<int>(const std::string& topic, int senderModuleId, const int& value);
template int extractMessageData<int>(const MCPMessage_V1* message);

// For double
template std::shared_ptr<void> serializeToMsgPack<double>(const double& value, std::size_t& dataSize);
template double deserializeFromMsgPack<double>(const void* data, std::size_t dataSize);
template std::shared_ptr<void> serializeToJSON<double>(const double& value, std::size_t& dataSize);
template double deserializeFromJSON<double>(const void* data, std::size_t dataSize);
template std::shared_ptr<MCPMessage_V1> createMsgPackMessage<double>(const std::string& topic, int senderModuleId, const double& value);
template std::shared_ptr<MCPMessage_V1> createJSONMessage<double>(const std::string& topic, int senderModuleId, const double& value);
template double extractMessageData<double>(const MCPMessage_V1* message);

// For std::vector<std::string>
template std::shared_ptr<void> serializeToMsgPack<std::vector<std::string>>(const std::vector<std::string>& value, std::size_t& dataSize);
template std::vector<std::string> deserializeFromMsgPack<std::vector<std::string>>(const void* data, std::size_t dataSize);
template std::shared_ptr<void> serializeToJSON<std::vector<std::string>>(const std::vector<std::string>& value, std::size_t& dataSize);
template std::vector<std::string> deserializeFromJSON<std::vector<std::string>>(const void* data, std::size_t dataSize);
template std::shared_ptr<MCPMessage_V1> createMsgPackMessage<std::vector<std::string>>(const std::string& topic, int senderModuleId, const std::vector<std::string>& value);
template std::shared_ptr<MCPMessage_V1> createJSONMessage<std::vector<std::string>>(const std::string& topic, int senderModuleId, const std::vector<std::string>& value);
template std::vector<std::string> extractMessageData<std::vector<std::string>>(const MCPMessage_V1* message);

// For std::vector<float>
template std::shared_ptr<void> serializeToMsgPack<std::vector<float>>(const std::vector<float>& value, std::size_t& dataSize);
template std::vector<float> deserializeFromMsgPack<std::vector<float>>(const void* data, std::size_t dataSize);
template std::shared_ptr<void> serializeToJSON<std::vector<float>>(const std::vector<float>& value, std::size_t& dataSize);
template std::vector<float> deserializeFromJSON<std::vector<float>>(const void* data, std::size_t dataSize);
template std::shared_ptr<MCPMessage_V1> createMsgPackMessage<std::vector<float>>(const std::string& topic, int senderModuleId, const std::vector<float>& value);
template std::shared_ptr<MCPMessage_V1> createJSONMessage<std::vector<float>>(const std::string& topic, int senderModuleId, const std::vector<float>& value);
template std::vector<float> extractMessageData<std::vector<float>>(const MCPMessage_V1* message);

} // namespace serialization
} // namespace mcp 