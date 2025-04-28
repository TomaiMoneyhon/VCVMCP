# Known Issues and Limitations

This document lists the known issues, limitations, and planned improvements for the MCP system.

## Current Issues

### 1. ✅ RingBuffer Thread Safety (FIXED)

**Description:** Previous implementations of `RingBuffer` had a thread safety issue in high-concurrency scenarios where some items could be consumed multiple times while others weren't consumed at all.

**Fix Implemented:** In Sprint 7, a comprehensive redesign of the RingBuffer was completed:
- Optimized specifically for Single-Producer/Single-Consumer (SPSC) usage pattern
- Implemented sequential consistency for all atomic operations
- Added explicit memory barriers at critical points 
- Simplified push/pop logic for improved reliability
- Achieved ~980,000 messages/second throughput with perfect reliability

**Current Status:** 
- The issue has been resolved in the current implementation
- Comprehensive test suite verifies thread safety with high concurrent loads
- Clear documentation added for proper usage patterns

**Usage Guidelines:**
- Use one producer thread and one consumer thread per RingBuffer instance
- Size buffers appropriately (64-128 elements recommended for most modules)
- Always check the return value of push() to handle buffer full conditions
- See the [Thread Safety Guide](guides/thread_safety.md) for detailed best practices

### 2. Error Reporting for Serialization Failures

**Description:** The current error reporting mechanism for serialization failures is limited, providing only basic error messages without detailed context.

**Impact:** Debugging serialization issues can be more difficult than necessary, especially for complex data structures.

**Workaround:** Implement more detailed error handling and logging in your module code.

**Status:** Scheduled for improvement in future updates.

### 3. Message Dispatch Timeout Options

**Description:** The broker's message dispatch does not currently support timeout options, which could lead to blocking in certain edge cases.

**Impact:** In extremely high-load scenarios or when subscribers are slow to process messages, this could potentially impact performance.

**Workaround:** Implement efficient message handling in subscribers and avoid publishing extremely large messages.

**Status:** Timeout options are planned for future implementation.

### 4. Build System Warnings

**Description:** The codebase currently generates several warnings during compilation:
- Unqualified std::move calls in the msgpack11 external library
- Extra qualification warnings in MCPSerialization.cpp

**Impact:** These warnings don't affect functionality but may clutter build output.

**Workaround:** None needed; these are style warnings that don't impact functionality.

**Status:** Low priority, scheduled for cleanup in future updates.

### 5. CMake Compatibility Warnings

**Description:** The GoogleTest dependency uses an older CMake configuration style that generates deprecation warnings.

**Impact:** These warnings indicate potential future compatibility issues with newer CMake versions but don't affect current builds.

**Workaround:** None needed currently; these are warnings from external dependencies.

**Status:** Medium priority, to be addressed during SDK integration.

## Limitations

### 1. JSON Format Support

**Description:** While JSON format (`application/json`) is defined, the current implementation has limited JSON support compared to MessagePack.

**Impact:** Users requiring JSON serialization may find the current implementation less comprehensive.

**Workaround:** Use MessagePack format (`application/msgpack`) for all serialization needs.

**Status:** Full JSON support is planned for future updates.

### 2. Direct Module-to-Module Communication

**Description:** The current architecture focuses on broker-mediated communication, with limited support for direct module-to-module communication.

**Impact:** Modules that would benefit from direct communication patterns may need to use the broker as an intermediary.

**Workaround:** Use the broker's discovery mechanisms to find provider modules, then use the broker for communication.

**Status:** Enhanced direct communication patterns are being considered for future versions.

### 3. Dynamic Topic Management

**Description:** Topics are currently static - they are registered at module initialization and unregistered at module removal.

**Impact:** Modules cannot easily change their provided topics dynamically at runtime.

**Workaround:** Design your topic structure to be static, or handle dynamic aspects at the data level rather than the topic level.

**Status:** Dynamic topic management is being considered for future versions.

## Future Enhancements

The following enhancements are planned for future releases:

1. **Enhanced Thread Safety**: Additional thread safety improvements for other components beyond RingBuffer.
2. **Improved Serialization Error Reporting**: More detailed error messages and context for serialization failures.
3. **Timeout Options**: Add timeout options for message dispatch and other potentially blocking operations.
4. **Enhanced JSON Support**: Full support for JSON serialization with feature parity to MessagePack.
5. **Direct Communication Patterns**: Additional mechanisms for direct module-to-module communication.
6. **Dynamic Topic Management**: Support for runtime changes to topic registration.
7. **Performance Optimizations**: Continued performance improvements for high-load scenarios.
8. **Standardized Topic Definitions**: Common topic naming conventions and data formats for standard use cases.

## Reporting New Issues

If you encounter an issue not listed here, please report it through the appropriate channels with the following information:

1. Detailed description of the issue
2. Steps to reproduce
3. Expected vs. actual behavior
4. Code samples demonstrating the issue (if applicable)
5. Environment details (VCV Rack version, OS, etc.) 