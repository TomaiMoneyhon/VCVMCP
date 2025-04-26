# VCV Rack MCP Active Context

## Current Focus

We are now transitioning from Sprint 3 (Message Structure & Serialization) to Sprint 4 (Message Dispatch) in Phase 2 of the VCV Rack MCP development.

### Just Completed: Sprint 3 - Message Structure & Serialization
- ✅ Defined the MCPMessage_V1 struct for exchanging data between modules
- ✅ Integrated msgpack11 library for efficient binary serialization
- ✅ Implemented serialization/deserialization helper functions
- ✅ Created template functions for common data types
- ✅ Developed helper functions for message creation and data extraction
- ✅ Created comprehensive unit tests for all serialization functionality
- ✅ All tests pass successfully for broker functionality and serialization

### Up Next: Sprint 4 - Message Dispatch
- Add the publish method to the broker interface and implementation
- Create a worker thread for asynchronous message delivery
- Implement message dispatch to registered subscribers
- Ensure thread safety for message queue operations
- Add comprehensive unit tests for the dispatch system
- Update API documentation with publish/receive examples

## Recent Changes

1. Defined the MCPMessage_V1 structure with:
   - Topic string for message routing
   - Sender module ID for origin tracking
   - Data format identifier (MSGPACK/JSON/BINARY)
   - Raw data buffer (std::shared_ptr<void>) and size

2. Implemented serialization helpers:
   - MessagePack serialization using msgpack11 library
   - Template functions for various data types (strings, integers, floats, arrays)
   - Helper functions to create messages with serialized data
   - Functions to extract and deserialize data from messages

3. Created a robust testing framework:
   - Unit tests for MCPMessage_V1 structure
   - Tests for MessagePack serialization with simple types and arrays
   - Tests for message creation and data extraction
   - Tests for error handling and edge cases

## Active Decisions

1. **Message Format**: 
   - Using MessagePack as primary serialization format via msgpack11
   - Added placeholder for JSON functionality for future extension
   - MCPMessage_V1 uses std::shared_ptr<void> for flexible data storage
   - DataFormat enum identifies the serialization format used

2. **Thread Safety**:
   - Message serialization occurs outside the audio thread
   - Worker thread will handle message dispatch (upcoming in Sprint 4)
   - Lock-free approach planned for audio thread communication

3. **Error Handling**:
   - MCPSerializationError class implemented for serialization error reporting
   - Comprehensive error checking in serialization/deserialization functions
   - More extensive error handling planned for Sprint 5

## Next Considerations

1. **Worker Thread Design**:
   - How to implement efficient message dispatch?
   - What thread synchronization primitives to use?
   - How to handle thread termination gracefully?

2. **Publish API Design**:
   - Should publish be synchronous or asynchronous?
   - How to handle delivery failures?
   - What guarantees should the API provide about delivery?

3. **Testing Strategy**:
   - How to test concurrent message publishing?
   - How to verify correct thread behavior?
   - How to simulate various error conditions?

4. **Performance Optimization**:
   - Should we use pre-allocated message pools?
   - How to minimize copying of large messages?
   - Should we implement message batching for efficiency? 