# Implementing an MCP Provider

This guide explains how to implement a provider module that shares information through the MCP system.

## Overview

An MCP provider is a module that offers contextual information or data through one or more "topics". Other modules can discover and subscribe to these topics to receive the provided information.

## Key Steps

1. Implement the `IMCPProvider_V1` interface
2. Register topics with the broker
3. Publish messages to registered topics
4. Unregister topics when no longer needed

## Implementation Steps

### Step 1: Implement the IMCPProvider_V1 Interface

Your module needs to implement the `IMCPProvider_V1` interface:

```cpp
#include "mcp/IMCPProvider_V1.h"

class MyModule : public rack::Module, public mcp::IMCPProvider_V1,
                 public std::enable_shared_from_this<MyModule> {
public:
    // IMCPProvider_V1 interface implementation
    std::vector<std::string> getProvidedTopics() const override {
        return {"my-module/parameter1", "my-module/parameter2"};
    }
    
    // Rest of your module implementation...
};
```

Key points:
- Inherit from `IMCPProvider_V1` to implement the provider interface
- Inherit from `std::enable_shared_from_this` to create proper shared_ptr references
- Implement `getProvidedTopics()` to list all topics your module provides

### Step 2: Register Topics with the Broker

Register your topics when your module is added to the rack:

```cpp
void onAdd() override {
    rack::Module::onAdd();
    
    // Get broker instance
    auto broker = mcp::getMCPBroker();
    if (!broker) {
        return;  // Handle broker not available
    }
    
    // Register topics
    auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(this->shared_from_this());
    broker->registerContext("my-module/parameter1", providerPtr);
    broker->registerContext("my-module/parameter2", providerPtr);
    
    // You can log if registration fails
    if (!broker->registerContext("my-module/parameter1", providerPtr)) {
        // Handle registration failure
    }
}
```

Key points:
- Always call the parent class `onAdd()` method first
- Obtain a shared_ptr to your module using `shared_from_this()`
- Cast to the `IMCPProvider_V1` interface
- Register each topic separately
- Check return values to handle failure cases

### Step 3: Publish Messages

Publish messages when you have data to share:

```cpp
void publishParameter1(float value) {
    auto broker = mcp::getMCPBroker();
    if (!broker) {
        return;  // Handle broker not available
    }
    
    try {
        // Create message with serialized data
        auto message = mcp::serialization::createMsgPackMessage(
            "my-module/parameter1",  // Topic
            this->id,                // Module ID
            value                    // Value to publish
        );
        
        // Publish the message
        broker->publish(message);
    }
    catch (const mcp::MCPSerializationError& e) {
        // Handle serialization errors
    }
}
```

Key points:
- Usually call publish from non-audio threads (UI event handlers, etc.)
- Use helper methods like `createMsgPackMessage` for simple data types
- Include proper error handling for serialization errors
- The broker dispatches your message to all subscribers asynchronously

### Step 4: Unregister Topics

Unregister your topics when your module is removed:

```cpp
void onRemove() override {
    // Get broker instance
    auto broker = mcp::getMCPBroker();
    if (broker) {
        auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(this->shared_from_this());
        broker->unregisterContext("my-module/parameter1", providerPtr);
        broker->unregisterContext("my-module/parameter2", providerPtr);
    }
    
    rack::Module::onRemove();
}
```

Key points:
- Unregister all topics your module provides
- Still check for valid broker, even during removal
- Always call the parent class `onRemove()` method last

## Topic Naming Conventions

Follow these conventions for topic names:

- Use lowercase, with hyphens for separating words
- Start with a module or category identifier
- Use slashes to create hierarchy
- Be descriptive but concise

Examples:
- `sequencer/clock/bpm`
- `mixer/master/volume`
- `my-module/parameter1`

## Publishing from the Audio Thread

If you need to publish from your module's `process()` method:

1. Use a thread-safe ring buffer to queue data
2. Use a separate worker thread to read from the queue and publish

See the [Thread Safety Guide](thread_safety.md) for detailed examples.

## Advanced Provider Patterns

### Multi-topic Provider

For modules that provide multiple topics:

```cpp
// Register multiple topics
for (const auto& topic : getProvidedTopics()) {
    broker->registerContext(topic, providerPtr);
}

// Unregister multiple topics
for (const auto& topic : getProvidedTopics()) {
    broker->unregisterContext(topic, providerPtr);
}
```

### Publishing Complex Data

For complex data structures:

```cpp
// Define a struct
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
        std::map<std::string, msgpack11::MsgPack> m;
        m["name"] = msgpack11::MsgPack(data.name);
        m["value"] = msgpack11::MsgPack(data.value);
        
        msgpack11::MsgPack::array values_array;
        for (int val : data.values) {
            values_array.push_back(msgpack11::MsgPack(val));
        }
        m["values"] = msgpack11::MsgPack(values_array);
        
        return msgpack11::MsgPack(m);
    }
    
    template<>
    MyData convertFromMsgPack<MyData>(const msgpack11::MsgPack& msgpack) {
        MyData data;
        if (!msgpack.is_object()) {
            throw MCPSerializationError("Expected object in MessagePack data");
        }
        
        auto obj = msgpack.object_items();
        
        if (obj.find("name") != obj.end()) {
            data.name = obj["name"].string_value();
        }
        
        if (obj.find("value") != obj.end()) {
            data.value = static_cast<float>(obj["value"].number_value());
        }
        
        if (obj.find("values") != obj.end() && obj["values"].is_array()) {
            auto arr = obj["values"].array_items();
            data.values.reserve(arr.size());
            for (const auto& item : arr) {
                data.values.push_back(item.int_value());
            }
        }
        
        return data;
    }
}}

// Then publish your complex data
MyData data = {
    .name = "Test",
    .value = 0.5f,
    .values = {1, 2, 3, 4, 5}
};

auto message = mcp::serialization::createMsgPackMessage(
    "my-module/complex-data",
    this->id,
    data
);
broker->publish(message);
```

### Periodic Publishing

For data that updates regularly:

```cpp
// Start a worker thread for periodic publishing
std::thread m_workerThread;
std::atomic<bool> m_running{false};

void startPublishing() {
    m_running.store(true);
    m_workerThread = std::thread([this]() {
        while (m_running.load()) {
            // Generate data
            float value = generateValue();
            
            // Publish
            publishParameter1(value);
            
            // Wait before next update
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

void stopPublishing() {
    m_running.store(false);
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}
```

## Complete Example

Here's a complete example of a simple provider module:

```cpp
#include "mcp/IMCPProvider_V1.h"
#include "mcp/IMCPBroker.h"
#include "mcp/MCPSerialization.h"
#include <rack.hpp>

class MyProvider : public rack::Module, public mcp::IMCPProvider_V1,
                  public std::enable_shared_from_this<MyProvider> {
public:
    enum ParamIds {
        VALUE_PARAM,
        NUM_PARAMS
    };
    
    MyProvider() {
        config(NUM_PARAMS, 0, 0, 0);
        configParam(VALUE_PARAM, 0.f, 1.f, 0.5f, "Value");
    }
    
    std::vector<std::string> getProvidedTopics() const override {
        return {"my-provider/value"};
    }
    
    void process(const ProcessArgs& args) override {
        // Get current parameter value
        float currentValue = params[VALUE_PARAM].getValue();
        
        // Only publish when value changes significantly
        if (std::abs(currentValue - m_lastPublishedValue) > 0.01f) {
            m_lastPublishedValue = currentValue;
            
            // Queue for publishing
            m_publishQueue.push(currentValue);
            m_hasDataToPublish.store(true);
        }
    }
    
    void onAdd() override {
        rack::Module::onAdd();
        
        // Register with broker
        auto broker = mcp::getMCPBroker();
        if (broker) {
            auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(shared_from_this());
            broker->registerContext("my-provider/value", providerPtr);
        }
        
        // Start publishing thread
        startPublishThread();
    }
    
    void onRemove() override {
        // Stop publishing thread
        stopPublishThread();
        
        // Unregister from broker
        auto broker = mcp::getMCPBroker();
        if (broker) {
            auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(shared_from_this());
            broker->unregisterContext("my-provider/value", providerPtr);
        }
        
        rack::Module::onRemove();
    }
    
private:
    float m_lastPublishedValue = 0.f;
    rack::dsp::RingBuffer<float, 16> m_publishQueue;
    std::atomic<bool> m_hasDataToPublish{false};
    std::atomic<bool> m_running{false};
    std::thread m_publishThread;
    
    void startPublishThread() {
        m_running.store(true);
        m_publishThread = std::thread([this]() {
            auto broker = mcp::getMCPBroker();
            if (!broker) return;
            
            while (m_running.load()) {
                if (m_hasDataToPublish.load()) {
                    float value;
                    while (m_publishQueue.pop(value)) {
                        try {
                            auto message = mcp::serialization::createMsgPackMessage(
                                "my-provider/value",
                                this->id,
                                value
                            );
                            broker->publish(message);
                        }
                        catch (const mcp::MCPSerializationError& e) {
                            // Handle error
                        }
                    }
                    m_hasDataToPublish.store(false);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    
    void stopPublishThread() {
        m_running.store(false);
        if (m_publishThread.joinable()) {
            m_publishThread.join();
        }
    }
};
```

## Testing Your Provider

To test your provider:

1. Use the `getMCPBroker()->getAvailableTopics()` function to verify registration
2. Create a subscriber module to receive your messages
3. Test edge cases like rapid value changes
4. Test hot-swapping your module to ensure clean registration/unregistration

## Common Pitfalls

1. **Forgetting to unregister**: Always unregister topics in `onRemove()`
2. **Publishing too frequently**: Avoid overwhelming the system with messages
3. **Blocking the audio thread**: Don't call `publish()` directly from `process()`
4. **Memory leaks**: Ensure your worker threads are properly stopped and joined

## Next Steps

- Review the [Thread Safety Guide](thread_safety.md) for concurrency best practices
- Explore the [Serialization Guide](serialization.md) for handling complex data types
- See [Example Implementations](../examples/README.md) for complete working examples 