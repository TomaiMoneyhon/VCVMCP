# Implementing an MCP Subscriber

This guide explains how to implement a subscriber module that receives information through the MCP system.

## Overview

An MCP subscriber is a module that receives contextual information from provider modules by subscribing to specific topics. Subscribers can process this information and use it to influence their behavior.

## Key Steps

1. Implement the `IMCPSubscriber_V1` interface
2. Subscribe to topics of interest
3. Implement the message handler
4. Pass data safely to the audio thread
5. Unsubscribe when no longer needed

## Implementation Steps

### Step 1: Implement the IMCPSubscriber_V1 Interface

Your module needs to implement the `IMCPSubscriber_V1` interface:

```cpp
#include "mcp/IMCPSubscriber_V1.h"

class MyModule : public rack::Module, public mcp::IMCPSubscriber_V1,
                 public std::enable_shared_from_this<MyModule> {
public:
    // IMCPSubscriber_V1 interface implementation
    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        // Process incoming messages (will be implemented in Step 3)
    }
    
    // Rest of your module implementation...
};
```

Key points:
- Inherit from `IMCPSubscriber_V1` to implement the subscriber interface
- Inherit from `std::enable_shared_from_this` to create proper shared_ptr references
- Implement `onMCPMessage()` to handle incoming messages

### Step 2: Subscribe to Topics

Subscribe to topics when your module is added to the rack:

```cpp
void onAdd() override {
    rack::Module::onAdd();
    
    // Get broker instance
    auto broker = mcp::getMCPBroker();
    if (!broker) {
        return;  // Handle broker not available
    }
    
    // Subscribe to topics
    auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(this->shared_from_this());
    broker->subscribe("other-module/parameter1", subscriberPtr);
    broker->subscribe("other-module/parameter2", subscriberPtr);
    
    // You can log if subscription fails
    if (!broker->subscribe("other-module/parameter1", subscriberPtr)) {
        // Handle subscription failure
    }
}
```

Key points:
- Always call the parent class `onAdd()` method first
- Obtain a shared_ptr to your module using `shared_from_this()`
- Cast to the `IMCPSubscriber_V1` interface
- Subscribe to each topic separately
- Check return values to handle failure cases

### Step 3: Implement the Message Handler

Implement the `onMCPMessage` method to process incoming messages:

```cpp
void onMCPMessage(const mcp::MCPMessage_V1* message) override {
    // Always check for null message
    if (!message) return;
    
    try {
        // Process messages based on topic
        if (message->topic == "other-module/parameter1") {
            // Extract data - this example assumes a float value
            float value = mcp::serialization::extractMessageData<float>(message);
            
            // Store in ring buffer for audio thread to use
            m_valueBuffer.push(value);
            m_hasNewValue.store(true, std::memory_order_release);
        }
        else if (message->topic == "other-module/parameter2") {
            // Handle other topic - this example assumes a string value
            std::string text = mcp::serialization::extractMessageData<std::string>(message);
            // Store or process the text value
            m_textValue = text;
        }
    }
    catch (const mcp::MCPSerializationError& e) {
        // Handle deserialization errors
    }
}
```

Key points:
- Remember that `onMCPMessage` is called on a worker thread, not the audio thread
- Use topic names to identify and route messages
- Use `extractMessageData<T>()` to deserialize the message data
- Handle errors with try/catch blocks
- Use thread-safe mechanisms to pass data to the audio thread

### Step 4: Access Data in the Audio Thread

Access the received data in your module's `process()` method:

```cpp
void process(const ProcessArgs& args) override {
    // Check if we have new values
    if (m_hasNewValue.load(std::memory_order_acquire)) {
        float value;
        // Process all values in the buffer
        while (m_valueBuffer.pop(value)) {
            // Use the value in your audio processing
            m_currentValue = value;
        }
        m_hasNewValue.store(false, std::memory_order_release);
    }
    
    // Use m_currentValue in audio processing
    outputs[OUT_OUTPUT].setVoltage(m_currentValue * 10.f);
}
```

Key points:
- Use atomic flags to signal when new data is available
- Use ring buffers to pass data between threads
- Process all available values to clear the buffer
- Reset the flag after processing

### Step 5: Unsubscribe from Topics

Unsubscribe from all topics when your module is removed:

```cpp
void onRemove() override {
    // Get broker instance
    auto broker = mcp::getMCPBroker();
    if (broker) {
        auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(this->shared_from_this());
        
        // Option 1: Unsubscribe from specific topics
        broker->unsubscribe("other-module/parameter1", subscriberPtr);
        broker->unsubscribe("other-module/parameter2", subscriberPtr);
        
        // Option 2: Unsubscribe from all topics at once
        broker->unsubscribeAll(subscriberPtr);
    }
    
    rack::Module::onRemove();
}
```

Key points:
- You can unsubscribe from specific topics or use `unsubscribeAll()` for convenience
- Still check for valid broker, even during removal
- Always call the parent class `onRemove()` method last

## Thread Safety Considerations

Remember that `onMCPMessage()` is called on a worker thread, not the audio thread. This has important implications:

1. **DO NOT** modify audio processing state directly in `onMCPMessage()`
2. **DO NOT** call VCV UI functions directly from `onMCPMessage()`
3. **DO** use thread-safe mechanisms (like ring buffers) to pass data to the audio thread

Example of thread-safe member variables:

```cpp
private:
    // For passing values to audio thread
    rack::dsp::RingBuffer<float, 16> m_valueBuffer;
    std::atomic<bool> m_hasNewValue{false};
    
    // For audio thread usage
    float m_currentValue = 0.f;
    
    // For UI thread usage (if needed)
    std::string m_textValue;
    std::mutex m_textMutex;  // For protecting access to m_textValue
```

For more detailed information, see the [Thread Safety Guide](thread_safety.md).

## Discovery and Dynamic Subscription

Sometimes you don't know topic names in advance. You can discover them:

```cpp
void discoverAndSubscribe() {
    auto broker = mcp::getMCPBroker();
    if (!broker) return;
    
    // Get all available topics
    auto topics = broker->getAvailableTopics();
    
    // Subscribe to topics matching a pattern
    auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(this->shared_from_this());
    for (const auto& topic : topics) {
        // Example: subscribe to all tempo-related topics
        if (topic.find("/tempo") != std::string::npos) {
            broker->subscribe(topic, subscriberPtr);
        }
    }
}
```

## Handling Complex Data

For complex data structures:

```cpp
// Define the same struct as the provider
struct MyData {
    std::string name;
    float value;
    std::vector<int> values;
};

// Implement MessagePack conversion for your type
namespace mcp {
namespace serialization {
    template<>
    msgpack11::MsgPack convertToMsgPack<MyData>(const MyData& data) {
        // Same implementation as in the provider
    }
    
    template<>
    MyData convertFromMsgPack<MyData>(const msgpack11::MsgPack& msgpack) {
        // Same implementation as in the provider
    }
}}

// Then in onMCPMessage:
void onMCPMessage(const mcp::MCPMessage_V1* message) override {
    if (!message) return;
    
    try {
        if (message->topic == "other-module/complex-data") {
            // Extract the complex data
            MyData data = mcp::serialization::extractMessageData<MyData>(message);
            
            // Use the data...
            // Note: For complex data, you might want to use a queue with a mutex
            // instead of a simple ring buffer
        }
    }
    catch (const mcp::MCPSerializationError& e) {
        // Handle error
    }
}
```

## Complete Example

Here's a complete example of a simple subscriber module:

```cpp
#include "mcp/IMCPSubscriber_V1.h"
#include "mcp/IMCPBroker.h"
#include "mcp/MCPSerialization.h"
#include <rack.hpp>

class MySubscriber : public rack::Module, public mcp::IMCPSubscriber_V1,
                     public std::enable_shared_from_this<MySubscriber> {
public:
    enum OutputIds {
        VALUE_OUTPUT,
        NUM_OUTPUTS
    };
    
    MySubscriber() {
        config(0, 0, NUM_OUTPUTS, 0);
    }
    
    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        if (!message) return;
        
        try {
            if (message->topic == "provider/value") {
                float value = mcp::serialization::extractMessageData<float>(message);
                m_valueBuffer.push(value);
                m_hasNewValue.store(true, std::memory_order_release);
            }
        }
        catch (const mcp::MCPSerializationError& e) {
            // Log error
        }
    }
    
    void process(const ProcessArgs& args) override {
        if (m_hasNewValue.load(std::memory_order_acquire)) {
            float value;
            while (m_valueBuffer.pop(value)) {
                m_currentValue = value;
            }
            m_hasNewValue.store(false, std::memory_order_release);
        }
        
        outputs[VALUE_OUTPUT].setVoltage(m_currentValue * 10.f);
    }
    
    void onAdd() override {
        rack::Module::onAdd();
        
        // Subscribe to topics
        auto broker = mcp::getMCPBroker();
        if (broker) {
            auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(shared_from_this());
            broker->subscribe("provider/value", subscriberPtr);
        }
    }
    
    void onRemove() override {
        // Unsubscribe from all topics
        auto broker = mcp::getMCPBroker();
        if (broker) {
            auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(shared_from_this());
            broker->unsubscribeAll(subscriberPtr);
        }
        
        rack::Module::onRemove();
    }
    
private:
    rack::dsp::RingBuffer<float, 16> m_valueBuffer;
    std::atomic<bool> m_hasNewValue{false};
    float m_currentValue = 0.f;
};
```

## Testing Your Subscriber

To test your subscriber:

1. Create a matching provider module or use the reference provider
2. Verify that messages are being received in `onMCPMessage()`
3. Verify that data is being properly passed to the audio thread
4. Test edge cases like high message frequencies
5. Test with invalid or unexpected data formats
6. Test hot-swapping your module to ensure clean subscription/unsubscription

## Common Pitfalls

1. **Thread safety issues**: Modifying audio state directly from `onMCPMessage()`
2. **Forgetting to unsubscribe**: Always unsubscribe in `onRemove()`
3. **Buffer overflow**: Not processing ring buffer data fast enough
4. **Missing error handling**: Not properly handling serialization errors
5. **Processing everything**: Trying to process too many messages in `onMCPMessage()`

## Next Steps

- Review the [Thread Safety Guide](thread_safety.md) for concurrency best practices
- Learn more about RingBuffer usage in the [RingBuffer Guide](ringbuffer.md)
- See [Example Implementations](../examples/README.md) for complete working examples 