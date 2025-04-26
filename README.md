# VCV Rack Model Context Protocol (MCP)

## Overview

The VCV Rack Model Context Protocol (MCP) is a standardized system for VCV Rack 2 designed to supplement existing communication methods. It enables modules to publish, discover, and subscribe to arbitrary, structured contextual information.

This repository contains the implementation of Phase 1, Sprint 1 of the MCP development plan, focusing on the core broker and basic registration functionality.

## Current Features (Phase 1, Sprint 1)

- Thread-safe Broker singleton implementation
- Topic registration/unregistration functions
- Provider/topic discovery functions
- Basic interfaces for providers
- Unit tests for core functionality
- Example demonstration program

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
  - `MCPBroker.h`: The broker implementation

- `src/mcp/`: Implementation files
  - `IMCPBroker.cpp`: Interface implementation
  - `MCPBroker.cpp`: Broker implementation

- `test/mcp/`: Unit tests
  - `BrokerTests.cpp`: Tests for broker functionality

- `examples/`: Example programs
  - `broker-registration/`: Example demonstrating registration

## Next Steps

This implementation represents Sprint 1 of the MCP development plan. Future sprints will include:

- Subscriber interface implementation
- Subscription management
- Message structure definition
- Message serialization and dispatch
- Reference module implementation

## License

[Add appropriate license information] 