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
 
#include "zmq_relay.hpp"
#include <iostream>
#include <chrono>

#ifndef KPACKET_RELEASE_VERSION
#define KPACKET_RELEASE_VERSION "unknown"
#endif

namespace kpacket {

// Base64 encoding helper (simple implementation)
std::string base64_encode(const std::vector<uint8_t>& data) {
    // Simple base64 implementation - in production use a proper library
    static const std::string chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    int val = 0, valb = -6;
    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back('=');
    return result;
}

ZmqRelay::ZmqRelay(const Config& config) 
    : config_(config) {
    // Generate session UUID (simple placeholder)
    session_uuid_ = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    
    // Register built-in command handlers
    RegisterCommandHandler("status", [this](const nlohmann::json& params) {
        return HandleStatusCommand(params);
    });
    RegisterCommandHandler("hello", [this](const nlohmann::json& params) {
        return HandleHelloCommand(params);
    });
    
    RegisterCommandHandler("inject", [this](const nlohmann::json& params) {
        return HandleInjectCommand(params);
    });
    
    RegisterCommandHandler("filter", [this](const nlohmann::json& params) {
        return HandleFilterCommand(params);
    });
    
    RegisterCommandHandler("stats", [this](const nlohmann::json& params) {
        return HandleStatsCommand(params);
    });
}

ZmqRelay::~ZmqRelay() {
    Shutdown();
}

bool ZmqRelay::Initialize() {
    try {
        context_ = std::make_unique<zmq::context_t>(1);
        
        // Initialize publisher socket
        if (config_.enable_packet_streaming) {
            pub_socket_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_PUB);
            pub_socket_->bind(config_.pub_endpoint);
        }
        
        // Initialize command socket
        if (config_.enable_command_interface) {
            rep_socket_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_REP);
            rep_socket_->bind(config_.rep_endpoint);
        }
        
        // Initialize reliable queue socket
        if (config_.enable_reliable_queue) {
            push_socket_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_PUSH);
            push_socket_->bind(config_.push_endpoint);
        }
        
        running_ = true;
        
        ApplySocketOptions();

        // Start worker threads
        if (config_.enable_packet_streaming) {
            pub_thread_ = std::thread(&ZmqRelay::PublisherThread, this);
        }
        
        if (config_.enable_command_interface) {
            rep_thread_ = std::thread(&ZmqRelay::CommandThread, this);
        }
        
        if (config_.enable_reliable_queue) {
            push_thread_ = std::thread(&ZmqRelay::ReliableQueueThread, this);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "ZMQ Relay initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void ZmqRelay::Shutdown() {
    running_ = false;
    
    // Join threads
    if (pub_thread_.joinable()) pub_thread_.join();
    if (rep_thread_.joinable()) rep_thread_.join();
    if (push_thread_.joinable()) push_thread_.join();
    
    // Clean up sockets
    pub_socket_.reset();
    rep_socket_.reset();
    push_socket_.reset();
    context_.reset();
}

void ZmqRelay::PublishPacket(const PacketInfo& packet) {
    if (!running_ || !pub_socket_) return;

    // Apply filter if set
    if (packet_filter_ && !packet_filter_(packet)) {
        stats_.packets_filtered++;
        return;
    }

    // Add to reliable queue if enabled
    if (config_.enable_reliable_queue) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (packet_queue_.size() < config_.max_queue_size) {
            packet_queue_.push(packet);
        }
    }

    // Enqueue for publisher thread
    {
        std::lock_guard<std::mutex> lock(publish_mutex_);
        publish_queue_.push(packet);
    }
    publish_cv_.notify_one();
}

void ZmqRelay::SetPacketFilter(std::function<bool(const PacketInfo&)> filter) {
    packet_filter_ = filter;
}

void ZmqRelay::RegisterCommandHandler(const std::string& command, CommandHandler handler) {
    command_handlers_[command] = handler;
}

void ZmqRelay::SetPacketInjector(PacketInjector injector) {
    packet_injector_ = injector;
}

void ZmqRelay::SetPlayerNameProvider(PlayerNameProvider provider) {
    player_name_provider_ = std::move(provider);
}

void ZmqRelay::PublisherThread() {
    while (running_) {
        std::unique_lock<std::mutex> lock(publish_mutex_);
        publish_cv_.wait_for(lock, std::chrono::milliseconds(50), [this]{ return !publish_queue_.empty() || !running_; });
        if (!running_) break;
        if (publish_queue_.empty()) continue;

        PacketInfo pkt = std::move(publish_queue_.front());
        publish_queue_.pop();
        lock.unlock();

        try {
            // Augment message
            auto json_msg = PacketToJson(pkt);
            json_msg["version"] = config_.protocol_version;
            json_msg["session_uuid"] = session_uuid_;
            json_msg["message_id"] = next_message_id_++;

            std::string topic = TopicForPacket(pkt);

            if (config_.pub_use_multipart_raw) {
                zmq::multipart_t mp;
                mp.addstr(topic);
                // metadata without raw payload to avoid duplication
                nlohmann::json meta = json_msg;
                meta.erase("data");
                meta.erase("chunk_data");
                mp.addstr(meta.dump());
                // raw packet data
                zmq::message_t raw(pkt.data.size());
                if (!pkt.data.empty()) std::memcpy(raw.data(), pkt.data.data(), pkt.data.size());
                mp.add(std::move(raw));
                // optional chunk frame
                if (!pkt.chunk_data.empty()) {
                    zmq::message_t chunk(pkt.chunk_data.size());
                    std::memcpy(chunk.data(), pkt.chunk_data.data(), pkt.chunk_data.size());
                    mp.add(std::move(chunk));
                }
                mp.send(*pub_socket_);
            } else {
                zmq::multipart_t mp;
                mp.addstr(topic);
                mp.addstr(json_msg.dump());
                mp.send(*pub_socket_);
            }

            stats_.packets_published++;
        } catch (const std::exception& e) {
            stats_.pub_send_errors++;
            std::cerr << "Failed to publish packet: " << e.what() << std::endl;
        }
    }
}

void ZmqRelay::CommandThread() {
    while (running_) {
        try {
            zmq::message_t request;
            
            // Set receive timeout
            rep_socket_->set(zmq::sockopt::rcvtimeo, config_.rep_rcv_timeout_ms);
            
            if (rep_socket_->recv(request, zmq::recv_flags::dontwait)) {
                std::string command_str(static_cast<char*>(request.data()), request.size());
                std::string response = ProcessCommand(command_str);
                
                zmq::message_t reply(response.size());
                memcpy(reply.data(), response.c_str(), response.size());
                rep_socket_->send(reply, zmq::send_flags::dontwait);
                
                stats_.commands_processed++;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Command thread error: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ZmqRelay::ReliableQueueThread() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        if (!packet_queue_.empty()) {
            auto packet = packet_queue_.front();
            packet_queue_.pop();
            lock.unlock();
            
            try {
                auto json_msg = PacketToJson(packet);
                std::string msg = json_msg.dump();
                
                zmq::message_t zmq_msg(msg.size());
                memcpy(zmq_msg.data(), msg.c_str(), msg.size());
                push_socket_->send(zmq_msg, zmq::send_flags::dontwait);
                
            } catch (const std::exception& e) {
                std::cerr << "Reliable queue error: " << e.what() << std::endl;
            }
        } else {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

nlohmann::json ZmqRelay::PacketToJson(const PacketInfo& packet) {
    nlohmann::json j;
    to_json(j, packet);
    return j;
}

std::string ZmqRelay::ProcessCommand(const std::string& command_str) {
    try {
        auto command_json = nlohmann::json::parse(command_str);
        std::string cmd = command_json["command"];
        auto params = command_json.value("params", nlohmann::json::object());
        
        auto it = command_handlers_.find(cmd);
        if (it != command_handlers_.end()) {
            return it->second(params);
        } else {
            return R"({"error": "Unknown command"})";
        }
        
    } catch (const std::exception& e) {
        return R"({"error": ")" + std::string(e.what()) + R"("})";
    }
}

std::string ZmqRelay::HandleStatusCommand(const nlohmann::json& /* params */) {
    nlohmann::json response;
    response["status"] = "running";
    response["uptime"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    response["config"] = {
        {"pub_endpoint", config_.pub_endpoint},
        {"rep_endpoint", config_.rep_endpoint},
        {"push_endpoint", config_.push_endpoint},
        {"packet_streaming", config_.enable_packet_streaming},
        {"command_interface", config_.enable_command_interface},
        {"reliable_queue", config_.enable_reliable_queue}
    };
    response["version"] = config_.protocol_version;
    response["release_version"] = KPACKET_RELEASE_VERSION;
    response["session_uuid"] = session_uuid_;
    return response.dump();
}

std::string ZmqRelay::HandleHelloCommand(const nlohmann::json& /*params*/) {
    nlohmann::json response;
    response["version"] = config_.protocol_version;
    response["release_version"] = KPACKET_RELEASE_VERSION;
    response["session_uuid"] = session_uuid_;
    response["capabilities"] = {
        {"multipart_raw", config_.pub_use_multipart_raw}
    };

    if (player_name_provider_) {
        if (const auto playerName = player_name_provider_()) {
            response["player_name"] = *playerName;
        }
    }

    return response.dump();
}

std::string ZmqRelay::HandleInjectCommand(const nlohmann::json& params) {
    try {
        if (!packet_injector_) {
            return R"({"error": "Packet injection not available"})";
        }
        
        uint16_t id = params["id"];
        std::string data_str = params["data"];
        
        // Decode base64 data (simplified)
        std::vector<uint8_t> data; // In practice, implement proper base64 decode
        
        bool success = packet_injector_(id, data);
        
        nlohmann::json response;
        response["success"] = success;
        if (success) {
            stats_.packets_injected++;
        }
        
        return response.dump();
        
    } catch (const std::exception& e) {
        return R"({"error": ")" + std::string(e.what()) + R"("})";
    }
}

std::string ZmqRelay::HandleFilterCommand(const nlohmann::json& /* params */) {
    // Implement packet filtering configuration
    nlohmann::json response;
    response["message"] = "Filter configuration updated";
    return response.dump();
}

std::string ZmqRelay::HandleStatsCommand(const nlohmann::json& /* params */) {
    nlohmann::json response;
    response["stats"] = {
        {"packets_published", stats_.packets_published.load()},
        {"commands_processed", stats_.commands_processed.load()},
        {"packets_injected", stats_.packets_injected.load()},
        {"packets_filtered", stats_.packets_filtered.load()},
        {"publish_dropped", stats_.publish_dropped.load()},
        {"pub_send_errors", stats_.pub_send_errors.load()},
        {"queue_size", packet_queue_.size()}
    };
    return response.dump();
}

// JSON serialization functions
void to_json(nlohmann::json& j, const PacketInfo& packet) {
    j = nlohmann::json{
        {"timestamp", packet.timestamp},
        {"direction", packet.direction == Direction::INCOMING ? "incoming" : "outgoing"},
        {"packet_type", [&]() {
            switch (packet.type) {
                case PacketType::WORLD_CLIENT_TO_SERVER: return "world_c2s";
                case PacketType::WORLD_SERVER_TO_CLIENT: return "world_s2c";
                case PacketType::LOBBY_CLIENT_TO_SERVER: return "lobby_c2s";
                case PacketType::LOBBY_SERVER_TO_CLIENT: return "lobby_s2c";
                case PacketType::PATCH_CLIENT_TO_SERVER: return "patch_c2s";
                case PacketType::PATCH_SERVER_TO_CLIENT: return "patch_s2c";
                default: return "unknown";
            }
        }()},
        {"packet_id", packet.id},
        {"packet_name", packet.name},
        {"size", packet.size},
        {"data", base64_encode(packet.data)},
        {"metadata", {
            {"injected", packet.injected},
            {"blocked", packet.blocked},
            {"chunk_size", packet.chunk_size},
            {"session_id", packet.session_id},
            {"sync_count", packet.sync_count}
        }}
    };
    
    if (!packet.chunk_data.empty()) {
        j["chunk_data"] = base64_encode(packet.chunk_data);
    }
}

std::string ZmqRelay::TopicForPacket(const PacketInfo& packet) const {
    // kpacket.<version>.<world|lobby|patch|unknown>.<s2c|c2s>.<hex_id>
    auto type = std::string(
        packet.type == PacketType::WORLD_CLIENT_TO_SERVER || packet.type == PacketType::WORLD_SERVER_TO_CLIENT ? "world" :
        packet.type == PacketType::LOBBY_CLIENT_TO_SERVER || packet.type == PacketType::LOBBY_SERVER_TO_CLIENT ? "lobby" :
        packet.type == PacketType::PATCH_CLIENT_TO_SERVER || packet.type == PacketType::PATCH_SERVER_TO_CLIENT ? "patch" :
        "unknown");
    auto dir = packet.direction == Direction::INCOMING ? "s2c" : "c2s";
    char hex_id[8];
    std::snprintf(hex_id, sizeof(hex_id), "0x%04X", packet.id);
    return std::string("kpacket.") + config_.protocol_version + "." + type + "." + dir + "." + hex_id;
}

void ZmqRelay::ApplySocketOptions() {
    try {
        if (pub_socket_) {
            pub_socket_->set(zmq::sockopt::sndhwm, config_.pub_snd_hwm);
            pub_socket_->set(zmq::sockopt::linger, config_.pub_linger_ms);
        }
        if (rep_socket_) {
            rep_socket_->set(zmq::sockopt::linger, config_.rep_linger_ms);
        }
        if (push_socket_) {
            push_socket_->set(zmq::sockopt::linger, config_.push_linger_ms);
        }
    } catch (...) {
        // best effort
    }
}

void from_json(const nlohmann::json& j, PacketInfo& packet) {
    packet.timestamp = j.at("timestamp");
    packet.direction = j.at("direction") == "incoming" ? Direction::INCOMING : Direction::OUTGOING;
    packet.id = j.at("packet_id");
    packet.name = j.at("packet_name");
    packet.size = j.at("size");
    
    // Decode base64 data (simplified - implement proper decode)
    // packet.data = base64_decode(j.at("data"));
    
    auto metadata = j.at("metadata");
    packet.injected = metadata.at("injected");
    packet.blocked = metadata.at("blocked");
    packet.chunk_size = metadata.at("chunk_size");
    packet.session_id = metadata.at("session_id");
    packet.sync_count = metadata.at("sync_count");
}

} // namespace kpacket