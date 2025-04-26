# VCV Rack MCP - Active Context

## Current Work Focus

The project has completed Phase 1, Sprint 2 (Subscription & Basic Lifecycle) and is preparing to begin Phase 2, Sprint 3 (Message Structure & Serialization).

### Sprint 2 Completed (Subscription & Basic Lifecycle)

We have successfully implemented:

- The `IMCPSubscriber_V1` interface with `onMCPMessage` placeholder
- Subscription management in the broker:
  - `subscribe` function for topic subscription
  - `unsubscribe` function for topic unsubscription
  - `unsubscribeAll` function for module cleanup
- Thread-safe subscription handling with mutex protection
- Weak reference management for subscribers
- Enhanced unit tests for subscription functionality
- Initial API documentation

### Sprint 3 Preparation (Message Structure & Serialization)

We are now preparing to implement message structure and serialization functionality, including:

- Defining the `MCPMessage_V1` structure
- Integrating MessagePack for serialization
- Creating helper functions for serialization/deserialization
- Implementing message structure unit tests
- Preparing for publish/dispatch implementation

## Recent Changes

The subscription functionality of the MCP has been completed with the following key components:

1. **New Interface**:
   - `IMCPSubscriber_V1`: Interface for modules that want to receive updates with `onMCPMessage` callback

2. **Enhanced Broker Interface**:
   - Added `subscribe`, `unsubscribe`, and `unsubscribeAll` functions
   - Forward-declared `MCPMessage_V1` structure for upcoming Sprint 3

3. **Broker Implementation**:
   - Added subscription management with thread-safe operations
   - Implemented proper weak reference handling for subscribers
   - Added appropriate mutex protection for subscription operations

4. **Testing**:
   - Added unit tests for subscription/unsubscription
   - Added tests for weak reference handling
   - Added concurrency tests for subscription operations

5. **Documentation**:
   - Created API documentation covering both Sprint 1 and Sprint 2 functionality
   - Updated README.md with Sprint 2 features

## Active Decisions and Considerations

### Current Decisions

1. **Subscriber Interface Design**: Implemented a simple `IMCPSubscriber_V1` interface with a single `onMCPMessage` callback to keep the API clean and flexible.

2. **Subscription Storage**: Implemented a topic-centric organization in the broker:
   - Using `std::unordered_map<std::string, std::vector<std::weak_ptr<IMCPSubscriber_V1>>>` for efficient topic lookup
   - This allows for efficient message dispatch to all subscribers of a topic

3. **Thread Safety**: Separated mutex protection for registry and subscriptions:
   - `m_registryMutex` for provider registration
   - `m_subscriptionMutex` for subscriber management
   - This reduces contention between provider and subscriber operations

4. **Lifecycle Integration**: Implemented lifecycle management through:
   - `unsubscribeAll` for easy cleanup when a module is removed
   - Weak reference handling to automatically handle destroyed modules

### Open Questions for Sprint 3

1. **Message Structure Design**: How should the `MCPMessage_V1` structure be designed?
   - What fields are required for effective message routing?
   - How should binary data be represented and managed?
   - How to ensure backward compatibility for future versions?

2. **Serialization Approach**: How to integrate MessagePack most effectively?
   - Direct integration or wrapper helpers?
   - Support for common VCV Rack data types?
   - Error handling for serialization failures?

3. **Memory Management**: How to efficiently manage serialized data?
   - Who owns the memory for message payloads?
   - How to avoid excessive copying of data?
   - When to free message memory?

## Next Steps

### Immediate Tasks (Sprint 3)

1. Define the `MCPMessage_V1` structure
2. Integrate MessagePack library
3. Implement serialization/deserialization helpers
4. Create unit tests for message structure and serialization
5. Update documentation with message structure details

### Upcoming Milestones

**Sprint 3: Message Structure & Serialization** (Current Focus)
- Define message structure
- Integrate serialization library
- Create helper functions
- Test serialization/deserialization
- Document message format

**Sprint 4: Basic Publish & Receive** (Next)
- Implement publish functionality
- Create worker thread for message dispatch
- Implement subscriber notification
- Add thread-safe communication patterns
- Test end-to-end message flow 