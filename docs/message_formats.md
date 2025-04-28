# MCP Message Formats

This document provides detailed specifications for message formats used with standard MCP topics. Using these standardized message formats ensures interoperability between modules from different developers.

## JSON Standard Format

Most MCP messages use JSON as the data format. The standard structure is:

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

Where:
- `type`: Identifies the message type within the topic context
- `version`: Message schema version
- `timestamp`: UNIX timestamp in milliseconds
- `source`: Identifier of the message sender
- `data`: Message-specific payload (structure varies by message type)

## Core System Topic Formats

### `mcp.system/broker-status`

```json
{
  "type": "status",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "mcp.broker",
  "data": {
    "state": "running",
    "uptime": 3600,
    "providerCount": 15,
    "subscriberCount": 32,
    "topicCount": 24,
    "messageRate": 120.5
  }
}
```

### `mcp.system/registry-updates`

```json
{
  "type": "registry-update",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "mcp.broker",
  "data": {
    "action": "provider-added",
    "topic": "com.example.vendor/example-topic",
    "provider": "com.example.module-name"
  }
}
```

### `mcp.system/error-reports`

```json
{
  "type": "error",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.module-name",
  "data": {
    "severity": "warning",
    "code": "E1001",
    "message": "Failed to process message",
    "context": {
      "topic": "com.example.vendor/example-topic",
      "action": "subscribe"
    }
  }
}
```

## Clock and Transport Formats

### `com.vcvrack.core/clock`

```json
{
  "type": "clock-update",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.clock-module",
  "data": {
    "bpm": 120.0,
    "running": true,
    "beatPosition": 1.75,
    "barPosition": 1.75,
    "timeInSeconds": 3.5
  }
}
```

### `com.vcvrack.core/transport-state`

```json
{
  "type": "transport-update",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.transport-module",
  "data": {
    "state": "playing",  // "stopped", "playing", "recording", "paused"
    "looping": true,
    "loopStart": 0.0,
    "loopEnd": 16.0
  }
}
```

### `com.vcvrack.core/time-signature`

```json
{
  "type": "time-signature",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.transport-module",
  "data": {
    "numerator": 4,
    "denominator": 4,
    "beatsPerBar": 4,
    "beatUnit": "quarter"
  }
}
```

## MIDI Message Formats

### `com.vcvrack.core/midi-notes`

```json
{
  "type": "note",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.midi-module",
  "data": {
    "action": "note-on", // "note-on", "note-off"
    "note": 60,  // MIDI note number (0-127)
    "velocity": 100,  // 0-127
    "channel": 0  // MIDI channel (0-15)
  }
}
```

### `com.vcvrack.core/midi-cc`

```json
{
  "type": "cc",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.midi-module",
  "data": {
    "controller": 1,  // MIDI CC number (0-127)
    "value": 64,      // 0-127
    "channel": 0      // MIDI channel (0-15)
  }
}
```

## Modulation and Control Formats

### `com.vcvrack.core/cv`

```json
{
  "type": "cv",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.control-module",
  "data": {
    "value": 2.5,       // CV value in volts
    "min": -5.0,        // Min range
    "max": 5.0,         // Max range
    "smoothing": 0.01   // Optional smoothing time (seconds)
  }
}
```

### `com.vcvrack.core/gate`

```json
{
  "type": "gate",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.gate-module",
  "data": {
    "state": true,      // true = high, false = low
    "voltage": 5.0      // Optional voltage level when high
  }
}
```

### `com.vcvrack.core/trigger`

```json
{
  "type": "trigger",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.trigger-module",
  "data": {
    "duration": 0.001,  // Trigger duration in seconds
    "voltage": 5.0      // Trigger voltage
  }
}
```

## Audio Analysis Formats

### `com.vcvrack.core/spectrum`

```json
{
  "type": "spectrum",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.analyzer-module",
  "data": {
    "bins": [0.1, 0.2, 0.3, ...],  // Spectrum magnitude values
    "binCount": 1024,              // Number of frequency bins
    "minFreq": 20.0,               // Lowest frequency (Hz)
    "maxFreq": 20000.0,            // Highest frequency (Hz)
    "scale": "log"                 // "log" or "linear" frequency scale
  }
}
```

### `com.vcvrack.core/pitch-detection`

```json
{
  "type": "pitch",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.pitch-detector",
  "data": {
    "frequency": 440.0,     // Detected frequency in Hz
    "confidence": 0.95,     // Detection confidence (0-1)
    "midi_note": 69,        // Closest MIDI note
    "cents_offset": -5.0    // Cents sharp/flat from exact pitch
  }
}
```

## Sequencer Communication Formats

### `com.vcvrack.core/sequence-data`

```json
{
  "type": "sequence",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.sequencer-module",
  "data": {
    "pattern": 0,           // Pattern index
    "length": 16,           // Sequence length
    "steps": [              // Array of step data
      {
        "note": 60,         // MIDI note or -1 for rest
        "velocity": 100,    // 0-127
        "duration": 0.9,    // Note duration as fraction of step
        "probability": 1.0, // Trigger probability (0-1)
        "enabled": true     // Whether step is enabled
      },
      // ... more steps
    ]
  }
}
```

### `com.vcvrack.core/step-events`

```json
{
  "type": "step-event",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.sequencer-module",
  "data": {
    "pattern": 0,          // Pattern index
    "step": 3,             // Step index that was triggered
    "state": "active",     // "active", "inactive"
    "repeatCount": 0       // How many times this step has repeated
  }
}
```

## Module State Formats

### `com.vcvrack.core/preset-load`

```json
{
  "type": "preset-load",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.preset-manager",
  "data": {
    "preset_id": "factory-1",
    "target_module": "com.example.target-module",
    "category": "bass"
  }
}
```

### `com.vcvrack.core/state-sync`

```json
{
  "type": "state-sync",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.module-a",
  "data": {
    "parameters": {
      "param1": 0.5,
      "param2": 0.75
    },
    "options": {
      "option1": "value1",
      "option2": true
    },
    "connections": [
      // connection data
    ]
  }
}
```

## Binary Data Encoding

For messages containing binary data (like audio samples or large datasets), the recommendation is to:

1. Use base64 encoding for the binary data
2. Include metadata about the binary format

Example:

```json
{
  "type": "audio-data",
  "version": "1.0",
  "timestamp": 1625097600000,
  "source": "com.example.sampler-module",
  "data": {
    "format": "f32le",    // 32-bit float, little-endian
    "sampleRate": 44100,
    "channels": 2,
    "frames": 1024,
    "samples": "base64EncodedDataHere..."
  }
}
```

## Extending Message Formats

When extending message formats for custom topics:

1. Follow the base structure with `type`, `version`, `timestamp`, `source`, and `data`
2. Document your message format clearly
3. Include version information to support future changes
4. Consider backward compatibility
5. Use standard data types and units when possible

## Best Practices

1. Keep messages as small as practical for efficient transport
2. Use appropriate numeric precision
3. Include only necessary fields
4. Default to standard units (seconds, Hertz, volts)
5. For timing-critical applications, ensure the `timestamp` is accurate
6. Consider including a `sessionId` for related message sequences 