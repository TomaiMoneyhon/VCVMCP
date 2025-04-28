# MCP Topic System Documentation

## Overview

The MCP (Module Communication Protocol) Topic System enables standardized communication between VCV Rack modules. This collection of documents provides comprehensive guidance on how to effectively use topics within the MCP ecosystem.

## Core Documents

| Document | Description |
|----------|-------------|
| [Topic Naming Conventions](topic_naming_conventions.md) | Guidelines for creating well-formed topic names |
| [Standard Topics](standard_topics.md) | Catalog of officially supported topics |
| [Message Formats](message_formats.md) | Detailed specifications for standard message formats |
| [Topic Registration](topic_registration.md) | Process for registering new topics |
| [Topic Best Practices](topic_best_practices.md) | Recommended patterns for efficient topic usage |

## Getting Started

If you're new to the MCP Topic System, we recommend reading the documents in this order:

1. Start with [Topic Naming Conventions](topic_naming_conventions.md) to understand how topics are structured
2. Review [Standard Topics](standard_topics.md) to see if existing topics meet your needs
3. Study [Message Formats](message_formats.md) to understand how to format your messages
4. Read [Topic Best Practices](topic_best_practices.md) for implementation guidance
5. If creating new topics, follow the [Topic Registration](topic_registration.md) process

## Quick Reference

### Topic Structure

Topics follow a URI-style format:
```
namespace/resource
```

Example: `com.vcvrack.core/clock`

### Common Namespaces

- `mcp.system/*`: Core MCP system functionality
- `com.vcvrack.core/*`: Standard VCV Rack functionality
- `com.vendorname.productname/*`: Vendor-specific topics

### Message Format

Most messages use JSON with this structure:
```json
{
  "type": "message-type",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.module-name",
  "data": {
    // Message type-specific payload
  }
}
```

## Implementation Examples

The [Topic Best Practices](topic_best_practices.md) document includes code examples for:

- Implementing a basic topic provider
- Implementing a basic topic subscriber
- Message throttling and filtering techniques
- Batching messages for efficiency

## Contributing

The MCP Topic System is designed to evolve with the needs of the VCV Rack community. To contribute:

1. Follow the [Topic Registration](topic_registration.md) process for new standard topics
2. Submit improvements to documentation via pull requests
3. Share your implementation experiences on the [VCV Rack forum](https://community.vcvrack.com/)

## Versioning

The MCP Topic System documentation is versioned alongside the MCP API. This documentation corresponds to MCP API version 1.0.

## Support

For questions about the MCP Topic System, contact:
- The VCV Rack forum: [https://community.vcvrack.com/](https://community.vcvrack.com/)
- The MCP GitHub repository: [https://github.com/vcvrack/mcp](https://github.com/vcvrack/mcp) 