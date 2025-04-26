# VCV Rack Model Context Protocol (MCP) V1 - Agile Development Plan

**Version:** 1.0
**Date:** April 26, 2025
**Methodology:** Agile (Scrum-based, 2-week Sprints)
**Core Constraint:** Each sprint delivers a runnable, integrated, tested, and potentially demonstrable "Shippable Increment".

## 1. Overall Vision & Goals

The Model Context Protocol (MCP) aims to supplement existing VCV Rack communication mechanisms (CV/Gate, Expanders) by providing a standardized, robust system for modules to exchange complex, structured data.

**Primary Goals (Ref: PRD 1.2):**

* **Enable Rich Data Exchange:** Allow modules to publish, discover, and subscribe to arbitrary, structured contextual information (e.g., musical metadata, large datasets, complex internal states).
* **Standardize Communication:** Provide a common protocol for non-adjacent module communication, reducing plugin-specific workarounds.
* **Improve Interoperability:** Facilitate cooperation between modules from different developers via a shared standard.
* **Unlock New Module Capabilities:** Enable modules focused on patch-wide orchestration, meta-control, advanced visualization, and deeper collaboration.
* **Maintain Performance:** Implement efficiently to minimize impact on real-time audio processing (NFR-01).

## 2. Architecture Summary

The MCP will be implemented using the **Hybrid Broker/Interface Model** (Ref: PRD 3, Design Doc 6.1):

* **Central Broker:** A globally accessible, thread-safe singleton (`IMCPBroker`) managing topic registration/unregistration, provider/subscriber discovery, and publish/subscribe message dispatch.
* **Defined Interfaces:** Standard C++ interfaces (`IMCPProvider_V1`, `IMCPSubscriber_V1`) implemented by participating modules.
* **Communication:** Primarily Publish/Subscribe via the Broker for updates. Optional direct interface calls between modules are possible post-discovery for specific query/response needs.

## 3. High-Level Roadmap & Phases

Based on the suggested implementation roadmap (Ref: Design Doc 7.2), the development is broken into the following major phases:

| Phase                                       | Key Focus                                                                                                | Associated Requirements (Examples)                                                                 |
| :------------------------------------------ | :------------------------------------------------------------------------------------------------------- | :------------------------------------------------------------------------------------------------- |
| **Phase 1: Core Broker & API Foundation** | Implement the basic Broker singleton, core API functions, basic thread-safety, lifecycle integration.      | FR-01, FR-02, FR-03, FR-04, FR-08, FR-09, FR-13, FR-14, NFR-03, NFR-05, NFR-09                      |
| **Phase 2: Serialization & Pub/Sub Flow** | Integrate MessagePack, define `MCPMessage_V1`, implement publish/receive logic, basic message delivery.    | FR-05, FR-06, FR-07, FR-10, FR-11, FR-12, NFR-06                                                    |
| **Phase 3: Reference Implementation & Docs** | Develop basic provider/subscriber example modules, refine API usability, create initial developer docs.    | FR-14 (API Usage), NFR-07 (API Usability/Docs)                                                     |
| **Phase 4: Testing & Refinement** | Rigorous performance, thread-safety, and robustness testing. Error handling, API stabilization, doc polish. | NFR-01, NFR-02, NFR-03, NFR-04, NFR-05, NFR-08 (Scalability), NFR-09 (Versioning), NFR-10 (Stability) |
| **Phase 5: SDK Integration & Release Prep** | Integrate MCP into the VCV Rack SDK, finalize documentation, prepare for release.                          | NFR-11                                                                                             |

## 4. Sprint Breakdown (2-Week Sprints)

This is a proposed initial breakdown. Tasks may shift based on discoveries during development.

---

### Phase 1: Core Broker & API Foundation

**Sprint 1: Broker Singleton & Basic Registration**

* **Sprint Goal:** Establish the core, thread-safe Broker singleton and implement basic topic registration/unregistration functionality.
* **Key Features/Tasks:**
    * Implement `IMCPBroker` singleton structure with basic thread-safety (using `std::mutex` for registry access) (NFR-03).
    * Implement `registerContext` (FR-01) and `unregisterContext` (FR-02) API functions.
    * Implement internal topic/provider registry data structure.
    * Implement `getAvailableTopics` (FR-08) and `findProviders` (FR-09) discovery functions (read-only access).
    * Define initial `IMCPProvider_V1` interface (FR-14).
    * Integrate with module lifecycle (`onAdd`/`onRemove`) for basic registration/unregistration calls (FR-13).
    * Set up basic unit tests for registration, unregistration, and discovery logic.
    * Define initial Protocol Versioning mechanism (NFR-09).
* **Shippable Increment Definition:** A runnable Broker singleton that allows modules to register/unregister topics and query the registry via the defined API functions. Core logic is unit tested for thread-safe registry access.

**Sprint 2: Subscription & Basic Lifecycle**

* **Sprint Goal:** Implement subscriber registration/unregistration and ensure robust lifecycle management.
* **Key Features/Tasks:**
    * Define `IMCPSubscriber_V1` interface including `onMCPMessage` placeholder (FR-14, FR-07).
    * Implement `subscribe` (FR-03) and `unsubscribe` (FR-04) API functions in the Broker, including registry updates.
    * Implement `unsubscribeAll` for module cleanup (FR-04).
    * Refine module lifecycle integration (`onAdd`/`onRemove`) for subscription management (FR-13).
    * Use `std::weak_ptr` for provider/subscriber references in the Broker registry to handle module destruction safely.
    * Add unit tests for subscription/unsubscription logic and lifecycle cleanup.
    * Begin initial API documentation (NFR-07).
* **Shippable Increment Definition:** Broker singleton now manages subscriber registration alongside provider registration. Lifecycle management ensures proper cleanup on module removal. Subscription logic is unit tested. API includes basic subscription functions.

---

### Phase 2: Serialization & Pub/Sub Flow

**Sprint 3: Message Structure & Serialization**

* **Sprint Goal:** Define the core message structure and integrate MessagePack serialization for data payloads.
* **Key Features/Tasks:**
    * Define `MCPMessage_V1` struct including `topic`, `senderModuleId`, `dataFormat`, `data` (`std::shared_ptr<void>`), `dataSize` (FR-10, FR-11, FR-12).
    * Integrate MessagePack library.
    * Implement helper functions/examples for serializing C++ data structures (e.g., `std::string`, simple structs) into a `std::shared_ptr<void>` byte buffer for `MCPMessage_V1::data` using MessagePack (NFR-06).
    * Implement helper functions/examples for deserializing MessagePack data from `MCPMessage_V1::data` based on `dataFormat` and `dataSize` (NFR-06).
    * Add unit tests for serialization/deserialization helpers.
    * Document `MCPMessage_V1` and serialization usage (NFR-07).
* **Shippable Increment Definition:** A defined `MCPMessage_V1` structure and tested helper functions/examples demonstrating serialization and deserialization of basic data types using MessagePack into/from the message structure.

**Sprint 4: Basic Publish & Receive**

* **Sprint Goal:** Implement the basic publish/subscribe message flow using the Broker and serialized messages.
* **Key Features/Tasks:**
    * Implement the `publish` function in the Broker (FR-05). Internally, this should queue the `MCPMessage_V1`.
    * Implement a basic worker thread mechanism in the Broker to process the publish queue.
    * Worker thread retrieves messages, identifies relevant subscribers for the topic, and calls their `onMCPMessage` callback (FR-06, FR-07). **Crucially, `onMCPMessage` must be called off the audio thread.** (NFR-01, NFR-02).
    * Implement basic error handling for deserialization failures within the subscriber's `onMCPMessage` (NFR-04).
    * Add integration tests for the basic publish -> broker dispatch -> subscriber `onMCPMessage` flow with simple serialized data.
    * Conduct initial performance assessment of the publish/dispatch mechanism (NFR-01).
* **Shippable Increment Definition:** A runnable publish/subscribe system where a provider can publish a MessagePack-serialized message via the Broker, and a subscribed module receives the message via its `onMCPMessage` callback (executed on a non-audio thread). Basic flow is integration tested.

---

### Phase 3: Reference Implementation & Docs

**Sprint 5: Reference Provider & Subscriber**

* **Sprint Goal:** Develop simple, functional reference modules demonstrating MCP provider and subscriber roles.
* **Key Features/Tasks:**
    * Create a basic `ReferenceProvider` module that:
        * Implements `IMCPProvider_V1`.
        * Registers a topic (e.g., "reference/counter") on `onAdd`.
        * Publishes serialized counter updates (e.g., via MessagePack) periodically or on trigger via the Broker.
        * Unregisters on `onRemove`.
    * Create a basic `ReferenceSubscriber` module that:
        * Implements `IMCPSubscriber_V1`.
        * Subscribes to "reference/counter" on `onAdd`.
        * Implements `onMCPMessage` to deserialize the counter value and display it (e.g., on a light or display widget). Requires using a thread-safe queue like `dsp::RingBuffer` to pass data from `onMCPMessage` (worker thread) to `process` (audio/GUI thread). (NFR-02, NFR-03).
        * Unsubscribes on `onRemove`.
    * Test the reference modules interacting within VCV Rack.
* **Shippable Increment Definition:** Two runnable VCV Rack modules (`ReferenceProvider`, `ReferenceSubscriber`) demonstrating the end-to-end MCP workflow: registration, subscription, publishing serialized data via MessagePack, receiving/deserializing data across threads using a ring buffer, and unregistration.

**Sprint 6: API Usability & Documentation**

* **Sprint Goal:** Refine the MCP API based on reference implementation experience and create comprehensive initial developer documentation.
* **Key Features/Tasks:**
    * Review and refine the C++ API (`IMCPBroker`, `IMCPProvider_V1`, `IMCPSubscriber_V1`, `MCPMessage_V1`) for clarity, consistency, and ease of use (NFR-07).
    * Develop comprehensive developer documentation covering:
        * MCP Concepts (Broker, Pub/Sub, Topics).
        * API Reference.
        * Step-by-step guide for implementing providers and subscribers (using reference modules as examples).
        * Serialization guidelines (MessagePack focus).
        * Thread-safety considerations and patterns (using `dsp::RingBuffer`).
    * Peer review documentation for clarity and accuracy.
* **Shippable Increment Definition:** A refined MCP API and a comprehensive set of developer documentation enabling third-party developers to understand and implement MCP provider/subscriber modules.

---

### Phase 4: Testing & Refinement

**Sprint 7: Performance & Scalability Testing**

* **Sprint Goal:** Rigorously test MCP performance and scalability under various conditions.
* **Key Features/Tasks:**
    * Develop performance testing scenarios (e.g., many providers, many subscribers, high message frequency, large message payloads).
    * Benchmark MCP operations (publish, subscribe, discovery) impact on audio thread CPU usage (NFR-01).
    * Test Broker performance under load (registry lookups, message dispatch throughput) (NFR-08).
    * Identify and address performance bottlenecks in the Broker or serialization/deserialization process.
    * Optimize locking strategies in the Broker if contention is observed.
    * Document performance characteristics and guidelines.
* **Shippable Increment Definition:** Performance benchmark results, identified bottlenecks addressed, and documented performance characteristics of the MCP system under tested loads.

**Sprint 8: Thread-Safety & Robustness Testing**

* **Sprint Goal:** Ensure the MCP system is robust and thread-safe through targeted testing and refinement.
* **Key Features/Tasks:**
    * Develop specific tests for thread-safety, aiming to induce race conditions or deadlocks in the Broker and cross-thread communication (NFR-03).
    * Test edge cases: module addition/removal during active publishing/subscribing, invalid messages, deserialization errors (NFR-04, NFR-05).
    * Implement more robust error handling and logging within the Broker and API functions.
    * Refine API stability and finalize V1 versioning strategy (NFR-09, NFR-10).
    * Conduct code reviews focusing on concurrency and error handling.
    * Update documentation with refined error handling and stability notes.
* **Shippable Increment Definition:** A hardened MCP implementation with improved robustness and verified thread-safety under stress test scenarios. Finalized V1 API contract.

---

### Phase 5: SDK Integration & Release Prep

**Sprint 9: SDK Integration & Final Docs**

* **Sprint Goal:** Integrate the MCP framework into the VCV Rack SDK and finalize all documentation.
* **Key Features/Tasks:**
    * Integrate the MCP Broker implementation and public API headers into the VCV Rack SDK build process (NFR-11).
    * Ensure necessary accessor functions (e.g., `getMCPBroker()`) are exposed correctly.
    * Perform final review and polish of all developer documentation.
    * Create examples for the SDK documentation.
    * Prepare release notes.
* **Shippable Increment Definition:** VCV Rack SDK incorporating the MCP framework. Finalized, comprehensive documentation and examples ready for release.

*(Further sprints might involve beta testing feedback, bug fixing, and potential development of standardized topic definitions or bridging modules as outlined in Future Considerations.)*

## 5. Key Considerations

* **Non-Functional Requirements:** NFRs are not an afterthought. Performance (NFR-01), Real-time Safety (NFR-02), and Thread Safety (NFR-03) are critical and addressed early (Broker implementation, Pub/Sub flow, Ring Buffers) and tested rigorously (Sprints 7, 8). API Usability (NFR-07) is addressed during API design and refinement (Sprint 6). Versioning (NFR-09) is established early (Sprint 1) and finalized (Sprint 8).
* **Documentation:** Documentation (NFR-07) is developed incrementally alongside features, starting in Sprint 2 and culminating in Sprint 6 and Sprint 9.
* **Testing:** Unit tests are created from Sprint 1. Integration tests start in Sprint 4. Dedicated performance and stress testing occurs in Sprints 7 and 8.
* **Feedback Loop:** While not explicitly a sprint task, gathering feedback from internal use (reference implementation) and potentially early adopters (if applicable) is crucial for API refinement.

## 6. Definition of Done (DoD)

For any feature or task to be considered "Done" within a sprint, it must meet the following criteria:

* **Code Complete:** All required code has been written.
* **Integrated:** Code is successfully integrated into the main MCP branch/build.
* **Unit Tested:** Sufficient unit tests covering the new logic have been written and are passing.
* **Integration Tested:** Relevant integration tests (if applicable for the feature) are passing.
* **Meets Requirements:** Functionality meets the acceptance criteria defined by the relevant FR/NFR.
* **Documented:** Necessary code comments, API documentation, or user-facing documentation updates are complete.
* **Peer Reviewed:** Code and documentation have been reviewed by at least one other team member.
* **Runnable Increment:** The changes contribute to a runnable, testable state of the overall MCP system (fulfilling the Shippable Increment constraint).
* **(Performance/Safety Context):** For relevant tasks, performance implications have been considered/measured, and thread-safety requirements have been met and tested.

