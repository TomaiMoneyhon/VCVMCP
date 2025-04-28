# Introduction to the VCV Rack Model Context Protocol (MCP)

## What is MCP?

The Model Context Protocol (MCP) is a standardized system for VCV Rack 2 that allows modules to exchange complex, structured data. It supplements the existing communication mechanisms (CV/Gate signals and Expanders) by providing a more flexible way for modules to share information across a patch, regardless of their physical placement.

## Why MCP?

Traditional VCV Rack communication methods have limitations:

- **CV/Gate Signals**: Limited to single floating-point values; require explicit patching
- **Expanders**: Limited to physically adjacent modules
- **Serialization/Patch Storage**: Designed for state persistence, not real-time communication

MCP addresses these limitations by providing:

- **Structured Data Exchange**: Share complex data structures, not just single values
- **Non-adjacent Communication**: Exchange data between modules anywhere in a patch
- **Standardized Protocol**: Common interface for all modules to use
- **Discovery Mechanism**: Find modules providing specific information

## Key Concepts

### Broker

The core of the MCP system is the **Broker** - a centralized manager that facilitates communication between modules. The Broker:

- Maintains a registry of topics and their providers
- Manages subscriptions
- Handles message dispatch

Modules interact with the Broker through a well-defined API, never directly with each other.

### Topics

A **Topic** is a named channel used to categorize and route information. Topics use a hierarchical naming scheme, similar to REST endpoints:

```
category/subcategory/name
```

Examples:
- `sequencer/clock/bpm`
- `synth/voice/allocation`
- `master/theme/color`

### Providers

A **Provider** is a module that offers information through the MCP system. Providers:

- Register one or more topics with the Broker
- Publish messages containing data to their registered topics

Any module can be a provider by implementing the `IMCPProvider_V1` interface.

### Subscribers

A **Subscriber** is a module that receives information from the MCP system. Subscribers:

- Subscribe to topics of interest
- Receive messages when data is published to those topics

Any module can be a subscriber by implementing the `IMCPSubscriber_V1` interface.

### Messages

A **Message** is a data package sent through the MCP system. Each message contains:

- The topic it belongs to
- The sender's module ID
- The data format (e.g., MessagePack, JSON)
- The serialized data payload
- The data size

Messages are serialized to allow structured data to be transmitted efficiently.

## Architectural Overview

The MCP system uses a **Hybrid Broker/Interface Model**:

1. **Central Broker** (Singleton): Manages topics, subscriptions, and message dispatch
2. **Provider Interfaces**: Implemented by modules that offer data
3. **Subscriber Interfaces**: Implemented by modules that consume data
4. **Publish/Subscribe Pattern**: Data flows from providers to subscribers through the broker

This architecture provides a balance of flexibility, performance, and simplicity.

## Communication Flow

The typical communication flow in MCP:

1. **Registration**: Provider modules register topics with the Broker
2. **Discovery**: Subscriber modules query the Broker for available topics
3. **Subscription**: Subscriber modules subscribe to topics of interest
4. **Publication**: Provider modules publish data to their registered topics
5. **Dispatch**: The Broker delivers messages to interested subscribers
6. **Processing**: Subscribers process the received data

## Thread Safety

The MCP system operates in VCV Rack's multi-threaded environment:

- **Registration/Discovery**: Thread-safe operations via mutex locks
- **Message Dispatch**: Performed off the audio thread on worker threads
- **Data Transfer**: Uses thread-safe ring buffers for audio thread communication

Understanding thread safety is critical when implementing MCP-compatible modules.

## When to Use MCP

MCP is ideal for:

- Sharing complex structured data between modules
- Communicating between non-adjacent modules
- Providing patch-wide contextual information
- Implementing modules that need to discover or react to other modules

MCP complements, rather than replaces, existing communication methods:

- Use **CV/Gate** for real-time control signals
- Use **Expanders** for tightly coupled adjacent modules
- Use **MCP** for complex data and non-adjacent communication

## Getting Started

To start using MCP:

1. Explore the [implementation guides](../guides/provider_implementation.md) for providers and subscribers
2. Review the [thread safety guide](../guides/thread_safety.md) for concurrency best practices
3. Look at the [example implementations](../examples/README.md) for working code

## Next Steps

- Learn about [Topic Naming Conventions](topic_naming.md)
- Understand [Message Serialization](serialization.md)
- Explore the [API Reference](../api/README.md) 