# VCV Rack MCP API Documentation

## Introduction

The Model Context Protocol (MCP) provides a standardized system for VCV Rack 2 modules to exchange complex, structured data. This document describes the API implemented in Phase 1, covering Sprint 1 (Broker & Registration) and Sprint 2 (Subscription & Lifecycle).

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

## Thread Safety

The MCP system is designed to be thread-safe. All broker methods can be called from any thread, including the audio thread, though care should be taken to avoid performance impacts.

Key thread safety considerations:

1. Registration/unregistration can happen from any thread, including the audio thread.
2. Subscription/unsubscription can happen from any thread.
3. The `onMCPMessage` callback will be called on a worker thread, not the audio thread.
4. Modules should use thread-safe communication (like `dsp::RingBuffer`) to pass data from the `onMCPMessage` callback to the audio thread.

## Future Developments

Sprint 2 has implemented the subscription and lifecycle management functionality. In upcoming Sprints:

- Sprint 3 will define the message structure and implement serialization.
- Sprint 4 will implement the publish/receive functionality.

Until then, the `onMCPMessage` callback won't be called, as message dispatch hasn't been implemented yet. 