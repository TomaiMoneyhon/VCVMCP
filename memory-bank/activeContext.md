# VCV Rack MCP - Active Context

## Current Work Focus

The project has completed Phase 1, Sprint 1 (Core Broker & Basic Registration) and is ready to begin Sprint 2 (Subscription & Basic Lifecycle).

### Sprint 1 Completed (Core Broker & Basic Registration)

We have successfully implemented:

- Core project structure with CMake build system
- The `IMCPBroker` interface with singleton pattern
- The `IMCPProvider_V1` interface
- Thread-safe broker implementation with mutex protection
- Topic registration/unregistration functionality
- Provider/topic discovery functions
- Unit tests for all core functionality
- Example demonstration program

### Sprint 2 Preparation (Subscription & Basic Lifecycle)

We are now preparing to implement subscriber functionality, including:

- Defining the `IMCPSubscriber_V1` interface with `onMCPMessage` callback
- Implementing `subscribe` and `unsubscribe` API functions
- Adding `unsubscribeAll` for module cleanup
- Enhancing lifecycle integration
- Expanding unit test coverage

## Recent Changes

The initial implementation of the MCP Broker has been completed with the following key components:

1. **Core Interfaces**:
   - `IMCPProvider_V1`: Interface for modules that provide contextual information
   - `IMCPBroker`: Interface for the central broker with registration and discovery API

2. **Broker Implementation**:
   - `MCPBroker`: Thread-safe singleton class implementing the broker interface
   - Uses `std::make_shared` for proper singleton instance management
   - Uses weak references to providers to handle module destruction

3. **Project Structure**:
   - Organized as a CMake project with proper directory structure
   - Unit tests with Google Test framework
   - Example program demonstrating core functionality

4. **Implementation Challenges Solved**:
   - Resolved singleton implementation issues with proper destructor visibility
   - Implemented thread-safe registry with mutex protection
   - Created proper weak reference handling for automatic cleanup

## Active Decisions and Considerations

### Current Decisions

1. **Singleton Pattern**: We've implemented the broker as a singleton with `getInstance()` using `std::make_shared` for proper memory management.

2. **Thread Safety Approach**: The implementation uses `std::mutex` for thread-safety in registry operations, with separate mutexes for instance creation and registry access.

3. **Registry Data Structure**: Using `std::unordered_map` with `std::weak_ptr` references to provide efficient lookups and automatic cleanup of expired references.

4. **Versioning Strategy**: Interface versioning with `_V1` suffix has been implemented.

### Open Questions for Sprint 2

1. **Subscriber Interface Design**: How should the `IMCPSubscriber_V1` interface be designed to balance simplicity and flexibility?
   - Should it include topic filtering capabilities?
   - How will it handle different message formats?

2. **Subscription Storage**: What is the most efficient data structure for storing subscriber registrations?
   - Topic-centric organization: topic -> list of subscribers
   - Subscriber-centric organization: subscriber -> list of topics
   - Or a hybrid approach?

3. **Lifecycle Integration**: How to best integrate with VCV Rack's module lifecycle?
   - Automatic cleanup on module removal
   - Reference counting strategy for subscribers

## Next Steps

### Immediate Tasks (Sprint 2)

1. Define the `IMCPSubscriber_V1` interface
2. Implement subscriber registration/unregistration in the broker
3. Add module lifecycle integration for subscribers
4. Update unit tests to cover subscription functionality
5. Create example program demonstrating subscription

### Upcoming Milestones

**Sprint 2: Subscription & Basic Lifecycle** (Current Focus)
- Implement subscriber registration
- Create subscription management
- Enhance lifecycle integration
- Improve reference handling
- Expand test coverage

**Sprint 3: Message Structure & Serialization** (Next)
- Define `MCPMessage_V1` structure
- Integrate MessagePack serialization
- Implement serialization/deserialization helpers
- Add unit tests for message handling 