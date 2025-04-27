# VCV Rack Model Context Protocol (MCP)

## Purpose & Vision
The VCV Rack Model Context Protocol (MCP) is a lightweight communication framework enabling modules to exchange contextual data in a standardized way. It allows modules to share and react to information like musical key, tempo, chord progressions, sequence patterns, and visual theming without requiring direct connections via patch cables.

## Problems Solved
1. **Contextual information sharing**: Allows modules to exchange non-audio information without consuming panel space for additional inputs/outputs
2. **Standardized communication**: Provides a consistent API for all modules to publish and subscribe to shared data
3. **Decoupled integration**: Modules can interact without directly depending on each other
4. **Enhanced user experience**: Reduces patch cable clutter and simplifies complex patches
5. **Extended module capabilities**: Enables new creative possibilities through contextual awareness

## Core Requirements
1. **Minimal overhead**: Negligible CPU and memory impact
2. **Real-time safe**: Zero impact on audio processing
3. **Thread safety**: Proper handling of concurrent access
4. **Ease of adoption**: Simple API that's easy for module developers to implement
5. **Versioned interfaces**: Clear upgrade path for future enhancements
6. **Typed data**: Support for various data types with serialization

## User Experience Goals
1. **Transparent operation**: The protocol should work "behind the scenes" with minimal user configuration
2. **Discoverability**: Users should be able to easily find which modules support which contexts
3. **Flexibility**: Support for both simple use cases and complex interactions
4. **Reliability**: Stable operation without crashes or glitches
5. **Extensibility**: The ability to define new context types as needed

## Implementation Strategy
The MCP project is being developed in four phases:

### Phase 1: Core Framework (COMPLETED)
- ✅ Interface definitions for broker, provider, and subscriber
- ✅ Central broker implementation with context registry
- ✅ Registration and discovery mechanisms
- ✅ Basic thread safety
- ✅ Unit tests

### Phase 2: Message Exchange (COMPLETED)
- ✅ Message structure definition
- ✅ Topic-based subscription
- ✅ Asynchronous message dispatch
- ✅ Data serialization support (msgpack11)
- ✅ Thread-safe message handling
- ✅ End-to-end workflow example
- ✅ Comprehensive test coverage

### Phase 3: Reference Implementation & Docs (IN PROGRESS)
- 🔄 Reference provider module implementation
- 🔄 Reference subscriber module implementation
- 🔄 Thread-safe audio thread integration
- 🔄 Integration examples
- 🔄 API documentation
- 🔄 Developer's guide

### Phase 4: Integration & Refinement (PLANNED)
- ⬜ Integration with VCV Rack API
- ⬜ Performance optimization
- ⬜ Additional serialization formats
- ⬜ Extended example modules
- ⬜ Community documentation

## Current Focus
We are currently implementing reference provider and subscriber modules that demonstrate the complete MCP workflow. The serialization implementation is now fully functional, supporting various data types through the msgpack11 library. We've created a working serialization example that shows the complete end-to-end flow from registration to publishing and receiving messages with proper cleanup.

Now we're focusing on thread-safe data passing between the worker thread (where messages are received) and the audio thread (where VCV Rack modules process signals), which is a critical aspect for proper integration in the VCV Rack environment. 