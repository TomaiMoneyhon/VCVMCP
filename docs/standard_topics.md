# MCP Standard Topics

This document catalogs standardized topics used across the VCV Rack ecosystem. Using these standard topics ensures interoperability between modules from different developers.

## Core System Topics

| Topic Name | Description | Message Format | Direction |
|------------|-------------|----------------|-----------|
| `mcp.system/broker-status` | Reports the status of the MCP broker | JSON status object | Broker → All |
| `mcp.system/registry-updates` | Notifies about provider/subscriber changes | JSON update object | Broker → All |
| `mcp.system/error-reports` | System-level error notifications | JSON error object | Any → Any |

## Clock and Transport

| Topic Name | Description | Message Format | Direction |
|------------|-------------|----------------|-----------|
| `com.vcvrack.core/clock` | Master clock signal | JSON with BPM, beat position | Clock Source → Listeners |
| `com.vcvrack.core/transport-state` | Play, stop, record status | JSON with state enum | Transport → Listeners |
| `com.vcvrack.core/time-signature` | Time signature information | JSON with num/denom | Time Source → Listeners |
| `com.vcvrack.core/song-position` | Current position in a song/pattern | JSON with position data | Sequencer → Listeners |

## MIDI Communication

| Topic Name | Description | Message Format | Direction |
|------------|-------------|----------------|-----------|
| `com.vcvrack.core/midi-notes` | MIDI note events | JSON with note/velocity/channel | Controllers → Receivers |
| `com.vcvrack.core/midi-cc` | MIDI CC messages | JSON with cc/value/channel | Controllers → Receivers |
| `com.vcvrack.core/midi-program-change` | Program change messages | JSON with program/channel | Controllers → Receivers |
| `com.vcvrack.core/midi-clock` | MIDI clock messages | JSON with tick count | Clock Source → Listeners |

## Modulation and Control

| Topic Name | Description | Message Format | Direction |
|------------|-------------|----------------|-----------|
| `com.vcvrack.core/cv` | Control voltage signals | JSON with value/range | Sources → Destinations |
| `com.vcvrack.core/gate` | Gate signals | JSON with state (on/off) | Sources → Destinations |
| `com.vcvrack.core/trigger` | Trigger impulses | JSON with trigger event | Sources → Destinations |
| `com.vcvrack.core/envelope` | Envelope generator states | JSON with ADSR values | Generators → Listeners |

## Audio Analysis

| Topic Name | Description | Message Format | Direction |
|------------|-------------|----------------|-----------|
| `com.vcvrack.core/spectrum` | Frequency spectrum data | JSON with spectrum array | Analyzers → Visualizers |
| `com.vcvrack.core/amplitude` | Audio amplitude/volume data | JSON with amplitude value | Analyzers → Visualizers |
| `com.vcvrack.core/pitch-detection` | Detected pitch information | JSON with frequency/note | Analyzers → Listeners |
| `com.vcvrack.core/tempo-detection` | Detected tempo information | JSON with BPM estimate | Analyzers → Listeners |

## Sequencer Communication

| Topic Name | Description | Message Format | Direction |
|------------|-------------|----------------|-----------|
| `com.vcvrack.core/sequence-data` | Sequence pattern data | JSON with sequence array | Sequencers → Listeners |
| `com.vcvrack.core/step-events` | Step trigger events | JSON with step index/state | Sequencers → Listeners |
| `com.vcvrack.core/pattern-change` | Pattern selection changes | JSON with pattern index | Controllers → Sequencers |
| `com.vcvrack.core/quantization` | Scale/quantization information | JSON with scale data | Scale Sources → Listeners |

## Module State Saving/Loading

| Topic Name | Description | Message Format | Direction |
|------------|-------------|----------------|-----------|
| `com.vcvrack.core/preset-load` | Request to load a preset | JSON with preset ID | Controllers → Modules |
| `com.vcvrack.core/preset-save` | Request to save a preset | JSON with preset data | Modules → Storage |
| `com.vcvrack.core/state-sync` | Module state synchronization | JSON with state data | Any → Any |

## Workflow and UI

| Topic Name | Description | Message Format | Direction |
|------------|-------------|----------------|-----------|
| `com.vcvrack.core/ui-event` | User interface events | JSON with event data | UI → Modules |
| `com.vcvrack.core/parameter-change` | Parameter value changes | JSON with param/value | Controllers → Modules |
| `com.vcvrack.core/focus-change` | Focus changes between modules | JSON with module ID | UI → Modules |

## Extending This List

If you develop a topic that may be useful to other module developers, consider submitting a pull request to add it to this standard list. Include:

1. A well-formed topic name following the [Topic Naming Conventions](topic_naming_conventions.md)
2. A clear description of the topic's purpose
3. A specification of the message format
4. Example use cases

## Message Format Details

Detailed message format specifications for each standard topic can be found in the [Message Formats](message_formats.md) document. 