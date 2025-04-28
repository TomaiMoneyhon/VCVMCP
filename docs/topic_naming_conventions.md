# MCP Topic Naming Conventions

This document outlines the standardized conventions for naming topics in the MCP system. Adhering to these guidelines ensures consistency, prevents naming conflicts, and improves discoverability.

## General Structure

Topic names in MCP follow a URI-style format consisting of two parts:

```
namespace/resource
```

Example: `com.vcvrack.core/clock`

## Namespace Guidelines

The namespace portion of the topic name should follow these rules:

1. **Reverse Domain Notation**: Use reverse domain notation to ensure global uniqueness.
   - Example: `com.yourcompany.pluginname`

2. **Lower Case**: Use only lowercase letters for namespaces.
   - Correct: `com.vcvrack.core`
   - Incorrect: `Com.VCVRack.Core`

3. **No Special Characters**: Avoid using special characters except periods (`.`) as separators.
   - Correct: `org.thirdparty.utilities`
   - Incorrect: `org_thirdparty-utilities`

4. **Vendor Identification**: For commercial plugins, start with your company's domain.
   - Example: `com.yourcompany.pluginname`

5. **Open Source Projects**: For open-source projects without a domain, use `org.github.username` or similar.
   - Example: `org.github.username.coolmodule`

6. **Standard Library**: Core VCV Rack functionalities use the `com.vcvrack.core` namespace.
   - Example: `com.vcvrack.core/midi`

## Resource Guidelines

The resource portion of the topic name should follow these rules:

1. **Descriptive Names**: Use clear, descriptive names that indicate the purpose of the topic.
   - Example: `clock`, `pitch`, `sequencer-state`

2. **Lower Case**: Use lowercase letters for resource names.
   - Correct: `clock`
   - Incorrect: `Clock`

3. **Hyphens for Spaces**: Use hyphens to separate words in multi-word resource names.
   - Example: `gate-trigger`, `note-events`

4. **No Special Characters**: Avoid using special characters except hyphens (`-`).
   - Correct: `midi-note`
   - Incorrect: `midi_note` or `midi.note`

5. **Singular Nouns**: Use singular nouns for resources representing a single entity.
   - Example: `clock`, `trigger`, `cv`

6. **Plural for Collections**: Use plural nouns for resources representing collections.
   - Example: `notes`, `events`, `parameters`

## Versioning in Topics

For topics that may change their message format over time, include a version indicator in the resource name:

```
namespace/resource-v1
```

Example: `com.vcvrack.core/clock-v1`

When making breaking changes to a message format, increment the version number rather than changing the existing format:

```
com.vcvrack.core/clock-v1  # Original format
com.vcvrack.core/clock-v2  # New format with breaking changes
```

## Specialized Topic Categories

### System Topics

System topics used by the MCP framework itself use the `mcp.system` namespace:

- `mcp.system/broker-status`
- `mcp.system/registry-updates`
- `mcp.system/error-reports`

### Module-specific Topics

For module-specific features, include the module name in the resource:

```
com.yourcompany.pluginname/modulename-feature
```

Example: `com.vcvrack.fundamental/vco-settings`

### Hierarchical Resources

For related resources, use a hierarchical approach with hyphens:

```
namespace/parent-child-grandchild
```

Example: `com.vcvrack.core/midi-note-events`

## Examples of Well-formed Topic Names

Here are examples of properly formatted topic names:

- `com.vcvrack.core/clock`
- `com.vcvrack.core/transport-state`
- `com.vcvrack.core/midi-notes`
- `com.yourcompany.sequencer/pattern-data-v1`
- `org.github.username.utilities/quantizer-scales`
- `com.vcvrack.fundamental/scope-settings`

## Examples to Avoid

These are examples of poorly formatted topic names:

- `Clock` (no namespace, uppercase)
- `vcvrack/MIDI_Notes` (incorrect namespace, underscore, uppercase)
- `com.yourcompany/My Feature` (spaces in resource name)
- `my-module/stuff` (non-standard namespace)
- `com.vcvrack.core.clock` (using period instead of slash between namespace and resource)

## Registration Process

New public topics intended for cross-module communication should be documented in your module's documentation. Consider submitting widely useful topic specifications to the MCP documentation repository for inclusion in the standard topics list.

## Standard Topics

A list of standard topics used in the VCV Rack ecosystem can be found in the [Standard Topics](standard_topics.md) document. 