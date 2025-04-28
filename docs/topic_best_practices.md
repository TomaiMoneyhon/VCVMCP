# MCP Topic System Best Practices

This document outlines recommended practices for effectively using the MCP topic system in your modules.

## General Principles

### Topic Selection

1. **Use Standard Topics When Possible**
   - Before creating a custom topic, check if a [standard topic](standard_topics.md) exists
   - Using standard topics improves interoperability with other modules

2. **Create Focused Topics**
   - Each topic should serve a specific, well-defined purpose
   - Avoid "catch-all" topics that mix unrelated functionality

3. **Consider Scope and Audience**
   - Use public topics for cross-module communication
   - Use private topics for internal communication within your modules
   - Consider which modules will be providers vs. subscribers

### Message Design

1. **Keep Messages Lean**
   - Include only necessary information
   - Remember messages may be processed in real-time contexts

2. **Include Context**
   - Messages should be self-contained when possible
   - Include sufficient context for proper handling

3. **Use Appropriate Data Types**
   - Use integers for discrete values (notes, steps)
   - Use floats for continuous values (voltages, frequencies)
   - Use enums (string values) for state indicators

4. **Standardize Units**
   - Use seconds for time values
   - Use Hertz for frequencies
   - Use volts for CV values (-10V to +10V range typically)
   - Be explicit when using non-standard units

## Performance Considerations

### Real-time Safety

1. **Audio Thread Awareness**
   - Messages processed in the audio thread must be real-time safe
   - Avoid memory allocation, file I/O, or complex calculations
   - Consider moving complex processing to worker threads

2. **Message Rate Control**
   - Throttle high-frequency messages (e.g., CV updates)
   - Consider sending only meaningful changes
   - Use timestamps to handle timing accurately

3. **Resource Management**
   - Keep message sizes small for efficient transport
   - Consider batching small, frequent messages
   - Use binary formats for large data sets

### Threading Model

1. **Thread Safety**
   - Design message handlers to be thread-safe
   - Be aware which thread is producing/consuming messages
   - Use appropriate synchronization mechanisms

2. **Worker Thread Processing**
   - Offload non-real-time operations to worker threads
   - Use message queues to buffer between threads
   - Consider priority and scheduling for time-sensitive operations

## Topic Lifecycle Management

### Provider Responsibilities

1. **Topic Documentation**
   - Document your topic format and semantics
   - Specify expected message rates and timing
   - Provide example code when possible

2. **Version Management**
   - Clearly indicate topic versions
   - Support older versions during transition periods
   - Document migration paths for breaking changes

3. **Quality of Service**
   - Honor the message rates you advertise
   - Include accurate timestamps in messages
   - Consider message delivery guarantees

### Subscriber Responsibilities

1. **Graceful Degradation**
   - Handle cases where expected topics aren't available
   - Provide meaningful fallback behavior
   - Don't crash if messages are malformed

2. **Resource Conservation**
   - Only subscribe to topics you actively use
   - Unsubscribe when no longer needed
   - Process messages efficiently

3. **Validation**
   - Validate incoming messages
   - Handle unexpected message formats gracefully
   - Check version compatibility

## Topic System Integration Patterns

### Clock and Transport Synchronization

1. **Central Clock Authority**
   - One module should be the authoritative clock source
   - Other modules should synchronize to this source
   - Handle clock drift and jitter appropriately

2. **Transport State Management**
   - Implement start/stop behavior consistently
   - Honor loop settings from transport controllers
   - Manage timeline position accurately

### Preset and State Management

1. **State Persistence**
   - Save and restore state reliably
   - Include version information in state data
   - Support migration of old state formats

2. **Preset Exchange**
   - Use consistent format for preset data
   - Include metadata (name, category, tags)
   - Support preset browsing/preview when possible

### Module Coordination

1. **Parameter Linking**
   - Support linking parameters across modules
   - Implement smooth transitions for linked parameters
   - Consider relative vs. absolute parameter changes

2. **Resource Sharing**
   - Share resources (samples, patterns) efficiently
   - Use references rather than duplicating large data
   - Consider access control for shared resources

## Testing and Debugging

### Topic Monitoring

1. **Debug Tools**
   - Implement or use tools to monitor topic traffic
   - Log important message events
   - Add debug visualization for complex data

2. **Performance Profiling**
   - Measure message throughput and latency
   - Identify bottlenecks in message processing
   - Optimize critical message paths

### Resilience Testing

1. **Fault Injection**
   - Test with malformed messages
   - Test with missing providers
   - Test with timing anomalies

2. **Load Testing**
   - Test with high message volumes
   - Test with many subscribers/providers
   - Test with resource constraints

## Common Pitfalls to Avoid

1. **Circular Dependencies**
   - Avoid creating circular topic dependencies between modules
   - Design for unidirectional data flow where possible

2. **Tight Coupling**
   - Don't make modules completely dependent on topics
   - Provide alternative interfaces when appropriate
   - Allow manual configuration as fallback

3. **Thread Blocking**
   - Never block the audio thread
   - Avoid locks contended between audio and worker threads
   - Use lock-free techniques for thread communication

4. **Topic Pollution**
   - Don't create unnecessary custom topics
   - Don't send excessive messages
   - Clean up unused subscriptions

5. **Ownership Confusion**
   - Clearly define ownership of shared resources
   - Document lifecycle management expectations
   - Use reference counting or similar techniques

## Example Implementations

### Basic Topic Provider
```cpp
class ClockProvider : public IMCPProvider_V1 {
public:
    void process(float sampleTime) {
        // Update clock state
        m_clockState.bpm = m_currentBPM;
        m_clockState.running = m_isRunning;
        m_clockState.beatPosition = m_beatPos;
        
        // Only send messages when something changes or periodically
        if (m_clockChanged || m_messageTimer >= 0.1f) {
            m_messageTimer = 0.f;
            sendClockUpdate();
        }
        
        m_messageTimer += sampleTime;
    }
    
private:
    void sendClockUpdate() {
        // Create JSON message
        json clockMsg;
        clockMsg["type"] = "clock-update";
        clockMsg["version"] = "1.0";
        clockMsg["timestamp"] = getCurrentTimestamp();
        clockMsg["source"] = "com.example.clock-module";
        
        json data;
        data["bpm"] = m_clockState.bpm;
        data["running"] = m_clockState.running;
        data["beatPosition"] = m_clockState.beatPosition;
        clockMsg["data"] = data;
        
        // Publish to the topic
        m_broker->publish("com.vcvrack.core/clock", clockMsg.dump());
    }
    
    struct ClockState {
        float bpm = 120.0f;
        bool running = false;
        float beatPosition = 0.f;
    };
    
    ClockState m_clockState;
    float m_currentBPM = 120.0f;
    bool m_isRunning = false;
    float m_beatPos = 0.f;
    float m_messageTimer = 0.f;
    bool m_clockChanged = false;
};
```

### Basic Topic Subscriber
```cpp
class ClockSubscriber : public IMCPSubscriber_V1 {
public:
    void initialize() {
        // Subscribe to the clock topic
        m_broker->subscribe(this, "com.vcvrack.core/clock");
    }
    
    void onMessage(const std::string& topic, const std::string& message) override {
        // Only process messages from our topic of interest
        if (topic != "com.vcvrack.core/clock") 
            return;
            
        try {
            // Parse the message
            json clockMsg = json::parse(message);
            
            // Extract the data
            if (clockMsg.contains("data")) {
                auto& data = clockMsg["data"];
                
                // Update our internal state
                m_currentBPM = data["bpm"];
                m_isRunning = data["running"];
                m_beatPosition = data["beatPosition"];
                
                // Signal that we received a new clock update
                m_clockUpdated = true;
            }
        }
        catch (const std::exception& e) {
            // Handle parsing errors gracefully
            // Don't crash on malformed messages
        }
    }
    
    void process(float sampleTime) {
        // Use the clock data in your module
        if (m_clockUpdated) {
            // Do something with the updated clock
            m_clockUpdated = false;
        }
        
        // Continue processing based on last known clock state
        if (m_isRunning) {
            // Process according to clock state
        }
    }
    
private:
    float m_currentBPM = 120.0f;
    bool m_isRunning = false;
    float m_beatPosition = 0.f;
    bool m_clockUpdated = false;
};
```

## Advanced Techniques

### Filtering and Throttling

```cpp
// Example of message throttling
if (m_lastSendTime + m_minSendInterval > currentTime) {
    // Too soon, skip this update
    return;
}
m_lastSendTime = currentTime;

// Example of value filtering
if (std::abs(m_lastSentValue - newValue) < m_minValueChange) {
    // Change too small, don't send update
    return;
}
m_lastSentValue = newValue;
```

### Batching Messages

```cpp
// Collect events
m_pendingEvents.push_back({note, velocity, timestamp});

// Periodically or when appropriate, send as batch
if (shouldSendBatch()) {
    json batchMsg;
    batchMsg["type"] = "note-batch";
    batchMsg["events"] = m_pendingEvents;
    m_broker->publish("com.example.vendor/note-events", batchMsg.dump());
    m_pendingEvents.clear();
}
```

## Conclusion

Following these best practices will help ensure your MCP topics are efficient, reliable, and compatible with the broader VCV Rack ecosystem. Remember that the topic system is designed to simplify communication between modules - use it to enhance your module's functionality while maintaining good performance and user experience.