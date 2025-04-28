# MCP Thread Safety Guide

## Overview

The VCV Rack Model Context Protocol (MCP) is designed to operate in a multi-threaded environment. Understanding the threading model and ensuring proper thread safety is critical for creating stable, performant modules that use MCP.

## VCV Rack Threading Model

VCV Rack operates with several distinct threads:

1. **Audio Thread**: Processes audio and calls the `process()` method of each module. This thread has strict real-time requirements.
2. **UI Thread**: Handles user interface events and rendering.
3. **Worker Threads**: Used for non-real-time operations, including MCP message dispatch.

## MCP Threading Model

The MCP system follows these key principles:

1. **Non-blocking Audio Thread**: All operations that might block (like message dispatch) are performed off the audio thread.
2. **Thread-Safe Registration**: The broker uses mutex locks to ensure thread-safe registration, unregistration, and discovery.
3. **Worker Thread Dispatch**: Messages are dispatched to subscribers on worker threads, not the audio thread.
4. **Ring Buffer Communication**: Modules should use thread-safe ring buffers for passing data from worker threads to the audio thread.

## Thread Safety Requirements

### Audio Thread Safety

The `process()` method of your module runs on the audio thread and must be real-time safe:

* **DO NOT** allocate memory (no `new`, `delete`, or container resizing)
* **DO NOT** call functions that might block (file I/O, network I/O, mutexes)
* **DO NOT** call `onMCPMessage()` directly from the audio thread
* **DO NOT** call `publish()` directly from the audio thread if it might block

### Accessing MCP Data in Audio Thread

To safely use MCP data in your audio thread:

1. Receive data in `onMCPMessage()` (called on worker thread)
2. Copy/transfer the data to a thread-safe ring buffer
3. Read from the ring buffer in your `process()` method

Example:

```cpp
// In your module class
dsp::RingBuffer<float, 128> m_valueBuffer;
std::atomic<bool> m_hasNewValue{false};

// In onMCPMessage (worker thread)
void onMCPMessage(const mcp::MCPMessage_V1* message) override {
    try {
        if (message->topic == "example/value") {
            float value = mcp::serialization::extractMessageData<float>(message);
            // Push to ring buffer
            m_valueBuffer.push(value);
            m_hasNewValue.store(true, std::memory_order_release);
        }
    } catch (const mcp::MCPSerializationError& e) {
        // Handle error
    }
}

// In process (audio thread)
void process(const ProcessArgs& args) override {
    // Check if we have new data
    if (m_hasNewValue.load(std::memory_order_acquire)) {
        float value;
        while (m_valueBuffer.pop(value)) {
            // Process new value
        }
        m_hasNewValue.store(false, std::memory_order_release);
    }
    
    // Continue with audio processing...
}
```

### Publishing Data Safely

To publish data safely:

1. **From Worker/UI Threads**: You can call `publish()` directly.
2. **From Audio Thread**: Queue the data and publish it from a worker thread.

For audio thread publishing, create a queue:

```cpp
// In your module class
dsp::RingBuffer<float, 128> m_publishQueue;
std::atomic<bool> m_hasDataToPublish{false};

// In process (audio thread)
void process(const ProcessArgs& args) override {
    // Generate data to publish
    float newValue = calculateValue();
    
    // Queue the data
    if (m_publishQueue.push(newValue)) {
        m_hasDataToPublish.store(true, std::memory_order_release);
    }
    
    // Continue with audio processing...
}

// In worker thread
void workerThreadFunction() {
    while (running) {
        if (m_hasDataToPublish.load(std::memory_order_acquire)) {
            float value;
            while (m_publishQueue.pop(value)) {
                // Create and publish message
                auto message = mcp::serialization::createMsgPackMessage(
                    "example/value",
                    moduleId,
                    value
                );
                broker->publish(message);
            }
            m_hasDataToPublish.store(false, std::memory_order_release);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
```

## RingBuffer Implementation

### RingBuffer Thread Safety Improvements

The MCP system's `RingBuffer` implementation has been optimized for thread safety and reliability. The current implementation is designed specifically for Single-Producer/Single-Consumer (SPSC) usage patterns, which is the recommended approach in the MCP ecosystem.

#### Key Features of the Improved RingBuffer:

1. **SPSC Design**: Optimized for one producer thread and one consumer thread.
2. **Strong Memory Ordering**: Uses sequential consistency for all atomic operations.
3. **Explicit Memory Barriers**: Contains barriers at critical points to ensure visibility.
4. **High Performance**: Achieves ~980,000 messages/second throughput with perfect reliability.

#### Proper Usage Pattern:

```cpp
// In your module class
mcp::RingBuffer<float> m_valueBuffer{64}; // Size based on expected max message rate

// In onMCPMessage (producer thread)
void onMCPMessage(const mcp::MCPMessage_V1* message) override {
    float value = mcp::serialization::extractMessageData<float>(message);
    
    // Producer thread is the only thread that calls push()
    if (!m_valueBuffer.push(value)) {
        // Handle buffer full condition - log or discard
    }
}

// In process (consumer thread)
void process(const ProcessArgs& args) override {
    float value;
    
    // Consumer thread is the only thread that calls pop()
    while (m_valueBuffer.pop(value)) {
        // Process each value
    }
}
```

#### Best Practices for RingBuffer Usage:

1. **Follow SPSC Pattern**: 
   - One thread (typically worker thread) should be the exclusive producer.
   - One thread (typically audio thread) should be the exclusive consumer.
   - Never have multiple threads calling `push()` or multiple threads calling `pop()`.

2. **Properly Size the Buffer**: 
   - Buffer size should be based on the expected maximum message rate.
   - Recommended size: 64-128 elements for most modules.
   - Too small: Messages may be lost during high-frequency publishing.
   - Too large: Unnecessary memory consumption.

3. **Handle Buffer Full Conditions**: 
   - Always check the return value of `push()` to handle buffer full conditions.
   - Worker thread should not block if the buffer is full.
   - Implement appropriate error handling or fallback behavior.

4. **Clear Buffers at Appropriate Times**:
   - The `clear()` method is not thread-safe.
   - Only call `clear()` when no other threads are accessing the buffer.
   - Typically used during initialization or reset operations.

5. **One Buffer Per Data Stream**:
   - Use separate buffers for different types of data.
   - Do not mix different message types in the same buffer.

## Thread Safety Best Practices

1. **Minimize Locking**: Avoid locks in the audio thread.
2. **Use Standard Patterns**: Follow the ring buffer pattern for thread communication.
3. **Keep Messages Small**: Large messages take longer to process and serialize.
4. **Check Return Values**: Check ring buffer push/pop return values to handle overflow/underflow.
5. **Use Memory Ordering**: Use appropriate memory ordering in atomic operations.
6. **Test Under Load**: Test your module with high message frequencies and heavy CPU load.

## Thread-Safe Resource Management

When managing resources across threads:

1. **Use Weak References**: The broker uses `std::weak_ptr` to avoid dangling references.
2. **Cleanup on Removal**: Call `unsubscribeAll()` and `unregisterContext()` in `onRemove()`.
3. **Avoid Deletion While Processing**: Ensure objects aren't deleted while processing messages.

## Thread Safety Testing

To test thread safety in your module:

1. **Stress Tests**: Send many messages with multiple providers/subscribers.
2. **Race Condition Tests**: Create scenarios with concurrent registration/publishing.
3. **Cleanup Tests**: Test proper cleanup when modules are hot-swapped.

## Additional Resources

- [C++ Memory Model and Atomic Operations](https://en.cppreference.com/w/cpp/atomic/memory_order)
- [VCV Rack Real-time Audio Programming Guide](https://vcvrack.com/manual/DSP)
- [Lock-free Programming Guide](https://www.internalpointers.com/post/lock-free-multithreading-atomic-operations) 