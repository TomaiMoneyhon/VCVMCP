# VCV Rack MCP - Technical Context

## Technologies Used

### Development Environment

- **C++**: Primary implementation language (C++11/14 for compatibility with VCV Rack)
- **VCV Rack SDK**: Base framework for integrating with VCV Rack
- **CMake**: Build system integration
- **Git**: Version control
- **GitHub**: Repository hosting and project management

### Core Libraries

- **MessagePack**: Primary serialization format for efficient binary data exchange
  - C++ implementation: `msgpack-c`
  - Provides compact, efficient binary serialization with defined schemas
  - Faster and smaller than JSON for binary data

- **JSON**: Secondary serialization option for human-readable debugging
  - Implementation: `nlohmann/json` (header-only)
  - Used primarily for development and debugging

- **VCV Rack Components**:
  - `rack::dsp::RingBuffer`: For thread-safe communication between worker and audio threads
  - VCV Rack Module lifecycle hooks (onAdd, onRemove)
  - VCV Rack threading model integration

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
   - MessagePack C++ (`msgpack-c`, included or as submodule)
   - JSON library (header-only, included or as submodule)

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
  - MCP must use worker threads for message dispatch

- **Thread Safety Requirements**:
  - Registry operations must be thread-safe
  - Message queue operations must be thread-safe
  - Module access to MCP from multiple threads must be handled safely

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
| MessagePack C++ | 3.3.0+ | Efficient binary serialization | Boost Software License |
| nlohmann/json | 3.10.0+ | JSON parsing/serialization for debugging | MIT |
| VCV Rack SDK | Latest | Integration with VCV Rack | VCV Rack License |
| Google Test | 1.10.0+ | Unit testing framework | BSD 3-Clause |

### Internal Dependencies

- **Core MCP Components**:
  - `IMCPBroker`: Central broker singleton
  - `IMCPProvider_V1`: Provider interface
  - `IMCPSubscriber_V1`: Subscriber interface
  - `MCPMessage_V1`: Message structure

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