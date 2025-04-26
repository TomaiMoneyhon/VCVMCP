# VCV Rack MCP - System Patterns

## System Architecture

The MCP architecture is based on a **Hybrid Broker/Interface Model** with the following key components:

### 1. Central Broker (`IMCPBroker`)

A globally accessible, thread-safe singleton object responsible for:
- Managing the registry of topics and their providers
- Handling subscription registration and management
- Dispatching published messages to relevant subscribers

```
┌───────────────────────────────────────────┐
│                IMCPBroker                 │
├───────────────────────────────────────────┤
│ - topicRegistry: map<string, providers>   │
│ - subscriptions: map<topic, subscribers>  │
│ - messageQueue: queue<MCPMessage_V1>      │
│                                           │
│ + registerContext(topic, provider)        │
│ + unregisterContext(topic, provider)      │
│ + subscribe(topic, subscriber)            │
│ + unsubscribe(topic, subscriber)          │
│ + publish(message)                        │
│ + getAvailableTopics()                    │
│ + findProviders(topic)                    │
├───────────────────────────────────────────┤
│               Thread Safety                │
└───────────────────────────────────────────┘
```

### 2. Provider Interface (`IMCPProvider_V1`)

Implemented by modules that provide contextual information:
- Registers topics they provide with the broker
- Publishes updates when data changes

```
┌───────────────────────────────────────────┐
│            IMCPProvider_V1                │
├───────────────────────────────────────────┤
│ + getProvidedTopics()                     │
└───────────────────────────────────────────┘
```

### 3. Subscriber Interface (`IMCPSubscriber_V1`)

Implemented by modules that consume contextual information:
- Subscribes to topics of interest
- Receives updates via callbacks

```
┌───────────────────────────────────────────┐
│           IMCPSubscriber_V1               │
├───────────────────────────────────────────┤
│ + onMCPMessage(message)                   │
└───────────────────────────────────────────┘
```

### 4. Message Structure (`MCPMessage_V1`)

Defines the format for message exchange:
- Topic identifier
- Sender module ID
- Data format identifier
- Serialized data payload

```
┌───────────────────────────────────────────┐
│              MCPMessage_V1                │
├───────────────────────────────────────────┤
│ + topic: string                           │
│ + senderModuleId: int                     │
│ + dataFormat: string                      │
│ + data: shared_ptr<void>                  │
│ + dataSize: size_t                        │
└───────────────────────────────────────────┘
```

## Key Technical Decisions

### 1. Communication Pattern: Publish/Subscribe

The MCP uses a publish/subscribe pattern where:
- Providers publish updates to the broker
- The broker dispatches updates to all subscribed modules
- This asynchronous pattern decouples providers and subscribers

### 2. Thread Safety & Real-time Audio Considerations

- The broker uses appropriate locking (mutex) to ensure thread-safe registry updates
- Message dispatch occurs off the audio thread via worker threads
- Communication between MCP worker threads and audio threads in modules uses lock-free ring buffers
- API functions called from the audio thread must be non-blocking and have predictable execution time

### 3. Data Serialization

- MessagePack is the primary serialization format for its efficiency and compact representation
- JSON is supported as a secondary format for human-readable debugging
- Format identification in message headers allows for potential future formats

### 4. Lifecycle Management

- Module addition/removal is integrated with VCV Rack lifecycle events
- Proper cleanup occurs when modules are removed
- Weak references prevent memory leaks and dangling pointers

## Component Relationships

```
┌─────────────────┐        registers topics       ┌──────────────┐
│ Provider Module │◄─────────────────────────────►│  MCP Broker  │
└────────┬────────┘                               └──────┬───────┘
         │                                               │
         │ publishes                                     │ dispatches
         │ updates                                       │ to subscribers
         │                                               │
         │                                               ▼
         │                          subscribes    ┌──────────────┐
         └─────────────────────────────────────► │   Subscriber  │
                                                  └──────────────┘
```

## Design Patterns in Use

### 1. Singleton Pattern

The `IMCPBroker` is implemented as a singleton to ensure a single, globally accessible instance for centralized topic and subscription management.

### 2. Observer Pattern

The core of the MCP is based on the Observer pattern:
- Provider modules are subjects that notify of changes
- Subscriber modules are observers that receive notifications
- The broker manages the observer registration and notification dispatch

### 3. Publish/Subscribe Pattern

A variation of the Observer pattern where:
- Observers subscribe to specific topics rather than specific subjects
- The broker handles message routing based on topics
- Subjects (providers) are decoupled from the specific observers (subscribers)

### 4. Interface Segregation

The system uses distinct interfaces for different roles:
- `IMCPProvider_V1` for modules that provide data
- `IMCPSubscriber_V1` for modules that consume data
- This allows modules to implement only what they need

### 5. Thread-safe Queue Pattern

For worker thread communication with the audio thread:
- Lock-free ring buffers (`rack::dsp::RingBuffer`) ensure real-time safe communication
- Publishing happens on any thread but processing occurs off the audio thread
- This ensures audio processing is not disrupted by MCP operations 