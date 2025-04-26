# VCV Rack Model Context Protocol (MCP) - Project Brief

## Overview
The VCV Rack Model Context Protocol (MCP) is a new standardized system for VCV Rack 2 designed to supplement existing communication methods. It enables modules to publish, discover, and subscribe to arbitrary, structured contextual information.

## Core Requirements

### Problem Statement
Current inter-module communication mechanisms in VCV Rack 2 have limitations for complex data sharing:
- CV/Gate/Triggers: Limited to single floating-point values per polyphonic channel
- Serialization/Patch Storage: Not designed for real-time communication
- Expanders/Messages: Limited to physically adjacent modules
- Lack of Discovery: No standardized mechanism for modules to discover and address non-adjacent modules

### Goals
- Enable rich data exchange between modules (musical metadata, large datasets, complex states)
- Standardize communication for non-adjacent modules
- Improve interoperability between modules from different developers
- Unlock new module capabilities (patch-wide orchestration, meta-control, visualization)
- Maintain performance with minimal impact on real-time audio processing

### Non-Goals
- Replace CV/Gate signal paths for real-time control voltage modulation
- Replace Expander/Message system for tightly coupled, adjacent modules

## Implementation Approach
The MCP will implement a Hybrid Broker/Interface Model:
- Central Broker: A globally accessible, thread-safe singleton for managing topics, subscriptions, and message dispatch
- Defined Interfaces: Standard C++ interfaces for modules to implement 
- Communication: Primarily Publish/Subscribe pattern via the Broker

## Project Scope
- Implementation of core MCP architecture
- Integration into VCV Rack SDK
- Reference module implementations
- Comprehensive developer documentation
- Performance and robustness testing

## Success Criteria
- Modules can register, discover, and subscribe to topics via the MCP broker
- Modules can publish and receive complex, structured data
- The system performs efficiently with minimal impact on audio performance
- The API is well-documented and easy for developers to adopt
- MCP is successfully integrated into the VCV Rack SDK 