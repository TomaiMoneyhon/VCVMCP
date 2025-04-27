# VCV Rack MCP - Technology Context

## Development Environment

- **C++14 Standard**: The MCP is built using C++14 features to ensure compatibility with VCV Rack module development
- **CMake 3.15+**: Used for build system management
- **Google Test**: Framework for unit testing
- **msgpack11**: Lightweight MessagePack library for binary serialization

## Core Technologies

### Language & Libraries

1. **C++14**
   - Chosen to maintain compatibility with VCV Rack plugins
   - Using modern C++ features for safety and expressiveness
   - Leveraging smart pointers for memory management
   - Exception handling for error conditions
   - Templates for type-safe generic programming

2. **Standard Library**
   - `std::mutex` and `std::lock_guard` for thread synchronization
   - `std::shared_ptr` and `std::weak_ptr` for reference management
   - `std::vector` and `std::unordered_map` for data structures
   - `std::thread` and `std::condition_variable` for worker thread management
   - `std::string` for topic names and string handling

3. **msgpack11**
   - Lightweight MessagePack implementation for binary serialization
   - Modeled after json11 with a similar API pattern
   - Efficient binary format with minimal overhead
   - Support for serializing various data types (strings, numbers, arrays, maps)
   - Type-safe C++ API for easy integration

### Key Design Patterns

1. **Singleton Pattern**
   - Used for the MCPBroker to ensure a single global instance
   - Thread-safe initialization using double-checked locking
   - Accessible via getInstance() static method

2. **Publisher/Subscriber Pattern**
   - Core messaging pattern for the entire MCP system
   - Topics for message categorization
   - Multiple providers can publish to the same topic
   - Multiple subscribers can receive from the same topic

3. **Factory Methods**
   - Helper functions for creating messages and serializing data
   - Type-safe interfaces for common operations

4. **RAII (Resource Acquisition Is Initialization)**
   - Used throughout for managing resources
   - Lock guards for mutex management
   - Smart pointers for memory management

5. **Worker Thread Pattern**
   - Separate thread for message dispatch
   - Message queue for passing work to the thread
   - Condition variable for efficient waiting

## Architecture Components

### Core Interfaces

1. **IMCPBroker**
   - Central interface for the broker functionality
   - Methods for registration, subscription, and message dispatch
   - Singleton access point for the entire system

2. **IMCPProvider_V1**
   - Interface for modules that provide context information
   - Method to report available topics
   - Version-specific interface for future compatibility

3. **IMCPSubscriber_V1**
   - Interface for modules that consume context information
   - Callback method for receiving messages
   - Version-specific interface for future compatibility

### Message Structure

**MCPMessage_V1**
- Contains topic, sender ID, format information and serialized data
- Designed for efficient message passing
- Supports multiple serialization formats (currently MessagePack)
- Thread-safe shared ownership of data

### Serialization

1. **Serialization Helpers**
   - `createMsgPackMessage`: Creates a message with serialized data using MessagePack
   - `extractMessageData`: Type-safe deserialization of message data
   - Error handling for serialization/deserialization failures

2. **Supported Data Types**
   - Basic types: strings, integers, floats, booleans
   - Containers: vectors, maps
   - Custom types (via msgpack11 protocol specialization)

### Thread Management

1. **Broker Thread Safety**
   - Separate mutexes for different concerns:
     - Registry mutex for topic/provider management
     - Subscription mutex for subscriber management
     - Queue mutex for message queue access
   - Fine-grained locking to minimize contention

2. **Worker Thread**
   - Dedicated thread for message dispatch
   - Non-blocking message queue
   - Proper shutdown handling

3. **Subscriber Thread Safety**
   - Messages delivered on worker thread, not audio thread
   - Subscribers must handle thread-safety when processing messages
   - Recommended patterns: ring buffers for passing data to audio thread

## Performance Considerations

1. **Memory Management**
   - Minimal copying of data
   - Shared pointers for shared ownership
   - Weak pointers to prevent circular references
   - Efficient message passing with move semantics

2. **CPU Overhead**
   - Lock-free optimizations where possible
   - Fine-grained locking to minimize contention
   - Efficient binary serialization with msgpack11
   - No dynamic allocation in audio thread

3. **Scalability**
   - Efficient lookup in topic registries
   - O(1) subscriber lookup
   - Message filtering by topic
   - Minimal overhead per message

## Testing Approach

1. **Unit Testing**
   - Google Test framework
   - Test fixtures for common setups
   - Mocks for testing components in isolation
   - Comprehensive coverage of core functionality

2. **Thread Safety Testing**
   - Concurrent access tests
   - Race condition detection
   - Stress testing with many threads

3. **Integration Testing**
   - End-to-end workflow tests
   - Example programs demonstrating usage
   - Real-world use case testing

## Current Implementation Status

The core broker functionality is fully implemented and tested. Serialization is implemented using msgpack11 for efficient binary serialization. The messaging system is operational with a worker thread dispatching messages to subscribers.

We have created a complete serialization example demonstrating the full workflow from registration to message publishing and receipt. Thread-safe data passing from worker thread to audio thread is the current focus, which is critical for proper VCV Rack integration.

All unit tests are passing, including broker functionality, serialization, and end-to-end message flow tests.

## Technologies Used

### Development Environment

- **C++**: Primary implementation language (C++11/14 for compatibility with VCV Rack)
- **VCV Rack SDK**: Base framework for integrating with VCV Rack
- **CMake**: Build system integration
- **Git**: Version control
- **GitHub**: Repository hosting and project management

### Core Libraries

- **MessagePack**: Primary serialization format for efficient binary data exchange
  - C++ implementation: `msgpack11` (lightweight implementation)
  - Provides compact, efficient binary serialization
  - Faster and smaller than JSON for binary data
  - Header-only library for easy integration

- **JSON**: Secondary serialization option for human-readable debugging
  - Implementation: `nlohmann/json` (header-only)
  - Used primarily for development and debugging
  - Placeholder implementation currently in place for future extension

- **VCV Rack Components**:
  - `rack::dsp::RingBuffer`: For thread-safe communication between worker and audio threads
  - VCV Rack Module lifecycle hooks (onAdd, onRemove)
  - VCV Rack threading model integration

### Threading Components

- **Worker Thread**: Background thread that processes the message queue
  - Implemented using std::thread
  - Synchronization via std::mutex and std::condition_variable
  - Proper cleanup on destruction

- **Message Queue**: Thread-safe queue for message dispatch
  - Based on std::queue with mutex protection
  - Uses condition variable for efficient waiting
  - Lock-free access pattern for audio thread safety

## Development Setup

### Local Development Environment Requirements

1. **VCV Rack SDK**: Latest stable version (to be integrated with)
2. **C++ Compiler**:
   - macOS: Clang (via Xcode Command Line Tools)
   - Windows: MSVC (Visual Studio 2019+)
   - Linux: GCC 7+ or Clang

3. **Build Tools**:
   - CMake 3.15+
   - Make/Ninja/MSBuild (platform dependent)

4. **Dependencies**:
   - msgpack11 (included in external directory)
   - nlohmann/json (included in external directory)

### Development Workflow

1. **Setup**: Clone repository and initialize submodules
2. **Build**: Configure with CMake and build with native tool
3. **Test**: Run unit tests and integration tests
4. **Integration**: Test with VCV Rack by building test modules

### Testing Approach

- **Unit Testing**: Google Test framework for core functionality
- **Performance Testing**: Custom benchmarks for critical paths
- **Thread-safety Testing**: Specialized tests for concurrency
- **Integration Testing**: Reference modules tested in VCV Rack
- **Publish/Subscribe Testing**: Specific tests for message dispatch and delivery

## Technical Constraints

### Performance Constraints

- **Real-time Audio Processing**:
  - Audio thread must not be blocked by MCP operations
  - Functions called from audio thread must complete with predictable, minimal latency
  - No memory allocation in audio thread (or use pre-allocated buffers)
  - No locks contended with audio thread

- **Memory Usage**:
  - Minimal overhead for small patches
  - Reasonable scaling for large patches
  - Efficient message passing without excessive copying

### Threading Constraints

- **VCV Rack Threading Model**:
  - Primary audio thread runs the audio processing
  - UI thread for user interface updates
  - MCP worker thread for message dispatch

- **Thread Safety Requirements**:
  - Registry operations must be thread-safe
  - Message queue operations must be thread-safe
  - Module access to MCP from multiple threads must be handled safely
  - Subscriber callbacks must handle thread safety

### API Constraints

- **Versioning**:
  - API must be versioned for future compatibility
  - Interfaces must be clearly defined and stable once released

- **Ease of Use**:
  - Clear interfaces for module developers to implement
  - Minimal code required for basic usage
  - Comprehensive documentation for all API functions

### Integration Constraints

- **VCV Rack Compatibility**:
  - Must integrate with VCV Rack module lifecycle
  - Must follow VCV Rack API conventions and patterns
  - Must be compatible with VCV Rack SDK build system

## Dependencies

### External Dependencies

| Dependency | Version | Purpose | License |
|------------|---------|---------|---------|
| msgpack11 | latest | Efficient binary serialization | MIT |
| nlohmann/json | 3.10.0+ | JSON parsing/serialization for debugging | MIT |
| VCV Rack SDK | Latest | Integration with VCV Rack | VCV Rack License |
| Google Test | 1.10.0+ | Unit testing framework | BSD 3-Clause |

### Internal Dependencies

- **Core MCP Components**:
  - `IMCPBroker`: Central broker singleton
  - `IMCPProvider_V1`: Provider interface
  - `IMCPSubscriber_V1`: Subscriber interface
  - `MCPMessage_V1`: Message structure
  - `MCPSerialization`: Helper functions for serialization/deserialization

## Message Exchange Implementation

- **Publish/Subscribe System**:
  - Providers call `publish()` to send a message
  - Broker queues the message for processing
  - Worker thread dispatches messages to subscribers
  - Subscribers process messages in their `onMCPMessage()` callback

- **Message Structure**:
  - `MCPMessage_V1` contains topic, sender ID, data format, and binary data
  - `std::shared_ptr<void>` used for type-agnostic data storage
  - `DataFormat` enum specifies serialization format (MSGPACK, JSON, BINARY)

- **Serialization Helpers**:
  - Template functions for various data types
  - Type-specific conversion functions
  - Error handling via `MCPSerializationError` class
  - Message creation helpers that automatically serialize data

- **Worker Thread Operation**:
  - Thread waits on condition variable when no messages available
  - Processes messages in FIFO order from the queue
  - Catches and handles exceptions from subscribers
  - Cleans up expired weak references during delivery

## Deployment Considerations

- **Integration Path**:
  - Initially developed as a standalone extension
  - Eventually integrated into VCV Rack SDK for universal availability

- **Backward Compatibility**:
  - Should not break existing VCV Rack modules
  - Should gracefully handle modules without MCP support

- **Documentation**:
  - API reference documentation
  - Integration guide for developers
  - Example implementations 