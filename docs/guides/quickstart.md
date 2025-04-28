# MCP Quick Start Guide

This guide will help you quickly implement basic MCP functionality in your VCV Rack modules.

## 1. Prerequisites

- Basic understanding of C++ and VCV Rack module development
- VCV Rack SDK development environment set up
- MCP integrated into your build environment

## 2. Creating a Provider Module in 5 Minutes

### Step 1: Include the necessary headers

```cpp
#include "mcp/IMCPProvider_V1.h"
#include "mcp/IMCPBroker.h"
#include "mcp/MCPSerialization.h"
```

### Step 2: Implement the IMCPProvider_V1 interface

```cpp
class MyModule : public rack::Module, public mcp::IMCPProvider_V1,
                public std::enable_shared_from_this<MyModule> {
public:
    // ... your existing module code ...
    
    // Implement the IMCPProvider_V1 interface
    std::vector<std::string> getProvidedTopics() const override {
        return {"my-module/value"};
    }
};
```

### Step 3: Register with the broker in onAdd

```cpp
void onAdd() override {
    rack::Module::onAdd();
    
    // Register with the broker
    auto broker = mcp::getMCPBroker();
    if (broker) {
        auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(shared_from_this());
        broker->registerContext("my-module/value", providerPtr);
    }
}
```

### Step 4: Unregister in onRemove

```cpp
void onRemove() override {
    // Unregister from the broker
    auto broker = mcp::getMCPBroker();
    if (broker) {
        auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(shared_from_this());
        broker->unregisterContext("my-module/value", providerPtr);
    }
    
    rack::Module::onRemove();
}
```

### Step 5: Publish data

```cpp
void publishValue(float value) {
    auto broker = mcp::getMCPBroker();
    if (!broker) return;
    
    try {
        auto message = mcp::serialization::createMsgPackMessage(
            "my-module/value",
            this->id,
            value
        );
        broker->publish(message);
    }
    catch (const mcp::MCPSerializationError& e) {
        // Handle error
    }
}

// Call this from a UI event handler or worker thread
void onKnobChange() {
    float value = params[VALUE_PARAM].getValue();
    publishValue(value);
}
```

## 3. Creating a Subscriber Module in 5 Minutes

### Step 1: Include the necessary headers

```cpp
#include "mcp/IMCPSubscriber_V1.h"
#include "mcp/IMCPBroker.h"
#include "mcp/MCPSerialization.h"
```

### Step 2: Implement the IMCPSubscriber_V1 interface

```cpp
class MyModule : public rack::Module, public mcp::IMCPSubscriber_V1,
                public std::enable_shared_from_this<MyModule> {
public:
    // ... your existing module code ...
    
    // Implement the IMCPSubscriber_V1 interface
    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        if (!message) return;
        
        try {
            if (message->topic == "other-module/value") {
                float value = mcp::serialization::extractMessageData<float>(message);
                
                // Pass to audio thread via ring buffer
                m_valueBuffer.push(value);
                m_hasNewValue.store(true, std::memory_order_release);
            }
        }
        catch (const mcp::MCPSerializationError& e) {
            // Handle error
        }
    }
    
private:
    rack::dsp::RingBuffer<float, 16> m_valueBuffer;
    std::atomic<bool> m_hasNewValue{false};
    float m_currentValue = 0.f;
};
```

### Step 3: Subscribe in onAdd

```cpp
void onAdd() override {
    rack::Module::onAdd();
    
    // Subscribe to topics
    auto broker = mcp::getMCPBroker();
    if (broker) {
        auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(shared_from_this());
        broker->subscribe("other-module/value", subscriberPtr);
    }
}
```

### Step 4: Unsubscribe in onRemove

```cpp
void onRemove() override {
    // Unsubscribe from all topics
    auto broker = mcp::getMCPBroker();
    if (broker) {
        auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(shared_from_this());
        broker->unsubscribeAll(subscriberPtr);
    }
    
    rack::Module::onRemove();
}
```

### Step 5: Use the data in the audio thread

```cpp
void process(const ProcessArgs& args) override {
    // Check if we have new values
    if (m_hasNewValue.load(std::memory_order_acquire)) {
        float value;
        while (m_valueBuffer.pop(value)) {
            m_currentValue = value;
        }
        m_hasNewValue.store(false, std::memory_order_release);
    }
    
    // Use m_currentValue in your audio processing
    outputs[VALUE_OUTPUT].setVoltage(m_currentValue * 10.f);
}
```

## 4. Complete Example

Here's a minimal but complete example of a module that both provides and subscribes to MCP topics:

```cpp
#include "plugin.hpp"
#include "mcp/IMCPProvider_V1.h"
#include "mcp/IMCPSubscriber_V1.h"
#include "mcp/IMCPBroker.h"
#include "mcp/MCPSerialization.h"

struct MCPExampleModule : Module, mcp::IMCPProvider_V1, mcp::IMCPSubscriber_V1,
                         std::enable_shared_from_this<MCPExampleModule> {
    enum ParamId {
        VALUE_PARAM,
        PARAMS_LEN
    };
    
    enum OutputId {
        VALUE_OUTPUT,
        OUTPUTS_LEN
    };
    
    // For receiving values
    rack::dsp::RingBuffer<float, 16> m_valueBuffer;
    std::atomic<bool> m_hasNewValue{false};
    float m_currentValue = 0.5f;
    
    // For tracking sent values
    float m_lastSentValue = 0.f;
    
    MCPExampleModule() {
        config(PARAMS_LEN, 0, OUTPUTS_LEN, 0);
        configParam(VALUE_PARAM, 0.f, 1.f, 0.5f, "Value");
    }
    
    // IMCPProvider_V1 implementation
    std::vector<std::string> getProvidedTopics() const override {
        return {"example-module/value"};
    }
    
    // IMCPSubscriber_V1 implementation
    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        if (!message) return;
        
        try {
            if (message->topic == "example-module/value") {
                // Only process messages from other modules
                if (message->senderModuleId != this->id) {
                    float value = mcp::serialization::extractMessageData<float>(message);
                    m_valueBuffer.push(value);
                    m_hasNewValue.store(true, std::memory_order_release);
                }
            }
        }
        catch (const mcp::MCPSerializationError& e) {
            // Handle error
        }
    }
    
    void process(const ProcessArgs& args) override {
        // Get current parameter value
        float paramValue = params[VALUE_PARAM].getValue();
        
        // If parameter changed significantly, publish it
        if (std::abs(paramValue - m_lastSentValue) > 0.01f) {
            m_lastSentValue = paramValue;
            publishValue(paramValue);
        }
        
        // Check if we received new values
        if (m_hasNewValue.load(std::memory_order_acquire)) {
            float value;
            while (m_valueBuffer.pop(value)) {
                m_currentValue = value;
            }
            m_hasNewValue.store(false, std::memory_order_release);
        }
        
        // Output the current value
        outputs[VALUE_OUTPUT].setVoltage(m_currentValue * 10.f);
    }
    
    void onAdd() override {
        Module::onAdd();
        
        auto broker = mcp::getMCPBroker();
        if (broker) {
            // Cast to the appropriate interface types
            auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(shared_from_this());
            auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(shared_from_this());
            
            // Register as provider
            broker->registerContext("example-module/value", providerPtr);
            
            // Register as subscriber (to receive values from other instances)
            broker->subscribe("example-module/value", subscriberPtr);
        }
    }
    
    void onRemove() override {
        auto broker = mcp::getMCPBroker();
        if (broker) {
            auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(shared_from_this());
            auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(shared_from_this());
            
            broker->unregisterContext("example-module/value", providerPtr);
            broker->unsubscribeAll(subscriberPtr);
        }
        
        Module::onRemove();
    }
    
private:
    void publishValue(float value) {
        auto broker = mcp::getMCPBroker();
        if (!broker) return;
        
        try {
            auto message = mcp::serialization::createMsgPackMessage(
                "example-module/value",
                this->id,
                value
            );
            broker->publish(message);
        }
        catch (const mcp::MCPSerializationError& e) {
            // Handle error
        }
    }
};
```

## 5. Common Pitfalls

- **Thread Safety**: Never directly modify audio processing state in `onMCPMessage()`
- **Missing Unregister/Unsubscribe**: Always clean up in `onRemove()`
- **Casting Issues**: Use `std::dynamic_pointer_cast` to get the right interface
- **Error Handling**: Always handle serialization errors with try/catch blocks
- **Missing shared_from_this**: Remember to inherit from `std::enable_shared_from_this`

## 6. Next Steps

- Explore the [Thread Safety Guide](thread_safety.md) for detailed threading information
- Learn about [Serialization](serialization.md) for handling complex data types
- See the [Provider Implementation Guide](provider_implementation.md) for advanced provider features
- Check the [Subscriber Implementation Guide](subscriber_implementation.md) for advanced subscriber features 