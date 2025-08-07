# kpacket - ZMQ Packet Relay Design

## Overview

kpacket is a comprehensive packet relay system for Final Fantasy XI that captures, analyzes, and streams game packets over ZeroMQ (ZMQ). It integrates with the Ashita plugin framework to intercept packets at the network layer and provides real-time streaming and command interfaces for external applications.

## Architecture

### Core Components

1. **Packet Interceptor** - Integrates with Ashita's `HandleIncomingPacket` and `HandleOutgoingPacket` events
2. **Packet Classifier** - Identifies and categorizes packets based on the XiPackets documentation
3. **ZMQ Relay System** - Multi-socket ZMQ architecture for streaming and control
4. **Packet Serializer** - JSON-based packet serialization with metadata

### ZMQ Socket Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   FFXI Client   │    │  kpacket Plugin │    │ External Clients│
├─────────────────┤    ├──────────────────┤    ├─────────────────┤
│ Incoming Packets│───▶│ Packet Interceptor│───▶│ Analysis Tools  │
│ Outgoing Packets│◀───│ Packet Injector  │◀───│ Bot Clients     │
└─────────────────┘    └──────────────────┘    │ Monitoring Apps │
                                ▲               │ Debug Tools     │
                                │               └─────────────────┘
                       ┌────────▼────────┐
                       │  ZMQ Relay      │
                       ├─────────────────┤
                       │ PUB:5555 (Stream)│
                       │ REP:5556 (Command)│
                       │ PUSH:5557 (Queue)│
                       └─────────────────┘
```

### Socket Types and Purposes

1. **PUB/SUB (Port 5555)** - Real-time packet streaming
   - High-performance, fire-and-forget packet distribution
   - Topic-based filtering (e.g., `packet.0x000A`)
   - Multiple subscribers supported

2. **REQ/REP (Port 5556)** - Command and control interface
   - Synchronous command processing
   - Plugin status queries
   - Packet injection commands
   - Configuration changes

3. **PUSH/PULL (Port 5557)** - Reliable packet queuing
   - Guaranteed delivery for critical packets
   - Load balancing across multiple workers
   - Persistence for offline analysis

## Packet Classification System

### Packet Types

Based on the XiPackets documentation, packets are classified into:

- **World Packets** (0x0005-0x011E): Gameplay communication
- **Lobby Packets** (Various): Character selection and login
- **Patch Packets** (0x0001-0x0008): Update-related communication

### Packet Headers

Different packet types use different header formats:

**World Packets (4 bytes):**
```cpp
struct WorldHeader {
    uint16_t id : 9;     // Packet ID (0-511)
    uint16_t size : 7;   // Size / 4 (0-127 * 4 = 508 bytes max)
    uint16_t sync;       // Synchronization counter
};
```

**Lobby Packets (16 bytes):**
```cpp
struct LobbyHeader {
    uint32_t packet_size;    // Total packet size
    uint32_t terminator;     // Fixed terminator value
    uint32_t command;        // Command/packet ID
    uint8_t identifier[16];  // Session identifier
};
```

## Message Format

All packets are serialized to JSON with the following structure:

```json
{
  "timestamp": 1640995200000,
  "direction": "incoming|outgoing",
  "packet_type": "world_c2s|world_s2c|lobby_c2s|lobby_s2c|patch_c2s|patch_s2c",
  "packet_id": 10,
  "packet_name": "GP_CLI_COMMAND_LOGIN",
  "size": 92,
  "data": "base64_encoded_packet_data",
  "metadata": {
    "injected": false,
    "blocked": false,
    "chunk_size": 256,
    "session_id": "session_1640995200000",
    "sync_count": 12345
  },
  "chunk_data": "base64_encoded_chunk_data"
}
```

## Command Interface

The REQ/REP socket accepts JSON commands:

### Status Command
```json
{
  "command": "status",
  "params": {}
}
```

Response:
```json
{
  "status": "running",
  "uptime": 3600,
  "config": {
    "pub_endpoint": "tcp://*:5555",
    "rep_endpoint": "tcp://*:5556",
    "push_endpoint": "tcp://*:5557",
    "packet_streaming": true,
    "command_interface": true,
    "reliable_queue": true
  }
}
```

### Statistics Command
```json
{
  "command": "stats",
  "params": {}
}
```

Response:
```json
{
  "stats": {
    "packets_published": 15420,
    "commands_processed": 23,
    "packets_injected": 0,
    "packets_filtered": 1205,
    "queue_size": 0
  }
}
```

### Packet Injection Command
```json
{
  "command": "inject",
  "params": {
    "id": 10,
    "data": "base64_encoded_packet_data"
  }
}
```

Response:
```json
{
  "success": true
}
```

## Performance Considerations

### Threading Model

- **Main Thread**: Packet interception and immediate publishing
- **Publisher Thread**: Background packet streaming management
- **Command Thread**: Synchronous command processing
- **Queue Thread**: Reliable packet queue processing

### Memory Management

- Packet data is copied to avoid lifetime issues
- Base64 encoding for binary data transmission
- Configurable queue size limits to prevent memory exhaustion
- Smart pointer usage for automatic resource management

### Network Optimization

- Topic-based filtering reduces network traffic
- Batch processing for high-volume scenarios
- Configurable socket buffer sizes
- Non-blocking operations where possible

## Security Considerations

### Data Protection

- Packet data may contain sensitive information
- Consider encryption for network transmission
- Access control for command interface
- Audit logging for packet injection

### Network Exposure

- Default configuration binds to all interfaces (`*`)
- Consider firewall rules and network segmentation
- Authentication mechanisms for production use
- Rate limiting for command interface

## Integration Points

### Ashita Plugin Framework

- Implements `IPlugin` interface
- Uses Ashita's packet interception events
- Integrates with Ashita's logging system
- Follows Ashita's plugin lifecycle

### XiPackets Documentation

- Packet names and structures from XiPackets repository
- Automatic classification based on documented ranges
- Extensible mapping system for new packets
- Version-aware packet handling

## Future Enhancements

### Planned Features

1. **Packet Filtering**: Advanced filtering by content, size, frequency
2. **Packet Modification**: Real-time packet modification capabilities
3. **Recording/Playback**: Capture sessions for later analysis
4. **Web Interface**: HTTP API and web dashboard
5. **Database Integration**: Persistent storage for packet analysis
6. **Machine Learning**: Anomaly detection and pattern recognition

### Extensibility

The modular design allows for:
- Custom packet handlers
- Additional socket types
- Alternative serialization formats
- Plugin-based architecture for analyzers
- External tool integration