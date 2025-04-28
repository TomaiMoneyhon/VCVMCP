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

## Known Thread Safety Issues

### RingBuffer Thread Safety

The current implementation of `RingBuffer` has a known issue in high-concurrency scenarios where:

1. Some items may be consumed multiple times
2. Some items may not be consumed at all

This issue is scheduled to be addressed in Sprint 7. Until then:

* The issue primarily appears under extreme stress testing
* For most practical use cases, the RingBuffer implementation is sufficient
* Use the following best practices to minimize potential issues:
  * Keep buffer sizes reasonably large (at least 2x expected capacity)
  * Use separate buffers for different data types
  * Use atomic flags to signal when data is available

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