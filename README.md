# kpacket - Advanced FFXI Packet Relay System

A high-performance ZeroMQ-based packet relay system for Final Fantasy XI that captures, analyzes, and streams game packets in real-time.

## Features

- **Real-time Packet Streaming** - Live packet capture and distribution via ZeroMQ
- **Multi-Socket Architecture** - PUB/SUB for streaming, REQ/REP for commands, PUSH/PULL for reliability
- **Packet Classification** - Automatic categorization based on XiPackets documentation
- **JSON Serialization** - Human-readable packet format with metadata
- **Command Interface** - Remote control and packet injection capabilities
- **High Performance** - Multi-threaded design with minimal latency
- **Extensible** - Modular architecture for custom analyzers and tools

## Quick Start

### Prerequisites

- Windows 10/11
- Visual Studio 2019/2022 or compatible C++ compiler
- CMake 3.22+
- vcpkg package manager
- Ashita v4 SDK

### Required Dependencies

Install via vcpkg:
```bash
vcpkg install cppzmq nlohmann-json
```

### Building

1. Clone the repository with submodules
2. Configure with CMake: `cmake --preset=default`
3. Build: `cmake --build build --config Release`
4. Plugin will be copied to Ashita plugins directory automatically

### Usage

1. Load the plugin in Ashita: `/load kpacket`
2. Start the packet monitor client: `./bin/examples/packet_monitor.exe`
3. Connect your tools to ZMQ endpoints:
   - `tcp://localhost:5555` - Packet streaming (SUB)
   - `tcp://localhost:5556` - Commands (REQ)
   - `tcp://localhost:5557` - Reliable queue (PULL)

## Packet Format

Packets are streamed as JSON messages with comprehensive metadata including timestamp, direction, packet type, and base64-encoded data.

## Documentation

See [docs/DESIGN.md](docs/DESIGN.md) for detailed architecture and implementation information.

## License

GNU Lesser General Public License v3.0