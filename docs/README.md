# VCV Rack Model Context Protocol (MCP) Documentation

Welcome to the documentation for the VCV Rack Model Context Protocol (MCP). This documentation will help you understand and use the MCP system in your VCV Rack modules.

## Documentation Sections

### Getting Started

- [Introduction](concepts/introduction.md) - Overview of the MCP system and its concepts
- [Quickstart Guide](guides/quickstart.md) - Get up and running with MCP quickly

### Core Concepts

- [Architecture Overview](concepts/architecture.md) - Detailed explanation of the MCP architecture
- [Topic Management](concepts/topic_naming.md) - Topic naming conventions and best practices
- [Threading Model](concepts/threading_model.md) - Understanding the threading model used by MCP

### Implementation Guides

- [Provider Implementation](guides/provider_implementation.md) - How to implement a provider module
- [Subscriber Implementation](guides/subscriber_implementation.md) - How to implement a subscriber module
- [Thread Safety Guide](guides/thread_safety.md) - Guide to thread-safe programming with MCP
- [Serialization Guide](guides/serialization.md) - Working with serialized data in MCP

### API Reference

- [IMCPBroker](api/IMCPBroker.md) - The broker interface
- [IMCPProvider_V1](api/IMCPProvider_V1.md) - The provider interface
- [IMCPSubscriber_V1](api/IMCPSubscriber_V1.md) - The subscriber interface
- [MCPMessage_V1](api/MCPMessage_V1.md) - The message structure
- [Serialization Helpers](api/serialization.md) - Helper functions for serialization

### Best Practices

- [Performance Optimization](best-practices/performance.md) - Tips for optimizing MCP usage
- [Error Handling](best-practices/error_handling.md) - Best practices for error handling
- [Testing](best-practices/testing.md) - How to test MCP implementations

### Example Implementations

- [Basic Examples](examples/README.md) - Simple examples of MCP usage
- [Advanced Examples](examples/advanced.md) - More complex MCP implementations

## Version Information

This documentation covers MCP Version 1 (V1) implemented in Phases 1, 2, and 3 of the MCP development plan.

## Additional Resources

- [Known Issues](known_issues.md) - Current known issues and limitations
- [Frequently Asked Questions](faq.md) - Common questions and answers
- [Contributing Guide](contributing.md) - How to contribute to the MCP project

## About MCP

The Model Context Protocol (MCP) is a standardized system for VCV Rack 2 that enables modules to exchange complex, structured data. It supplements existing communication mechanisms (CV/Gate signals and Expanders) by providing a flexible way for modules to share information across a patch, regardless of their physical placement.

MCP was developed to address limitations in current VCV Rack communication methods and to enable new types of module interactions and functionality. 