#pragma once

#include <string>
#include <memory>
#include <cstddef>

namespace mcp {

/**
 * @brief Data format constants for MCPMessage_V1.
 * 
 * These constants define the standardized format identifiers for
 * message payloads. Currently supported formats are MessagePack and JSON.
 */
namespace DataFormat {
    /** MessagePack format identifier */
    const std::string MSGPACK = "application/msgpack";
    
    /** JSON format identifier */
    const std::string JSON = "application/json";
    
    /** Raw binary data format identifier */
    const std::string BINARY = "application/octet-stream";
}

/**
 * @brief Message structure for the Model Context Protocol.
 * 
 * This structure represents a message passed through the MCP system.
 * It contains information about the topic, the sender, the data format,
 * and the serialized data payload.
 * 
 * Version 1 of the message structure.
 */
struct MCPMessage_V1 {
    /**
     * @brief Constructor.
     * 
     * @param topic The topic name associated with this message.
     * @param senderModuleId The ID of the module sending the message.
     * @param dataFormat The format of the serialized data (e.g., "application/msgpack").
     * @param data A shared pointer to the serialized data payload.
     * @param dataSize The size of the serialized data in bytes.
     */
    MCPMessage_V1(
        const std::string& topic,
        int senderModuleId,
        const std::string& dataFormat,
        std::shared_ptr<void> data,
        std::size_t dataSize
    ) : topic(topic),
        senderModuleId(senderModuleId),
        dataFormat(dataFormat),
        data(data),
        dataSize(dataSize) {}
    
    /** The topic name associated with this message. */
    std::string topic;
    
    /** The ID of the module sending the message. */
    int senderModuleId;
    
    /** The format of the serialized data (e.g., "application/msgpack", "application/json"). */
    std::string dataFormat;
    
    /** A shared pointer to the serialized data payload. */
    std::shared_ptr<void> data;
    
    /** The size of the serialized data in bytes. */
    std::size_t dataSize;
};

} // namespace mcp 