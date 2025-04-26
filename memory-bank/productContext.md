# VCV Rack MCP - Product Context

## Why This Project Exists

VCV Rack is an open-source virtual modular synthesizer that simulates Eurorack hardware modules. While it provides excellent real-time audio processing through CV/Gate signals and offers an Expander system for adjacent modules, there's a significant gap in the platform's ability to share complex, structured data between non-adjacent modules in a standardized way.

This project aims to bridge that gap by providing a standardized Model Context Protocol (MCP) that enables modules to communicate rich, structured information across a patch regardless of their physical proximity.

## Problems It Solves

### 1. Limited Data Structures

Current CV/Gate signals in VCV Rack are limited to simple floating-point values. While effective for audio and control signals, they cannot efficiently represent complex data structures like:
- Musical metadata (key, scale, chord progressions)
- Sequence information
- Parameter mappings
- Visual data for interfaces
- State synchronization

### 2. Physical Adjacency Requirements

The current Expander/Message system requires modules to be physically adjacent. This creates significant constraints for:
- Modules that need to communicate with multiple other modules
- Complex patches where related modules might not be placed next to each other
- Module developers who want to create systems that interact with modules from other developers

### 3. Discoverability

There's no standardized way for modules to discover and address other modules in a patch. This forces developers to create custom, non-standard solutions that:
- Don't work across different module libraries
- Require custom implementations for each interaction
- Create fragmentation in the ecosystem

### 4. Interoperability

Without a common protocol, modules from different developers have limited ability to work together in sophisticated ways, hindering ecosystem growth and module capabilities.

## How It Should Work

The MCP is designed as a supplementary system that works alongside existing CV/Gate and Expander mechanisms. It follows a publish-subscribe architecture:

1. **Provider modules** register topics they offer with a central broker
2. **Consumer modules** discover available topics and subscribe to ones they're interested in
3. **When data changes**, provider modules publish updates to subscribed modules through the broker
4. **Subscriber modules** receive notifications and can process the structured data

This system enables new workflows such as:
- Musical context sharing (key, scale, chord information across sequencers)
- Complex state synchronization between distant modules
- Meta-control modules that can orchestrate multiple other modules
- Advanced visualizations of data from throughout a patch
- More sophisticated interaction between modules

## User Experience Goals

For **module users**:
- Seamless interoperability between compatible modules
- More musically aware modules that can respond to context
- Richer visual feedback and interaction
- No performance degradation in existing audio processing

For **module developers**:
- Clear, well-documented API for implementing MCP
- Minimal implementation complexity to adopt basic functionality
- Standardized patterns for common use cases
- Efficient performance with real-time audio considerations 