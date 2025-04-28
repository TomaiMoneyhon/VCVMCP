# MCP System Glossary

This document defines key terms used throughout the MCP documentation and codebase.

## Core Concepts

### MCP (Module Communication Protocol)
A standardized protocol that allows VCV Rack modules to communicate with each other by publishing and subscribing to messages on topics.

### Broker
The central component of the MCP system that manages the registration of providers and subscribers, and facilitates message delivery between them.

### Provider
A module that publishes data on a specific topic, making it available to subscribers.

### Subscriber
A module that subscribes to a specific topic to receive data from providers.

### Topic
A named channel used to categorize and route messages between providers and subscribers. Topics are identified by a URI-style string (e.g., "com.vcvrack.core/clock").

### Message
A discrete unit of data published by a provider and received by subscribers. Messages have a specific format and content type.

### Content Type
A MIME-style identifier that describes the format of a message (e.g., "application/msgpack" or "application/json").

### Worker Thread
A background thread used by the broker to process messages without blocking the audio thread.

### Registry
The broker's internal database of providers and subscribers, used to match subscribers with providers for specific topics.

## Technical Terms

### Real-time Safety
The property of a function or method that guarantees it will not block or take an unpredictable amount of time to execute, making it safe to call from the audio thread.

### Lock-free
A programming technique that avoids using mutexes or locks to synchronize access to shared data, often using atomic operations instead.

### Ring Buffer
A circular data structure with a fixed size that allows data to be added to one end and removed from the other without blocking, commonly used for communication between threads.

### Thread Safety
The property of code that ensures it functions correctly when accessed from multiple threads simultaneously.

### Audio Thread
The high-priority thread in VCV Rack that processes audio, where blocking operations can cause audio dropouts.

### Interface
A pure virtual class in C++ that defines a contract for derived classes, used extensively in MCP to define standard APIs.

### Implementation
A concrete class that implements one or more interfaces, providing the actual functionality.

### Serialization
The process of converting in-memory data structures to a format that can be stored or transmitted (like MessagePack or JSON), and later reconstructed.

### Deserialization
The reverse process of serialization, converting data from a stored or transmitted format back into in-memory data structures.

### MessagePack
A binary serialization format similar to JSON but more compact and efficient, used as the primary serialization format in MCP.

## MCP-Specific Terms

### IMCPBroker
The interface that defines the public API of the MCP broker, exposing methods for registration, publishing, and subscription.

### MCPBroker
The implementation of the IMCPBroker interface, providing the actual broker functionality.

### IMCPProvider
The interface that providers must implement to publish data on topics.

### IMCPSubscriber
The interface that subscribers must implement to receive data from topics they subscribe to.

### Topic URI
The standardized string format used to identify topics, typically following a reverse domain name pattern (e.g., "com.company.plugin/feature").

### Message Handler
A callback function provided by a subscriber that is called when a message is received on a subscribed topic.

### Broker Singleton
The single instance of the MCPBroker that exists within a VCV Rack session, accessible through the `getBroker()` function.

### Weak References
References to objects that don't prevent those objects from being destroyed, used in MCP to avoid memory leaks and dangling references.

### Format Version
A version number included in serialized data to ensure compatibility between different versions of the serialization format.

### Provider Registry
The part of the broker that keeps track of registered providers and their topics.

### Subscriber Registry
The part of the broker that keeps track of subscribers and their topic subscriptions.

### Message Queue
The internal data structure used by the broker to store messages before they are dispatched to subscribers.

### Dispatch Worker
The worker thread responsible for taking messages from the queue and delivering them to subscribers.

## API Versioning

### IMCPProvider_V1
The first version of the provider interface, subject to possible changes in future versions.

### IMCPSubscriber_V1
The first version of the subscriber interface, subject to possible changes in future versions.

### IMCPBroker_V1
The first version of the broker interface, subject to possible changes in future versions.

## Best Practices Terms

### Idempotent
A property of an operation where applying it multiple times has the same effect as applying it once, important for message handling.

### Stateless
A design approach where operations do not depend on previous operations or state, making the system more robust and easier to reason about.

### Thread-local Storage
Memory that is local to a specific thread, used to avoid sharing data between threads when not necessary.

### Backpressure
A mechanism to handle situations where a producer generates data faster than a consumer can process it, preventing resource exhaustion.

### Clean Shutdown
A graceful termination process where resources are properly released and pending operations are completed or canceled. 