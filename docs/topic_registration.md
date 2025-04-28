# Topic Registration Process

This document outlines the process for registering new topics in the MCP ecosystem. Registration helps prevent naming conflicts and promotes standardization across the ecosystem.

## Registration Types

Topics in the MCP system can be categorized into three types:

1. **Standard Topics**: Official topics maintained in the VCV Rack MCP documentation
2. **Vendor-Specific Topics**: Topics using a vendor's domain namespace
3. **Private Topics**: Internal topics not intended for cross-module communication

## When to Register a Topic

Registration is:
- **Required** for Standard Topics
- **Recommended** for Vendor-Specific Topics that may be useful to other developers
- **Not needed** for Private Topics used only within your own modules

## Registration Process for Standard Topics

To register a new standard topic (under `com.vcvrack.core` or other official namespaces):

1. **Check Existing Topics**
   - Review the [Standard Topics](standard_topics.md) document
   - Ensure your proposed topic doesn't duplicate existing functionality

2. **Follow Naming Conventions**
   - Adhere to the [Topic Naming Conventions](topic_naming_conventions.md)
   - Choose a meaningful, descriptive name

3. **Define Message Format**
   - Create a detailed message format specification
   - Follow the patterns in the [Message Formats](message_formats.md) document

4. **Submit a Proposal**
   - Fork the MCP repository
   - Add your topic to the appropriate section in the standard_topics.md document
   - Add your message format to message_formats.md
   - Submit a pull request with:
     - Topic name and description
     - Message format details
     - Example use cases
     - Rationale for standardization

5. **Review Process**
   - The proposal will be reviewed by the VCV Rack team
   - Community feedback may be solicited
   - Changes may be requested before acceptance

6. **Acceptance**
   - Once approved, your topic will be added to the official documentation
   - Your topic is now available for use by all developers

## Registration Process for Vendor-Specific Topics

For topics in your own namespace (e.g., `com.yourcompany.yourproduct`):

1. **Document Internally**
   - Maintain internal documentation of your topics
   - Follow the [Topic Naming Conventions](topic_naming_conventions.md)

2. **Optional Registration**
   - If your topic may be useful to other developers, consider adding it to the [Community Topics Registry](https://github.com/vcvrack/mcp-community-topics)
   - This improves discoverability and prevents accidental namespace collisions

3. **Community Registry Submission**
   - Fork the community topics repository
   - Add your topic information
   - Submit a pull request

## Topic Documentation Requirements

When registering a topic, provide the following information:

### Basic Information
- **Topic Name**: The full topic name
- **Description**: A clear, concise description of the topic's purpose
- **Namespace**: Which namespace the topic belongs to
- **Version**: Current version of the topic definition

### Message Format
- **Structure**: The expected message structure
- **Required Fields**: Fields that must be present
- **Optional Fields**: Fields that may be included
- **Field Types**: Expected data types for each field
- **Units**: Units of measurement for numeric values

### Usage Information
- **Direction**: Expected flow of messages (provider→subscriber)
- **Frequency**: How often messages are typically sent
- **Real-time Requirements**: Any timing constraints
- **Example Code**: Simple usage examples

## Reserved Namespaces

The following namespaces are reserved for official use:

- `mcp.system/*`: Core MCP system functionality
- `com.vcvrack.core/*`: Standard VCV Rack functionality
- `com.vcvrack.community/*`: Community-maintained standards

## Version Control of Topics

When making changes to an existing topic:

1. **Backward Compatible Changes**
   - Adding optional fields
   - Extending valid values for existing fields
   - Update the minor version number (1.0 → 1.1)

2. **Breaking Changes**
   - Removing or renaming fields
   - Changing field types
   - Create a new topic name with a version suffix
   - e.g., `com.vcvrack.core/clock-v2`

## Deprecation Process

When a topic needs to be deprecated:

1. Mark it as deprecated in the documentation
2. Provide the replacement topic name
3. Set a timeline for removal (typically 6-12 months)
4. Continue supporting the deprecated topic during the transition period

## Community Support

For questions about the topic registration process, contact:
- The VCV Rack forum: [https://community.vcvrack.com/](https://community.vcvrack.com/)
- The MCP GitHub repository: [https://github.com/vcvrack/mcp](https://github.com/vcvrack/mcp) 