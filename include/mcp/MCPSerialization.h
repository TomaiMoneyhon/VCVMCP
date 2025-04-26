#pragma once

#include "MCPMessage_V1.h"
#include <string>
#include <memory>
#include <vector>
#include <type_traits>
#include <stdexcept>

// Forward declaration for msgpack11::MsgPack
namespace msgpack11 {
    class MsgPack;
}

namespace mcp {

/**
 * @brief Exception class for serialization errors
 */
class MCPSerializationError : public std::runtime_error {
public:
    explicit MCPSerializationError(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * @brief Namespace containing serialization helper functions
 */
namespace serialization {

/**
 * @brief Convert C++ type to msgpack11::MsgPack
 * 
 * Specializations of this template should be provided for each type
 * that needs to be serialized to MessagePack format.
 * 
 * @tparam T The type to convert
 * @param value The value to convert
 * @return msgpack11::MsgPack The converted value
 */
template<typename T>
msgpack11::MsgPack convertToMsgPack(const T& value);

/**
 * @brief Convert msgpack11::MsgPack to C++ type
 * 
 * Specializations of this template should be provided for each type
 * that needs to be deserialized from MessagePack format.
 * 
 * @tparam T The type to convert to
 * @param msgpack The msgpack object to convert
 * @return T The converted value
 */
template<typename T>
T convertFromMsgPack(const msgpack11::MsgPack& msgpack);

/**
 * @brief Serialize object to MessagePack format
 * 
 * @tparam T Type of object to serialize
 * @param obj Object to serialize
 * @param dataSize Output parameter for the size of the serialized data
 * @return std::shared_ptr<void> Pointer to the serialized data
 */
template<typename T>
std::shared_ptr<void> serializeToMsgPack(const T& obj, std::size_t& dataSize);

/**
 * @brief Deserialize object from MessagePack format
 * 
 * @tparam T Type to deserialize to
 * @param data Pointer to the serialized data
 * @param dataSize Size of the serialized data
 * @return T The deserialized object
 */
template<typename T>
T deserializeFromMsgPack(const void* data, std::size_t dataSize);

/**
 * @brief Serialize object to JSON format
 * 
 * @tparam T Type of object to serialize
 * @param obj Object to serialize
 * @param dataSize Output parameter for the size of the serialized data
 * @return std::shared_ptr<void> Pointer to the serialized data
 */
template<typename T>
std::shared_ptr<void> serializeToJSON(const T& obj, std::size_t& dataSize);

/**
 * @brief Deserialize object from JSON format
 * 
 * @tparam T Type to deserialize to
 * @param data Pointer to the serialized data
 * @param dataSize Size of the serialized data
 * @return T The deserialized object
 */
template<typename T>
T deserializeFromJSON(const void* data, std::size_t dataSize);

/**
 * @brief Create an MCPMessage_V1 with MessagePack serialized data
 * 
 * @tparam T Type of object to serialize
 * @param topic Message topic
 * @param senderModuleId Sender module ID
 * @param data Data to serialize
 * @return std::shared_ptr<MCPMessage_V1> The created message
 */
template<typename T>
std::shared_ptr<MCPMessage_V1> createMsgPackMessage(
    const std::string& topic, 
    int senderModuleId, 
    const T& data);

/**
 * @brief Create an MCPMessage_V1 with JSON serialized data
 * 
 * @tparam T Type of object to serialize
 * @param topic Message topic
 * @param senderModuleId Sender module ID
 * @param data Data to serialize
 * @return std::shared_ptr<MCPMessage_V1> The created message
 */
template<typename T>
std::shared_ptr<MCPMessage_V1> createJSONMessage(
    const std::string& topic, 
    int senderModuleId, 
    const T& data);

/**
 * @brief Extract data from an MCPMessage_V1
 * 
 * @tparam T Type to deserialize to
 * @param message The message to extract data from
 * @return T The deserialized data
 */
template<typename T>
T extractMessageData(const MCPMessage_V1* message);

} // namespace serialization
} // namespace mcp 