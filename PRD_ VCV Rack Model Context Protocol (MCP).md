# Product Requirements Document: VCV Rack Model Context Protocol (MCP) - V1 [cite: 1]

**Version:** 1.0 [cite: 1]
**Date:** April 26, 2025 [cite: 1]
**Based On:** "A Model Context Protocol for VCV Rack 2: Feasibility, Design, and Proposal" (VCV Rack MCP Development.pdf) [cite: 1]

## 1. Introduction & Goals [cite: 1]

### 1.1. Overview [cite: 2]

VCV Rack 2 currently relies on CV/Gate signals and adjacent Expander/Message systems for inter-module communication[cite: 2]. While effective for signal-level control and tightly coupled modules, these mechanisms are insufficient for exchanging complex, structured data between non-adjacent modules in a standardized way[cite: 3]. This document defines the requirements for a Model Context Protocol (MCP), a new standardized system within VCV Rack 2 designed to supplement existing communication methods[cite: 4].

### 1.2. Goals [cite: 5]

The primary goals of the MCP are: [cite: 5]

* **Enable Rich Data Exchange:** Allow modules to publish, discover, and subscribe to arbitrary, structured contextual information (e.g., musical metadata, large datasets, complex internal states)[cite: 5].
* **Standardize Communication:** Provide a common, robust protocol for non-adjacent module communication, reducing the need for non-standard, plugin-specific workarounds[cite: 6].
* **Improve Interoperability:** Facilitate the development of cooperating modules from different developers by providing a shared communication standard[cite: 7].
* **Unlock New Module Capabilities:** Enable new types of modules focused on patch-wide orchestration, meta-control, advanced data visualization, and deeper inter-module collaboration[cite: 8].
* **Maintain Performance:** Implement the protocol efficiently to minimize impact on VCV Rack's real-time audio processing[cite: 9].

### 1.3. Non-Goals [cite: 10]

* **Replace CV/Gate:** The MCP is not intended to replace the core CV/Gate signal path for real-time control voltage modulation[cite: 10].
* **Replace Expander/Message System:** The MCP is not intended to replace the efficient Expander/Message system for tightly coupled, adjacent modules[cite: 11, 12].

## 2. Problem Statement [cite: 12]

Current inter-module communication mechanisms in VCV Rack 2 have limitations for complex data sharing: [cite: 12]

* **CV/Gate/Triggers:** Limited to single floating-point values per polyphonic channel; lack structure for complex data; require explicit patching[cite: 13].
* **Serialization/Patch Storage:** Designed for state persistence (saving/loading), not real-time communication; unsuitable latency profile[cite: 14].
* **Expanders/Messages:** Strictly limited to physically adjacent modules; cannot facilitate general-purpose communication across a patch[cite: 15].
* **Lack of Discovery:** No standardized, reliable mechanism for one module to discover and address another arbitrary, non-adjacent module for data exchange[cite: 16].

These limitations hinder the development of modules requiring sophisticated interactions, such as sharing detailed sequence metadata, synchronizing intricate states between distant modules, or dynamically routing information based on patch context[cite: 17].

## 3. Proposed Solution: MCP Overview [cite: 17]

The MCP will be implemented as a system integrated into the VCV Rack environment, providing a standardized API for module developers[cite: 17]. It will function based on a Hybrid Broker/Interface Model: [cite: 18]

* **Central Broker:** A globally accessible, thread-safe singleton object responsible for: [cite: 18]
    * Registering/unregistering context "topics" offered by provider modules[cite: 19].
    * Allowing subscriber modules to discover available topics and the modules providing them[cite: 19].
    * Managing subscriptions[cite: 20].
    * Dispatching published messages/updates to relevant subscribers (Publish/Subscribe pattern)[cite: 20].
* **Defined Interfaces:** Standard C++ interfaces (e.g., IMCPProvider_V1, IMCPSubscriber_V1) implemented by participating modules[cite: 21].
* **Optional Direct Communication:** Once modules discover each other via the broker, they can potentially use the defined interfaces for more direct query/response interactions if needed, although Pub/Sub via the broker is the primary mechanism for updates[cite: 21].

## 4. Detailed Requirements [cite: 22]

### 4.1. Functional Requirements [cite: 23]

| ID    | Requirement Description                                                                                             | Source Section(s) | Priority |
| :---- | :------------------------------------------------------------------------------------------------------------------ | :---------------- | :------- |
| FR-01 | Topic Registration: Modules acting as context providers must be able to register specific topics they offer with the MCP broker. | 6.1, 6.2          | Must     |
| FR-02 | Topic Unregistration: Provider modules must be able to unregister topics when they are removed or no longer providing context. | 6.1, 6.2          | Must     |
| FR-03 | Subscription: Modules acting as context consumers must be able to subscribe to specific topics of interest via the MCP broker. | 6.1, 6.2          | Must     |
| FR-04 | Unsubscription: Subscriber modules must be able to unsubscribe from specific topics or all topics (e.g., upon removal). | 6.1, 6.2          | Must     |
| FR-05 | Publishing: Provider modules must be able to publish updates/messages associated with a specific topic via the MCP broker. | 6.1, 6.2          | Must     |
| FR-06 | Message Delivery: The MCP broker must dispatch published messages to all currently registered subscribers for that topic. | 6.1, 6.2          | Must     |  [cite: 24]
| FR-07 | Message Reception: Subscriber modules must have a mechanism (e.g., callback function onMCPMessage) to receive messages for topics they are subscribed to. | 6.1, 6.2          | Must     |
| FR-08 | Topic Discovery: Modules must be able to query the MCP broker for a list of currently available/registered topics. | 6.1, 6.2          | Must     |
| FR-09 | Provider Discovery: Modules must be able to query the MCP broker for a list of module IDs currently providing a specific topic. | 6.1, 6.2          | Must     |
| FR-10 | Arbitrary Data Support: The MCP message structure must support transmitting arbitrary data payloads.                   | 5.2, 6.2, 6.3     | Must     |
| FR-11 | Data Format Identification: The MCP message structure must include an identifier indicating the format of the data payload (e.g., "application/msgpack", "application/json"). | 6.2, 6.3          | Must     | [cite: 24, 25]
| FR-12 | Sender Identification: The MCP message structure must include the module ID of the sender.                             | 6.2               | Must     | [cite: 25]
| FR-13 | Lifecycle Management: The MCP system must integrate with the VCV Rack module lifecycle (e.g., onAdd, onRemove) for safe registration/unregistration. | 5.8, 6.4          | Must     | [cite: 25]
| FR-14 | API Access: A clear C++ API must be provided for developers to interact with the MCP broker and implement provider/subscriber roles. | 5.4, 6.2          | Must     | [cite: 25]

### 4.2. Non-Functional Requirements [cite: 26]

| ID     | Requirement Description                                                                                                                             | Source Section(s) | Priority |
| :----- | :-------------------------------------------------------------------------------------------------------------------------------------------------- | :---------------- | :------- |
| NFR-01 | Performance: MCP operations (especially those callable from the audio thread) must have minimal performance overhead and avoid blocking the audio thread. | 5.5, 7.1          | Must     | [cite: 27, 28]
| NFR-02 | Real-time Safety: Functions intended for use on the audio thread must be real-time safe (non-blocking, predictable execution time, no dynamic memory allocation if possible, no contended locks). | 5.5, 5.6          | Must     | [cite: 28]
| NFR-03 | Thread Safety: All interactions with the shared MCP broker and any shared MCP state must be thread-safe, handling VCV Rack's multi-threaded engine correctly. | 5.6, 6.1, 7.1     | Must     | [cite: 28]
| NFR-04 | Robustness: The system should handle errors gracefully (e.g., deserialization failures, invalid module IDs).                                           | 5.6, 6.4          | Must     | [cite: 28]
| NFR-05 | Stability: The MCP implementation must not introduce crashes, hangs, or data corruption.                                                               | 5.6               | Must     | [cite: 28]
| NFR-06 | Data Efficiency: The primary data serialization format should be efficient in terms of speed (read/write) and size.                                    | 5.2, 6.3, 7.1     | Must     | [cite: 28]
| NFR-07 | Usability (API): The developer-facing API should be clear, concise, well-documented, and relatively easy to use.                                     | 5.4, 7.1          | High     | [cite: 28, 29]
| NFR-08 | Scalability: The architecture should handle a reasonable number of topics, providers, and subscribers without significant performance degradation.         | 5.1               | High     | [cite: 29]
| NFR-09 | Versioning: The protocol and API must include versioning mechanisms to allow for future evolution while maintaining compatibility where possible.         | 5.7               | Must     | [cite: 29]
| NFR-10 | Compatibility: The core MCP API, once standardized, should aim for stability across VCV Rack versions.                                               | 5.7               | Must     | [cite: 29]
| NFR-11 | SDK Integration: The MCP should be integrated into the VCV Rack SDK for universal availability to developers.                                         | 5.8, 7.2          | Must     | [cite: 29]

## 5. Design & Technical Specifications Summary [cite: 30]

* **Architecture:** Hybrid Broker/Interface Model[cite: 30].
    * Broker: Thread-safe Singleton (IMCPBroker)[cite: 30]. Manages topic registry, subscriptions, and Pub/Sub message dispatch[cite: 31].
    * Interfaces: IMCPProvider_V1, IMCPSubscriber_V1 implemented by modules[cite: 31].
* **Communication:** Primarily Pub/Sub via Broker. Optional direct interface calls post-discovery[cite: 32].
* **Data Representation:** [cite: 33]
    * Message Structure: MCPMessage_V1 containing topic, sender ID, data format, data payload (std::shared_ptr<void>), data size[cite: 33].
    * Serialization: MessagePack recommended as default (application/msgpack)[cite: 33]. JSON (application/json) optional secondary format[cite: 34].
* **Discovery:** [cite: 34]
    * Broker maintains topic -> provider ID mapping[cite: 34].
    * API functions: getAvailable Topics(), findProviders(topic)[cite: 34].
* **Threading Model:** [cite: 35]
    * Broker operations must be thread-safe (using std::mutex, etc.)[cite: 35].
    * Message dispatch (onMCPMessage calls) must occur off the audio thread (e.g., via worker threads)[cite: 36].
    * Communication between MCP worker threads and module audio threads should use thread-safe queues (e.g., rack::dsp::RingBuffer)[cite: 37].
* **API:** C++ based, as outlined in Section 6.2 of the source document. Global accessor function (getMCPBroker()) needed[cite: 38].

## 6. Future Considerations & Enhancements [cite: 39]

* Advanced querying capabilities (beyond simple topic lookup)[cite: 39].
* Definition of standardized topic names and data formats for common use cases (musical context, transport state, etc.)[cite: 40].
* UI integration for visualizing MCP connections/activity[cite: 41].
* Dedicated bridging modules (MCP <-> CV/Gate, MCP <-> OSC, MCP <-> MIDI)[cite: 41].
* Optional support for schema validation (FlatBuffers, JSON Schema)[cite: 42].

## 7. Open Questions [cite: 42]

(None explicitly identified in the source document, but implementation details will raise questions during development, e.g., specific locking strategies, worker thread pool management, detailed error handling protocols.) [cite: 42]