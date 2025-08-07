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

#include "packet_types.hpp"
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <memory>
#include <functional>

namespace kpacket {

class ZmqRelay {
public:
    struct Config {
        std::string pub_endpoint = "tcp://*:5555";      // Packet streaming
        std::string rep_endpoint = "tcp://*:5556";      // Command interface
        std::string push_endpoint = "tcp://*:5557";     // Reliable queuing
        bool enable_packet_streaming = true;
        bool enable_command_interface = true;
        bool enable_reliable_queue = true;
        size_t max_queue_size = 10000;
    };

    ZmqRelay(const Config& config = Config{});
    ~ZmqRelay();

    // Core functionality
    bool Initialize();
    void Shutdown();
    bool IsRunning() const { return running_; }

    // Packet streaming
    void PublishPacket(const PacketInfo& packet);
    void SetPacketFilter(std::function<bool(const PacketInfo&)> filter);

    // Command interface
    using CommandHandler = std::function<std::string(const nlohmann::json&)>;
    void RegisterCommandHandler(const std::string& command, CommandHandler handler);

    // Packet injection
    using PacketInjector = std::function<bool(uint16_t id, const std::vector<uint8_t>& data)>;
    void SetPacketInjector(PacketInjector injector);

private:
    Config config_;
    std::unique_ptr<zmq::context_t> context_;
    
    // Sockets
    std::unique_ptr<zmq::socket_t> pub_socket_;
    std::unique_ptr<zmq::socket_t> rep_socket_;
    std::unique_ptr<zmq::socket_t> push_socket_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread pub_thread_;
    std::thread rep_thread_;
    std::thread push_thread_;
    
    // Packet queue for reliable delivery
    std::queue<PacketInfo> packet_queue_;
    std::mutex queue_mutex_;
    
    // Filtering and handling
    std::function<bool(const PacketInfo&)> packet_filter_;
    std::map<std::string, CommandHandler> command_handlers_;
    PacketInjector packet_injector_;
    
    // Thread functions
    void PublisherThread();
    void CommandThread();
    void ReliableQueueThread();
    
    // Utility functions
    nlohmann::json PacketToJson(const PacketInfo& packet);
    std::string ProcessCommand(const std::string& command_str);
    
    // Built-in command handlers
    std::string HandleStatusCommand(const nlohmann::json& params);
    std::string HandleInjectCommand(const nlohmann::json& params);
    std::string HandleFilterCommand(const nlohmann::json& params);
    std::string HandleStatsCommand(const nlohmann::json& params);
    
    // Statistics
    struct Stats {
        std::atomic<uint64_t> packets_published{0};
        std::atomic<uint64_t> commands_processed{0};
        std::atomic<uint64_t> packets_injected{0};
        std::atomic<uint64_t> packets_filtered{0};
    } stats_;
};

// Utility functions for JSON serialization
void to_json(nlohmann::json& j, const PacketInfo& packet);
void from_json(const nlohmann::json& j, PacketInfo& packet);

} // namespace kpacket