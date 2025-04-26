#pragma once

#include <string>
#include <memory>

namespace mcp {

// Forward declaration for MCPMessage_V1 (will be fully defined in Sprint 3)
struct MCPMessage_V1;

/**
 * @brief Interface for modules that subscribe to contextual information.
 * 
 * This interface is implemented by modules that want to receive contextual
 * information from other modules through the MCP system. Subscriber modules
 * register with the broker to subscribe to specific topics.
 * 
 * Version 1 of the interface.
 */
class IMCPSubscriber_V1 {
public:
    virtual ~IMCPSubscriber_V1() = default;

    /**
     * @brief Callback function called when a new message is available.
     * 
     * This function is called by the broker when a message for a topic that this
     * module is subscribed to is published. It will be called on a worker thread,
     * not on the audio thread, so implementations must handle thread safety.
     * 
     * @param message The message that was published.
     */
    virtual void onMCPMessage(const MCPMessage_V1* message) = 0;
};

} // namespace mcp 