/*
 * Ashita kpacket Plugin - Copyright (c) 2025 Poroburu Development Team
 * Contact: https://www.ashitaxi.com/
 * Contact: https://discord.gg/Ashita
 *
 * This file is part of Ashita kpacket Plugin.
 *
 * Ashita kpacket Plugin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ashita Example Plugin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Ashita Example Plugin.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace kpacket {

enum class PacketType {
    WORLD_CLIENT_TO_SERVER,
    WORLD_SERVER_TO_CLIENT,
    LOBBY_CLIENT_TO_SERVER,
    LOBBY_SERVER_TO_CLIENT,
    PATCH_CLIENT_TO_SERVER,
    PATCH_SERVER_TO_CLIENT,
    UNKNOWN
};

enum class Direction {
    INCOMING,
    OUTGOING
};

struct PacketHeader {
    // World packet header (4 bytes)
    struct WorldHeader {
        uint16_t id : 9;
        uint16_t size : 7;
        uint16_t sync;
    };
    
    // Lobby packet header (28 bytes)
    struct LobbyHeader {
        uint32_t packet_size;
        uint32_t terminator;
        uint32_t command;
        uint8_t identifier[16];
    };
    
    PacketType type;
    union {
        WorldHeader world;
        LobbyHeader lobby;
    };
};

struct PacketInfo {
    uint64_t timestamp;
    Direction direction;
    PacketType type;
    uint16_t id;
    std::string name;
    uint32_t size;
    std::vector<uint8_t> data;
    
    // Metadata
    bool injected;
    bool blocked;
    uint32_t chunk_size;
    std::string session_id;
    
    // Additional context
    uint16_t sync_count;
    std::vector<uint8_t> chunk_data;
};

// Packet classification utilities
class PacketClassifier {
public:
    static PacketType ClassifyPacket(uint16_t id, Direction direction);
    static std::string GetPacketName(uint16_t id, PacketType type);
    static bool IsWorldPacket(uint16_t id);
    static bool IsLobbyPacket(uint16_t id);
    static bool IsPatchPacket(uint16_t id);
};

// Packet parsing utilities
class PacketParser {
public:
    static PacketHeader ParseHeader(const uint8_t* data, PacketType type);
    static PacketInfo CreatePacketInfo(
        uint16_t id, 
        uint32_t size, 
        const uint8_t* data,
        Direction direction,
        bool injected = false,
        bool blocked = false,
        uint32_t chunk_size = 0,
        const uint8_t* chunk_data = nullptr
    );
};

} // namespace kpacket