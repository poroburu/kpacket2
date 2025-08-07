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
#include "packet_types.hpp"
#include <unordered_map>
#include <cstring>

namespace kpacket {

// Known packet mappings based on XiPackets documentation
static const std::unordered_map<uint16_t, std::string> world_client_packets = {
    {0x000A, "GP_CLI_COMMAND_LOGIN"},
    {0x000B, "GP_CLI_COMMAND_LOGOUT"},
    {0x000C, "GP_CLI_COMMAND_GAMEOK"},
    {0x000D, "GP_CLI_COMMAND_NETEND"},
    {0x000F, "GP_CLI_COMMAND_CLSTAT"},
    {0x0015, "GP_CLI_COMMAND_POS"},
    {0x0016, "GP_CLI_COMMAND_CHARREQ"},
    {0x0017, "GP_CLI_COMMAND_CHARREQ2"},
    {0x001A, "GP_CLI_COMMAND_ACTION"},
    {0x001B, "GP_CLI_COMMAND_FRIENDPASS"},
    {0x001E, "GP_CLI_COMMAND_GM"},
    {0x001F, "GP_CLI_COMMAND_GMCOMMAND"},
    {0x0028, "GP_CLI_COMMAND_ITEM_DUMP"},
    {0x0029, "GP_CLI_COMMAND_ITEM_MOVE"},
    {0x002A, "GP_CLI_COMMAND_ITEM_ATTR"},
    {0x0032, "GP_CLI_COMMAND_ITEM_TRADE_REQ"},
    {0x0033, "GP_CLI_COMMAND_ITEM_TRADE_RES"},
    {0x0034, "GP_CLI_COMMAND_ITEM_TRADE_LIST"},
    {0x0035, "GP_CLI_COMMAND_ITEM_PRESENT"},
    {0x0036, "GP_CLI_COMMAND_ITEM_TRANSFER"},
    {0x0037, "GP_CLI_COMMAND_ITEM_USE"},
    {0x0038, "GP_CLI_COMMAND_ITEM_MAKE"},
    {0x005D, "GP_CLI_COMMAND_MOTION"},
    {0x00B5, "GP_CLI_COMMAND_CHAT_STD"},
    {0x00B6, "GP_CLI_COMMAND_CHAT_NAME"}
    // Add more as needed...
};

static const std::unordered_map<uint16_t, std::string> world_server_packets = {
    {0x0005, "GP_SERV_COMMAND_PACKETCONTROL"},
    {0x0006, "GP_SERV_COMMAND_NARAKU"},
    {0x0008, "GP_SERV_COMMAND_ENTERZONE"},
    {0x0009, "GP_SERV_COMMAND_MESSAGE"},
    {0x000A, "GP_SERV_COMMAND_LOGIN"},
    {0x000B, "GP_SERV_COMMAND_LOGOUT"},
    {0x000D, "GP_SERV_COMMAND_CHAR_PC"},
    {0x000E, "GP_SERV_COMMAND_CHAR_NPC"},
    {0x0011, "GP_SERV_COMMAND_CHAR_DEL"},
    {0x0017, "GP_SERV_COMMAND_CHAT_STD"},
    {0x001C, "GP_SERV_COMMAND_ITEM_MAX"},
    {0x001D, "GP_SERV_COMMAND_ITEM_SAME"},
    {0x001E, "GP_SERV_COMMAND_ITEM_NUM"},
    {0x001F, "GP_SERV_COMMAND_ITEM_LIST"},
    {0x0020, "GP_SERV_COMMAND_ITEM_ATTR"},
    {0x005A, "GP_SERV_COMMAND_MOTIONMES"}
    // Add more as needed...
};

static const std::unordered_map<uint16_t, std::string> lobby_packets = {
    {0x0007, "RequestSelectChr"},
    {0x0014, "RequestDeleteChr"},
    {0x001F, "RequestGetChr"},
    {0x0021, "RequestCreateChr"},
    {0x0022, "RequestCreateChrPre"},
    {0x0024, "RequestQueryWorldList"},
    {0x0026, "RequestLobbyLogin"},
    {0x0028, "RequestRenameChr"},
    {0x002B, "RequestMoveGMChr"}
    // Add more as needed...
};

PacketType PacketClassifier::ClassifyPacket(uint16_t id, Direction direction) {
    // World packets are the most common (0x0005-0x011E range)
    if (id >= 0x0005 && id <= 0x011E) {
        return direction == Direction::INCOMING ? 
            PacketType::WORLD_SERVER_TO_CLIENT : 
            PacketType::WORLD_CLIENT_TO_SERVER;
    }
    
    // Lobby packets (various ranges, check specific IDs)
    if (lobby_packets.find(id) != lobby_packets.end()) {
        return direction == Direction::INCOMING ?
            PacketType::LOBBY_SERVER_TO_CLIENT :
            PacketType::LOBBY_CLIENT_TO_SERVER;
    }
    
    // Patch packets (0x0001-0x0008 range)
    if (id >= 0x0001 && id <= 0x0008) {
        return direction == Direction::INCOMING ?
            PacketType::PATCH_SERVER_TO_CLIENT :
            PacketType::PATCH_CLIENT_TO_SERVER;
    }
    
    return PacketType::UNKNOWN;
}

std::string PacketClassifier::GetPacketName(uint16_t id, PacketType type) {
    switch (type) {
        case PacketType::WORLD_CLIENT_TO_SERVER: {
            auto it = world_client_packets.find(id);
            return it != world_client_packets.end() ? it->second : "UNKNOWN_WORLD_CLIENT";
        }
        case PacketType::WORLD_SERVER_TO_CLIENT: {
            auto it = world_server_packets.find(id);
            return it != world_server_packets.end() ? it->second : "UNKNOWN_WORLD_SERVER";
        }
        case PacketType::LOBBY_CLIENT_TO_SERVER:
        case PacketType::LOBBY_SERVER_TO_CLIENT: {
            auto it = lobby_packets.find(id);
            return it != lobby_packets.end() ? it->second : "UNKNOWN_LOBBY";
        }
        case PacketType::PATCH_CLIENT_TO_SERVER:
            return "PATCH_CLIENT_" + std::to_string(id);
        case PacketType::PATCH_SERVER_TO_CLIENT:
            return "PATCH_SERVER_" + std::to_string(id);
        default:
            return "UNKNOWN_PACKET";
    }
}

bool PacketClassifier::IsWorldPacket(uint16_t id) {
    return id >= 0x0005 && id <= 0x011E;
}

bool PacketClassifier::IsLobbyPacket(uint16_t id) {
    return lobby_packets.find(id) != lobby_packets.end();
}

bool PacketClassifier::IsPatchPacket(uint16_t id) {
    return id >= 0x0001 && id <= 0x0008;
}

PacketHeader PacketParser::ParseHeader(const uint8_t* data, PacketType type) {
    PacketHeader header;
    header.type = type;
    
    if (type == PacketType::WORLD_CLIENT_TO_SERVER || 
        type == PacketType::WORLD_SERVER_TO_CLIENT) {
        // World packet header parsing
        uint16_t first_word = *reinterpret_cast<const uint16_t*>(data);
        header.world.id = first_word & 0x01FF;
        header.world.size = (first_word >> 9) & 0x007F;
        header.world.sync = *reinterpret_cast<const uint16_t*>(data + 2);
    } else if (type == PacketType::LOBBY_CLIENT_TO_SERVER || 
               type == PacketType::LOBBY_SERVER_TO_CLIENT) {
        // Lobby packet header parsing
        header.lobby.packet_size = *reinterpret_cast<const uint32_t*>(data);
        header.lobby.terminator = *reinterpret_cast<const uint32_t*>(data + 4);
        header.lobby.command = *reinterpret_cast<const uint32_t*>(data + 8);
        std::memcpy(header.lobby.identifier, data + 12, 16);
    }
    
    return header;
}

PacketInfo PacketParser::CreatePacketInfo(
    uint16_t id, 
    uint32_t size, 
    const uint8_t* data,
    Direction direction,
    bool injected,
    bool blocked,
    uint32_t chunk_size,
    const uint8_t* chunk_data) {
    
    PacketInfo info;
    
    // Set timestamp
    auto now = std::chrono::system_clock::now();
    info.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Basic packet info
    info.direction = direction;
    info.id = id;
    info.size = size;
    info.data.assign(data, data + size);
    
    // Classify packet type
    info.type = PacketClassifier::ClassifyPacket(id, direction);
    info.name = PacketClassifier::GetPacketName(id, info.type);
    
    // Metadata
    info.injected = injected;
    info.blocked = blocked;
    info.chunk_size = chunk_size;
    
    // Parse header for sync count if it's a world packet
    if (PacketClassifier::IsWorldPacket(id)) {
        auto header = ParseHeader(data, info.type);
        info.sync_count = header.world.sync;
    }
    
    // Store chunk data if provided
    if (chunk_data && chunk_size > 0) {
        info.chunk_data.assign(chunk_data, chunk_data + chunk_size);
    }
    
    // Generate session ID (simplified - in practice you'd want a more robust system)
    info.session_id = "session_" + std::to_string(info.timestamp);
    
    return info;
}

} // namespace kpacket