# VCV Rack Model Context Protocol (MCP)

## Overview

The VCV Rack Model Context Protocol (MCP) is a standardized system for VCV Rack 2 designed to supplement existing communication methods. It enables modules to publish, discover, and subscribe to arbitrary, structured contextual information.

This repository contains the implementation of Phase 1 of the MCP development plan, covering Sprint 1 (Broker & Registration) and Sprint 2 (Subscription & Lifecycle).

## Current Features

### Phase 1, Sprint 1: Broker Singleton & Basic Registration
- Thread-safe Broker singleton implementation
- Topic registration/unregistration functions
- Provider/topic discovery functions
- Basic interfaces for providers
- Unit tests for core functionality
- Example demonstration program

### Phase 1, Sprint 2: Subscription & Basic Lifecycle
- IMCPSubscriber_V1 interface with onMCPMessage placeholder
- Subscribe/unsubscribe functionality in the Broker
- Lifecycle management with unsubscribeAll for module cleanup
- Thread-safe subscription management using weak references
- Extended unit tests for subscription functionality
- Initial API documentation

## Building the Project

### Requirements

- C++14 compatible compiler
- CMake 3.15+
- Git

### Build Steps

```bash
# Clone the repository
git clone https://github.com/vcvrack/mcp.git
cd mcp

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Run tests
ctest

# Run example
./bin/broker_registration
```

## Project Structure

- `include/mcp/`: Interface and class declarations
  - `IMCPBroker.h`: The broker interface 
  - `IMCPProvider_V1.h`: The provider interface
  - `IMCPSubscriber_V1.h`: The subscriber interface (added in Sprint 2)
  - `MCPBroker.h`: The broker implementation

- `src/mcp/`: Implementation files
  - `IMCPBroker.cpp`: Interface implementation
  - `MCPBroker.cpp`: Broker implementation

- `test/mcp/`: Unit tests
  - `BrokerTests.cpp`: Tests for broker functionality

- `examples/`: Example programs
  - `broker-registration/`: Example demonstrating registration

- `docs/`: Documentation
  - `api_documentation.md`: API documentation

## Next Steps

This implementation represents Sprint 2 of the MCP development plan. Future sprints will include:

- Sprint 3: Message structure definition and serialization
- Sprint 4: Message dispatch implementation and publish/receive functionality
- Sprint 5: Reference implementation modules

## License

[Add appropriate license information] 