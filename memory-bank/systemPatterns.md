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
│ - workerThread: thread                    │
│                                           │
│ + registerContext(topic, provider)        │
│ + unregisterContext(topic, provider)      │
│ + subscribe(topic, subscriber)            │
│ + unsubscribe(topic, subscriber)          │
│ + publish(message)                        │
│ + getAvailableTopics()                    │
│ + findProviders(topic)                    │
├───────────────────────────────────────────┤
│      Thread Safety & Asynchronous Dispatch │
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

### 5. Worker Thread and Message Queue

Handles asynchronous message processing:
- Processes messages off the audio thread
- Ensures real-time safety for audio processing
- Manages subscriber notification

```
┌───────────────────────────────────────────┐
│          Worker Thread System             │
├───────────────────────────────────────────┤
│ - messageQueue: queue<MCPMessage_V1>      │
│ - queueMutex: mutex                       │
│ - queueCondition: condition_variable      │
│ - threadRunning: atomic<bool>             │
│                                           │
│ + processMessageQueue()                   │
│ + deliverMessage(message)                 │
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
- Message dispatch occurs off the audio thread via a worker thread
- Communication between MCP worker thread and audio threads in modules uses lock-free ring buffers
- API functions called from the audio thread must be non-blocking and have predictable execution time
- The message queue is protected by a mutex to ensure thread-safe access
- A condition variable is used for efficient worker thread waiting

### 3. Message Queue Operation

- **Publishing**: Thread-safe enqueuing of messages
  - Acquires queue mutex
  - Adds message to the queue
  - Notifies worker thread using condition variable
  - Returns immediately (non-blocking)

- **Processing**: Worker thread message handling
  - Waits efficiently using condition variable
  - Processes messages in FIFO order
  - Delivers to subscribers without holding locks
  - Catches and handles exceptions

### 4. Data Serialization

- MessagePack is the primary serialization format for its efficiency and compact representation
- JSON is supported as a secondary format for human-readable debugging
- Format identification in message headers allows for potential future formats

### 5. Lifecycle Management

- Module addition/removal is integrated with VCV Rack lifecycle events
- Proper cleanup occurs when modules are removed
- Weak references prevent memory leaks and dangling pointers
- Worker thread is cleanly terminated when the broker is destroyed

## Component Relationships and Data Flow

```
┌─────────────────┐        registers topics       ┌──────────────┐
│ Provider Module │◄─────────────────────────────►│  MCP Broker  │
└────────┬────────┘                               └──────┬───────┘
         │                                               │
         │ publishes                                     │ queues in
         │ messages                                      │ message queue
         │                                               │
         │                                               ▼
         │                                        ┌──────────────┐
         └─────────────────────────────────────► │ Worker Thread │
                                                  └──────┬───────┘
                                                         │
                                                         │ dispatches to
                                                         │ subscribers
                                                         │
                                                         ▼
                                                  ┌──────────────┐
                                                  │  Subscribers │
                                                  └──────────────┘
```

### Message Flow Sequence

```
┌───────────┐          ┌──────────┐          ┌───────────┐          ┌────────────┐
│  Provider │          │  Broker  │          │  Worker   │          │ Subscriber │
└─────┬─────┘          └────┬─────┘          └─────┬─────┘          └──────┬─────┘
      │                     │                      │                        │
      │ publish(message)    │                      │                        │
      │────────────────────►│                      │                        │
      │                     │                      │                        │
      │                     │ queue message        │                        │
      │                     │◄───────────────────► │                        │
      │                     │                      │                        │
      │                     │ notify()             │                        │
      │                     │────────────────────► │                        │
      │                     │                      │                        │
      │                     │                      │ process message        │
      │                     │                      │◄─────────────────────► │
      │                     │                      │                        │
      │                     │                      │ onMCPMessage(message)  │
      │                     │                      │────────────────────────►
      │                     │                      │                        │
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

### 4. Producer/Consumer Pattern

For message queue processing:
- Providers (and broker) act as producers, adding to the queue
- Worker thread acts as the consumer, processing messages
- Condition variable synchronizes the producer and consumer

### 5. Thread-safe Queue Pattern

For worker thread communication with the audio thread:
- Lock-free ring buffers (`rack::dsp::RingBuffer`) ensure real-time safe communication
- Publishing happens on any thread but processing occurs off the audio thread
- This ensures audio processing is not disrupted by MCP operations

### 6. Interface Segregation

The system uses distinct interfaces for different roles:
- `IMCPProvider_V1` for modules that provide data
- `IMCPSubscriber_V1` for modules that consume data
- This allows modules to implement only what they need 

## Core Architectural Patterns

### 1. Hybrid Broker/Interface Model

The MCP system uses a hybrid approach combining:

- **Central Broker** (Singleton): Manages registration, discovery, and message dispatch
- **Defined Interfaces**: Implemented by modules to participate (IMCPProvider_V1, IMCPSubscriber_V1)
- **Publish/Subscribe Pattern**: For communication between modules

This hybrid design provides:
- Central coordination via the broker
- Decoupling between providers and subscribers
- Flexibility for modules to act as both providers and subscribers

### 2. Thread-Safe Communication

The MCP system includes several patterns for thread safety:

- **Worker Thread Dispatch**: Message delivery occurs on worker threads, not the audio thread
- **Lock-Free Ring Buffers**: Used for passing data from worker threads to the audio thread
- **Mutex Protection**: For broker registry operations
- **Weak References**: Used for provider/subscriber lifecycle management
- **Atomic Operations**: For signaling between threads

### 3. Topic-Based Message Routing

Messages are routed using a topic-based system:

- **Topics as Channels**: Each topic represents a distinct channel of communication
- **Topic Registry**: The broker maintains a mapping of topics to provider modules
- **Subscription-Based Delivery**: Subscribers only receive messages for topics they subscribe to
- **Message Filtering**: Subscribers can filter messages based on topic and other criteria

## Topic System Patterns

### 1. URI-Style Topic Naming Convention

Topics follow a standardized URI-style format with two parts:

```
namespace/resource
```

Key patterns:
- **Reverse Domain Notation**: Namespaces use reverse domain notation for uniqueness (e.g., `com.vcvrack.core`)
- **Lowercase Hyphenated Resources**: Resources use lowercase with hyphens for multi-word names
- **Version Indicators**: Version numbers in resource names for breaking changes (e.g., `clock-v2`)
- **Reserved Namespaces**: Specific namespaces reserved for system use (`mcp.system/*`)

### 2. Hierarchical Topic Organization

Topics are organized into a logical hierarchy:

- **System Topics**: Core MCP functionality (`mcp.system/broker-status`)
- **Core VCV Topics**: Standard VCV Rack features (`com.vcvrack.core/clock`)
- **Vendor Topics**: Vendor-specific functionality (`com.company.product/feature`)
- **Module-Specific Topics**: Topics specific to a particular module (`com.company.product/module-feature`)
- **Functional Categories**: Topics grouped by functionality (clock, MIDI, modulation, etc.)

### 3. Standard Message Structure

Messages follow a consistent structure:

```json
{
  "type": "message-type",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.module-name",
  "data": {
    // Message-specific payload
  }
}
```

Key components:
- **Type**: Identifies the message type within the topic context
- **Version**: Schema version for compatibility
- **Timestamp**: When the message was created
- **Source**: Identifier of the sender
- **Data**: Type-specific payload

### 4. Registration Patterns

Different topic types follow different registration patterns:

- **Standard Topics**: Registered through official documentation process
- **Vendor-Specific Topics**: Registered by vendors in their namespace
- **Private Topics**: Used internally without formal registration
- **Community Topics**: Registered in the community repository

### 5. Message Dispatch Patterns

The system uses several patterns for message delivery:

- **Worker Thread Dispatch**: Messages are delivered on worker threads, not the audio thread
- **Ring Buffer Communication**: Thread-safe passing of data to the audio thread
- **Value Filtering**: Only sending messages when values change significantly
- **Message Throttling**: Limiting message frequency for high-update scenarios
- **Batching**: Combining multiple related messages into a single update

## Implementation Patterns

### 1. Provider Implementation Pattern

```cpp
class MyProvider : public rack::Module, public mcp::IMCPProvider_V1,
                 public std::enable_shared_from_this<MyProvider> {
public:
    // IMCPProvider_V1 implementation
    std::vector<std::string> getProvidedTopics() const override {
        return {"com.example.my-module/my-topic"};
    }
    
    void onAdd() override {
        rack::Module::onAdd();
        auto broker = mcp::getMCPBroker();
        if (broker) {
            auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(shared_from_this());
            broker->registerContext("com.example.my-module/my-topic", providerPtr);
        }
    }
    
    void onRemove() override {
        auto broker = mcp::getMCPBroker();
        if (broker) {
            auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(shared_from_this());
            broker->unregisterContext("com.example.my-module/my-topic", providerPtr);
        }
        rack::Module::onRemove();
    }
    
    void publishValue(float value) {
        auto broker = mcp::getMCPBroker();
        if (!broker) return;
        
        try {
            auto message = mcp::serialization::createMsgPackMessage(
                "com.example.my-module/my-topic",
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

### 2. Subscriber Implementation Pattern

```cpp
class MySubscriber : public rack::Module, public mcp::IMCPSubscriber_V1,
                    public std::enable_shared_from_this<MySubscriber> {
public:
    // Ring buffer for thread-safe communication
    rack::dsp::RingBuffer<float, 16> m_valueBuffer;
    std::atomic<bool> m_hasNewValue{false};
    
    // IMCPSubscriber_V1 implementation
    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        if (!message) return;
        
        try {
            if (message->topic == "com.example.other-module/other-topic") {
                float value = mcp::serialization::extractMessageData<float>(message);
                
                // Thread-safe passing to audio thread
                m_valueBuffer.push(value);
                m_hasNewValue.store(true, std::memory_order_release);
            }
        }
        catch (const mcp::MCPSerializationError& e) {
            // Handle error
        }
    }
    
    void onAdd() override {
        rack::Module::onAdd();
        auto broker = mcp::getMCPBroker();
        if (broker) {
            auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(shared_from_this());
            broker->subscribe("com.example.other-module/other-topic", subscriberPtr);
        }
    }
    
    void onRemove() override {
        auto broker = mcp::getMCPBroker();
        if (broker) {
            auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(shared_from_this());
            broker->unsubscribeAll(subscriberPtr);
        }
        rack::Module::onRemove();
    }
    
    void process(const ProcessArgs& args) override {
        // Check for new values from the worker thread
        if (m_hasNewValue.load(std::memory_order_acquire)) {
            float value;
            while (m_valueBuffer.pop(value)) {
                // Process the value
                // ...
            }
            m_hasNewValue.store(false, std::memory_order_release);
        }
        
        // Continue with audio processing
        // ...
    }
};
```

### 3. Throttling and Filtering Pattern

```cpp
// Last sent values for filtering
float m_lastSentValue = 0.f;
float m_minValueChange = 0.01f;  // Minimum change to trigger an update

// Last send time for throttling
int64_t m_lastSendTime = 0;
int64_t m_minSendInterval = 100;  // Minimum ms between updates

void maybePublishValue(float newValue) {
    // Current time
    int64_t currentTime = getCurrentTimeMs();
    
    // Value filtering - only send if changed significantly
    if (std::abs(newValue - m_lastSentValue) < m_minValueChange) {
        return;
    }
    
    // Time throttling - only send if enough time has passed
    if (currentTime - m_lastSendTime < m_minSendInterval) {
        return;
    }
    
    // Update tracking variables
    m_lastSentValue = newValue;
    m_lastSendTime = currentTime;
    
    // Publish the value
    publishValue(newValue);
}
```

### 4. Message Batching Pattern

```cpp
// Collection of pending events
std::vector<NoteEvent> m_pendingEvents;
int64_t m_lastBatchTime = 0;
int64_t m_batchInterval = 50;  // Max ms between batch sends

void addNoteEvent(const NoteEvent& event) {
    m_pendingEvents.push_back(event);
    
    // Send batch immediately if it's getting large
    if (m_pendingEvents.size() >= 10) {
        sendEventBatch();
        return;
    }
    
    // Send batch if enough time has passed
    int64_t currentTime = getCurrentTimeMs();
    if (currentTime - m_lastBatchTime >= m_batchInterval) {
        sendEventBatch();
    }
}

void sendEventBatch() {
    if (m_pendingEvents.empty()) return;
    
    auto broker = mcp::getMCPBroker();
    if (!broker) return;
    
    try {
        auto message = mcp::serialization::createMsgPackMessage(
            "com.example.my-module/note-events",
            this->id,
            m_pendingEvents
        );
        broker->publish(message);
        
        // Clear the batch
        m_pendingEvents.clear();
        m_lastBatchTime = getCurrentTimeMs();
    }
    catch (const mcp::MCPSerializationError& e) {
        // Handle error
    }
}
```

### 3. RingBuffer Usage Pattern

The `RingBuffer` has been optimized for Single-Producer/Single-Consumer (SPSC) scenarios, which matches the most common use case in MCP modules. Following this pattern is critical for thread safety:

```cpp
// In a typical module class:
class ExampleModule : public rack::Module, public IMCPSubscriber_V1 {
private:
    // Ring buffer for thread-safe communication with audio thread
    mcp::RingBuffer<float> m_valueBuffer{64}; // Size should be power of 2 for best performance
    std::atomic<bool> m_hasNewData{false};
    
    // Worker thread context (called by MCP broker)
    void onMCPMessage(const mcp::MCPMessage_V1* message) override {
        if (!message) return;
        
        try {
            // Extract data from message
            float value = mcp::serialization::extractMessageData<float>(message);
            
            // PRODUCER ROLE: Only the worker thread should push to the buffer
            if (m_valueBuffer.push(value)) {
                // Signal to audio thread that new data is available
                m_hasNewData.store(true, std::memory_order_release);
            } else {
                // Buffer full - can log or handle as needed
                // In practice, this usually means the audio thread isn't 
                // consuming fast enough or is paused
            }
        } catch (const std::exception& e) {
            // Handle errors
        }
    }
    
public:
    // Audio thread context
    void process(const ProcessArgs& args) override {
        // CONSUMER ROLE: Only the audio thread should pop from the buffer
        if (m_hasNewData.load(std::memory_order_acquire)) {
            float value;
            while (m_valueBuffer.pop(value)) {
                // Process the value as needed
                // ...
            }
            
            // Reset flag after consuming all available data
            m_hasNewData.store(false, std::memory_order_release);
        }
        
        // Continue with normal audio processing
        // ...
    }
};
```

Key best practices:
1. Use a single producer thread (typically the worker thread via `onMCPMessage`)
2. Use a single consumer thread (typically the audio thread via `process()`)
3. Use a notification mechanism (like `std::atomic<bool>`) to signal new data
4. Clear the buffer only when no other threads are accessing it (typically in `onRemove()`)
5. Size the buffer appropriately for your use case (64-128 elements is typical)
6. Handle buffer-full conditions gracefully in the producer
7. Consume all available data in the consumer before resetting the notification flag

For higher performance needs (rare in typical modules), adjust these parameters:
- Increase buffer size to reduce the chance of buffer-full conditions
- Consider using exponential backoff when the buffer is full instead of immediately returning
- In extremely high-throughput scenarios, consider batching multiple values into a single buffer entry

## Best Practice Patterns

### 1. Graceful Degradation Pattern

```cpp
void process(const ProcessArgs& args) override {
    // Try to get value from subscription
    float value = 0.0f;
    bool hasValidValue = false;
    
    if (m_hasNewValue.load(std::memory_order_acquire)) {
        while (m_valueBuffer.pop(value)) {
            hasValidValue = true;
        }
        m_hasNewValue.store(false, std::memory_order_release);
    }
    
    // Fallback to manual input if no subscription
    if (!hasValidValue) {
        value = params[VALUE_PARAM].getValue();
    }
    
    // Process with the value, regardless of source
    outputs[VALUE_OUTPUT].setVoltage(value * 10.f);
}
```

### 2. Safe Cleanup Pattern

```cpp
void onRemove() override {
    // Unregister all topics and unsubscribe from all topics
    auto broker = mcp::getMCPBroker();
    if (broker) {
        // As provider
        auto providerPtr = std::dynamic_pointer_cast<mcp::IMCPProvider_V1>(shared_from_this());
        for (const auto& topic : getProvidedTopics()) {
            broker->unregisterContext(topic, providerPtr);
        }
        
        // As subscriber
        auto subscriberPtr = std::dynamic_pointer_cast<mcp::IMCPSubscriber_V1>(shared_from_this());
        broker->unsubscribeAll(subscriberPtr);
    }
    
    // Cancel any pending operations
    m_workerActive.store(false, std::memory_order_release);
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    
    // Clear any buffers
    float dummy;
    while (m_valueBuffer.pop(dummy)) {}
    
    rack::Module::onRemove();
}
```

These patterns form the foundation of the MCP system and provide a consistent approach to implementing providers and subscribers with proper thread safety, lifecycle management, and performance considerations. 