# VCV Rack MCP API Documentation

## Introduction

The Model Context Protocol (MCP) provides a standardized system for VCV Rack 2 modules to exchange complex, structured data. This document describes the API implemented in Phase 1 (Broker & API Foundation) and Phase 2 (Serialization), covering Sprint 1, 2, and 3.

## Core Interfaces

### IMCPBroker

The broker is the central component of the MCP system. It manages topic registration, discovery, and subscription.

```cpp
namespace mcp {
    class IMCPBroker {
    public:
        // Registration functions
        virtual bool registerContext(const std::string& topic, 
                                   std::shared_ptr<IMCPProvider_V1> provider) = 0;
        virtual bool unregisterContext(const std::string& topic, 
                                      std::shared_ptr<IMCPProvider_V1> provider) = 0;
        
        // Subscription functions (added in Sprint 2)
        virtual bool subscribe(const std::string& topic,
                             std::shared_ptr<IMCPSubscriber_V1> subscriber) = 0;
        virtual bool unsubscribe(const std::string& topic,
                               std::shared_ptr<IMCPSubscriber_V1> subscriber) = 0;
        virtual bool unsubscribeAll(std::shared_ptr<IMCPSubscriber_V1> subscriber) = 0;
        
        // Discovery functions
        virtual std::vector<std::string> getAvailableTopics() const = 0;
        virtual std::vector<std::shared_ptr<IMCPProvider_V1>> findProviders(
                                                    const std::string& topic) const = 0;
        
        // Versioning
        virtual int getVersion() const = 0;
    };
    
    // Global accessor
    std::shared_ptr<IMCPBroker> getMCPBroker();
}
```

### IMCPProvider_V1

The provider interface is implemented by modules that offer context information.

```cpp
namespace mcp {
    class IMCPProvider_V1 {
    public:
        virtual std::vector<std::string> getProvidedTopics() const = 0;
    };
}
```

### IMCPSubscriber_V1

The subscriber interface is implemented by modules that want to receive context information.

```cpp
namespace mcp {
    class IMCPSubscriber_V1 {
    public:
        virtual void onMCPMessage(const MCPMessage_V1* message) = 0;
    };
}
```

### MCPMessage_V1

The message structure for data exchange in the MCP system. Added in Sprint 3.

```cpp
namespace mcp {
    // Data format constants
    namespace DataFormat {
        const std::string MSGPACK = "application/msgpack";
        const std::string JSON = "application/json";
        const std::string BINARY = "application/octet-stream";
    }
    
    struct MCPMessage_V1 {
        MCPMessage_V1(
            const std::string& topic,
            int senderModuleId,
            const std::string& dataFormat,
            std::shared_ptr<void> data,
            std::size_t dataSize
        );
        
        std::string topic;              // The topic name
        int senderModuleId;             // ID of the sender module
        std::string dataFormat;         // Format of the data (e.g., "application/msgpack")
        std::shared_ptr<void> data;     // Serialized data payload
        std::size_t dataSize;           // Size of the serialized data in bytes
    };
}
```

### RingBuffer

The RingBuffer is a thread-safe, lock-free data structure for passing information between threads, particularly from worker threads to the audio thread.

```cpp
namespace mcp {
    template <typename T>
    class RingBuffer {
    public:
        // Constructor
        RingBuffer(size_t capacity = 16);
        
        // Core operations
        bool push(const T& value);
        bool pop(T& value);
        
        // Status inquiry
        size_t size() const;
        bool empty() const;
        bool full() const;
        
        // Management
        void clear();
    };
}
```

#### Purpose

The RingBuffer provides a thread-safe way to pass data between threads, specifically designed for the Single-Producer/Single-Consumer (SPSC) pattern. It is optimized for the MCP use case where:

- Worker thread receives messages via `onMCPMessage()` and pushes them to the buffer
- Audio thread processes messages by popping them from the buffer in its `process()` method

#### Thread Safety Guarantees

The RingBuffer implementation provides the following thread safety guarantees:

1. **SPSC Thread Safety**: Safe when one thread calls `push()` and a different thread calls `pop()`
2. **Status Method Safety**: `size()`, `empty()`, and `full()` are safe to call from any thread
3. **Non-Thread-Safe Methods**: `clear()` is not thread-safe and should only be called when no other threads are accessing the buffer

#### Implementation Details

- Uses atomic operations with sequential consistency for all memory operations
- Contains explicit memory barriers to ensure data visibility between threads
- Avoids the ABA problem through dedicated roles for producer and consumer threads
- Achieves high performance (~980,000 messages/second) with perfect reliability

#### Best Practices

1. **Follow the SPSC Pattern**: 
   - One thread should be the only producer (calls `push()`)
   - A different thread should be the only consumer (calls `pop()`)

2. **Proper Buffer Sizing**:
   - Default capacity is 16 elements
   - Recommended size is 64-128 elements for most modules
   - Oversized buffers waste memory; undersized buffers may drop messages

3. **Handle Full Buffers Appropriately**:
   ```cpp
   // In worker thread (producer)
   if (!buffer.push(value)) {
       // Handle buffer full condition - don't block!
   }
   ```

4. **Efficient Message Processing**:
   ```cpp
   // In audio thread (consumer)
   T value;
   while (buffer.pop(value)) {
       // Process each value
   }
   ```

## Serialization Helpers

The MCP system provides helper functions for serializing and deserializing data. Added in Sprint 3.

```cpp
namespace mcp {
    // Error handling
    class MCPSerializationError : public std::runtime_error {
        // Thrown when serialization or deserialization fails
    };
    
    namespace serialization {
        // MessagePack Serialization/Deserialization
        template<typename T>
        std::shared_ptr<void> serializeToMsgPack(const T& value, std::size_t& dataSize);
        
        template<typename T>
        T deserializeFromMsgPack(const void* data, std::size_t dataSize);
        
        // JSON Serialization/Deserialization
        template<typename T>
        std::shared_ptr<void> serializeToJSON(const T& value, std::size_t& dataSize);
        
        template<typename T>
        T deserializeFromJSON(const void* data, std::size_t dataSize);
        
        // Message Creation Helpers
        template<typename T>
        std::shared_ptr<MCPMessage_V1> createMsgPackMessage(
            const std::string& topic,
            int senderModuleId,
            const T& value);
        
        template<typename T>
        std::shared_ptr<MCPMessage_V1> createJSONMessage(
            const std::string& topic,
            int senderModuleId,
            const T& value);
        
        // Message Data Extraction Helper
        template<typename T>
        T extractMessageData(const MCPMessage_V1* message);
    }
}
```

## Usage Guide

### Accessing the Broker

The broker is implemented as a singleton and can be accessed using the global accessor function:

```cpp
#include "mcp/IMCPBroker.h"

auto broker = mcp::getMCPBroker();
```

### Implementing a Provider

To create a module that provides context information:

1. Implement the `IMCPProvider_V1` interface:

```cpp
class MyModule : public rack::Module, public mcp::IMCPProvider_V1 {
public:
    std::vector<std::string> getProvidedTopics() const override {
        return {"my-module/settings", "my-module/state"};
    }
    
    // ...other module code...
};
```

2. Register with the broker during initialization:

```cpp
void onAdd() override {
    rack::Module::onAdd();
    
    auto broker = mcp::getMCPBroker();
    broker->registerContext("my-module/settings", std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(this->shared_from_this()));
    broker->registerContext("my-module/state", std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(this->shared_from_this()));
}
```

3. Unregister during removal:

```cpp
void onRemove() override {
    auto broker = mcp::getMCPBroker();
    broker->unregisterContext("my-module/settings", std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(this->shared_from_this()));
    broker->unregisterContext("my-module/state", std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(this->shared_from_this()));
    
    rack::Module::onRemove();
}
```

### Implementing a Subscriber

To create a module that subscribes to context information:

1. Implement the `IMCPSubscriber_V1` interface:

```cpp
class MyModule : public rack::Module, public mcp::IMCPSubscriber_V1 {
public:
    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        // Process incoming message
        // This is called on a worker thread, not the audio thread!
        // Use thread-safe communication to pass data to the audio thread
        
        // Extract data from the message
        try {
            if (message->topic == "other-module/settings") {
                auto settings = mcp::serialization::extractMessageData<MySettings>(message);
                // Process settings...
            }
        } catch (const mcp::MCPSerializationError& e) {
            // Handle serialization error
        }
    }
    
    // ...other module code...
};
```

2. Subscribe during initialization:

```cpp
void onAdd() override {
    rack::Module::onAdd();
    
    auto broker = mcp::getMCPBroker();
    broker->subscribe("other-module/settings", std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(this->shared_from_this()));
}
```

3. Unsubscribe during removal:

```cpp
void onRemove() override {
    auto broker = mcp::getMCPBroker();
    broker->unsubscribeAll(std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(this->shared_from_this()));
    
    rack::Module::onRemove();
}
```

### Using RingBuffer for Thread-Safe Communication

To safely pass data from worker threads to the audio thread:

```cpp
class MyModule : public rack::Module, public mcp::IMCPSubscriber_V1 {
private:
    // Create a ring buffer for thread-safe communication
    mcp::RingBuffer<float> m_valueBuffer{64}; // Size based on expected message rate
    std::atomic<bool> m_hasNewValues{false};
    
public:
    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        // Called on worker thread
        try {
            if (message->topic == "other-module/value") {
                float value = mcp::serialization::extractMessageData<float>(message);
                
                // Push to ring buffer (producer)
                if (m_valueBuffer.push(value)) {
                    m_hasNewValues.store(true, std::memory_order_release);
                } else {
                    // Handle buffer full condition
                    // LOG_WARNING("Buffer full, value dropped");
                }
            }
        } catch (const mcp::MCPSerializationError& e) {
            // Handle error
        }
    }
    
    void process(const rack::engine::ProcessArgs& args) override {
        // Called on audio thread
        
        // Check if we have new values
        if (m_hasNewValues.load(std::memory_order_acquire)) {
            float value;
            
            // Pop all available values (consumer)
            while (m_valueBuffer.pop(value)) {
                // Process the value
                // e.g., outputs[VALUE_OUTPUT].setVoltage(value);
            }
            
            m_hasNewValues.store(false, std::memory_order_release);
        }
        
        // Continue with regular processing...
    }
};
```

### Creating and Sending Messages (Coming in Sprint 4)

The following code demonstrates how messages will be created and published once the publish functionality is implemented in Sprint 4:

```cpp
// Prepare data to send
MyData data = { /* ... */ };

// Create a message with MessagePack serialization
auto message = mcp::serialization::createMsgPackMessage(
    "my-module/data",  // Topic
    this->id,          // Module ID
    data               // Data to serialize
);

// In Sprint 4, this message will be published via the broker
// broker->publish(message);
```

### Data Types Compatible with Serialization

For MessagePack serialization, a type must either:
1. Be a basic type (int, float, string, etc.)
2. Be a container of basic types (vector, map, etc.)
3. Define the `MSGPACK_DEFINE(...)` macro with its member variables

For JSON serialization, a type must either:
1. Be a basic type supported by nlohmann::json
2. Be a container of supported types
3. Define `to_json` and `from_json` friend functions

## Thread Safety

The MCP system is designed to be thread-safe. All broker methods can be called from any thread, including the audio thread, though care should be taken to avoid performance impacts.

Key thread safety considerations:

1. Registration/unregistration can happen from any thread, including the audio thread.
2. Subscription/unsubscription can happen from any thread.
3. The `onMCPMessage` callback will be called on a worker thread, not the audio thread.
4. Modules should use thread-safe communication (like `dsp::RingBuffer`) to pass data from the `onMCPMessage` callback to the audio thread.

## Future Developments

- Sprint 3 (current) has implemented the message structure and serialization functionality.
- Sprint 4 will implement the publish/receive functionality, allowing providers to publish messages and subscribers to receive them.

When Sprint 4 is complete, the `onMCPMessage` callback will be called when messages are published to subscribed topics. 