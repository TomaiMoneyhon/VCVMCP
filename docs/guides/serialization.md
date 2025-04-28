# MCP Serialization Guide

This guide explains how serialization works in the MCP system and how to use it effectively in your modules.

## Overview

Serialization is the process of converting data structures into a format that can be transmitted, stored, and reconstructed. In the MCP system, serialization allows modules to exchange complex data structures beyond simple control voltages.

## Supported Formats

MCP supports two primary serialization formats:

1. **MessagePack** (`application/msgpack`): A binary format similar to JSON but more compact. This is the recommended format for most use cases.

2. **JSON** (`application/json`): A text-based format that is human-readable. Useful for debugging but less efficient than MessagePack.

## Basic Serialization/Deserialization

### Using Helper Functions

The MCP system provides helper functions that simplify serialization and deserialization:

```cpp
// Serialization example
float value = 0.5f;
std::size_t dataSize;
std::shared_ptr<void> data = mcp::serialization::serializeToMsgPack(value, dataSize);

// Deserialization example
float restoredValue = mcp::serialization::deserializeFromMsgPack<float>(data.get(), dataSize);
```

### Creating and Publishing Messages

The easiest way to publish data is using the `createMsgPackMessage` helper:

```cpp
// Create and publish a message with a float value
auto message = mcp::serialization::createMsgPackMessage(
    "my-module/parameter1",  // Topic
    this->id,                // Module ID
    0.5f                     // Value to publish
);
broker->publish(message);
```

### Extracting Data from Messages

When receiving a message in `onMCPMessage()`, extract the data using the `extractMessageData` helper:

```cpp
void onMCPMessage(const mcp::MCPMessage_V1* message) override {
    try {
        if (message->topic == "other-module/parameter1") {
            float value = mcp::serialization::extractMessageData<float>(message);
            // Use the value...
        }
    }
    catch (const mcp::MCPSerializationError& e) {
        // Handle errors...
    }
}
```

## Supported Data Types

The serialization system natively supports these data types:

* **Primitive Types**: `int`, `float`, `double`, `bool`
* **Strings**: `std::string`
* **Arrays**: `std::vector<T>` (where T is a supported type)
* **Objects**: Custom structs/classes (with proper conversion implementations)

## Serializing Custom Types

For custom data structures, you need to implement conversion functions:

```cpp
// Define your custom type
struct MyData {
    std::string name;
    float value;
    std::vector<int> values;
};

// Implement MessagePack conversion for your type
namespace mcp {
namespace serialization {
    
    // Convert C++ type to MsgPack
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
    
    // Convert MsgPack to C++ type
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
```

Once you've implemented these functions, you can use the standard helper functions:

```cpp
// Create your data structure
MyData data;
data.name = "Test";
data.value = 0.5f;
data.values = {1, 2, 3, 4, 5};

// Serialize and publish
auto message = mcp::serialization::createMsgPackMessage(
    "my-module/complex-data",
    this->id,
    data
);
broker->publish(message);

// ...and in the subscriber:
MyData receivedData = mcp::serialization::extractMessageData<MyData>(message);
```

## Best Practices

### Error Handling

Always use try/catch blocks when deserializing data to handle potential errors:

```cpp
try {
    auto value = mcp::serialization::extractMessageData<float>(message);
}
catch (const mcp::MCPSerializationError& e) {
    // Handle the error appropriately
    // e.what() provides the error message
}
```

### Data Structure Versioning

When designing custom data structures, consider future changes:

1. **Use Named Fields**: Always use named fields (objects/maps) rather than arrays for extensibility
2. **Optional Fields**: Make new fields optional with reasonable defaults
3. **Version Field**: Consider adding a version field to your structures

```cpp
// Good - extensible format
msgpack11::MsgPack::object data;
data["version"] = msgpack11::MsgPack(1);
data["value"] = msgpack11::MsgPack(value);
// Can add new fields later without breaking compatibility

// Bad - rigid format
msgpack11::MsgPack::array data;
data.push_back(msgpack11::MsgPack(value));
// Adding more values later breaks compatibility
```

### Performance Considerations

Serialization/deserialization can impact performance:

1. **Minimize Serialization Frequency**: Don't serialize/publish on every audio frame
2. **Keep Data Compact**: Only include necessary fields
3. **Reuse Buffers**: For high-frequency publishing, consider reusing buffers
4. **Check Data Size**: Be aware of message sizes for complex structures

### Thread Safety

Remember that serialization/deserialization might occur on different threads:

1. **Audio Thread**: Avoid serialization on the audio thread
2. **Worker Thread**: `onMCPMessage()` is called on a worker thread
3. **Thread-Safe Access**: Use thread-safe access when updating shared state

## Debugging Serialization

When debugging serialization issues:

1. **Verify Data Types**: Ensure your data matches the expected types
2. **Check JSON Format**: For complex structures, try using JSON format for debugging (it's human-readable)
3. **Log Data**: Log serialized data sizes and types
4. **Test Round-Trip**: Test serialization followed by deserialization

## Using Serialization with RingBuffers

When passing serialized data to the audio thread, use ring buffers:

```cpp
void onMCPMessage(const mcp::MCPMessage_V1* message) override {
    try {
        if (message->topic == "other-module/parameter1") {
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
```

## Complete Example

Here's a complete example of serialization and deserialization for a custom structure:

```cpp
#include "mcp/MCPSerialization.h"

// Define a custom data structure
struct SynthPreset {
    std::string name;
    float cutoff;
    float resonance;
    float attack;
    float decay;
    float sustain;
    float release;
    std::vector<float> modMatrix;
};

// Implement serialization support
namespace mcp {
namespace serialization {
    
    template<>
    msgpack11::MsgPack convertToMsgPack<SynthPreset>(const SynthPreset& preset) {
        std::map<std::string, msgpack11::MsgPack> m;
        m["version"] = msgpack11::MsgPack(1);
        m["name"] = msgpack11::MsgPack(preset.name);
        m["cutoff"] = msgpack11::MsgPack(preset.cutoff);
        m["resonance"] = msgpack11::MsgPack(preset.resonance);
        m["attack"] = msgpack11::MsgPack(preset.attack);
        m["decay"] = msgpack11::MsgPack(preset.decay);
        m["sustain"] = msgpack11::MsgPack(preset.sustain);
        m["release"] = msgpack11::MsgPack(preset.release);
        
        msgpack11::MsgPack::array modMatrix;
        for (float val : preset.modMatrix) {
            modMatrix.push_back(msgpack11::MsgPack(val));
        }
        m["modMatrix"] = msgpack11::MsgPack(modMatrix);
        
        return msgpack11::MsgPack(m);
    }
    
    template<>
    SynthPreset convertFromMsgPack<SynthPreset>(const msgpack11::MsgPack& msgpack) {
        SynthPreset preset;
        if (!msgpack.is_object()) {
            throw MCPSerializationError("Expected object in MessagePack data");
        }
        
        auto obj = msgpack.object_items();
        
        // Version check
        int version = 1;
        if (obj.find("version") != obj.end()) {
            version = obj["version"].int_value();
        }
        
        // Parse based on version
        preset.name = obj["name"].string_value();
        preset.cutoff = static_cast<float>(obj["cutoff"].number_value());
        preset.resonance = static_cast<float>(obj["resonance"].number_value());
        preset.attack = static_cast<float>(obj["attack"].number_value());
        preset.decay = static_cast<float>(obj["decay"].number_value());
        preset.sustain = static_cast<float>(obj["sustain"].number_value());
        preset.release = static_cast<float>(obj["release"].number_value());
        
        if (obj.find("modMatrix") != obj.end() && obj["modMatrix"].is_array()) {
            auto arr = obj["modMatrix"].array_items();
            preset.modMatrix.reserve(arr.size());
            for (const auto& item : arr) {
                preset.modMatrix.push_back(static_cast<float>(item.number_value()));
            }
        }
        
        return preset;
    }
}}

// Example usage
void publishPreset(SynthPreset preset) {
    auto broker = mcp::getMCPBroker();
    if (!broker) return;
    
    try {
        auto message = mcp::serialization::createMsgPackMessage(
            "synth/preset",
            this->id,
            preset
        );
        broker->publish(message);
    }
    catch (const mcp::MCPSerializationError& e) {
        // Handle error
    }
}
```

## Next Steps

- Explore the [Thread Safety Guide](thread_safety.md) for passing data between threads
- Review the [Provider Implementation Guide](provider_implementation.md) for publishing data
- Check the [Subscriber Implementation Guide](subscriber_implementation.md) for receiving data 