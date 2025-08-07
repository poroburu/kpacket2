/**
 * Simple C++ client to validate the ZMQ packet relay system
 * 
 * This client demonstrates:
 * 1. Subscribing to packet streams
 * 2. Sending commands to the plugin
 * 3. Filtering and processing packets
 */

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>

class PacketMonitor {
public:
    PacketMonitor(const std::string& sub_endpoint = "tcp://localhost:5555",
                  const std::string& req_endpoint = "tcp://localhost:5556")
        : sub_endpoint_(sub_endpoint), req_endpoint_(req_endpoint), running_(false) {}

    bool Initialize() {
        try {
            context_ = std::make_unique<zmq::context_t>(1);
            
            // Initialize subscriber for packet streaming
            sub_socket_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_SUB);
            sub_socket_->connect(sub_endpoint_);
            
            // Subscribe to all packets (empty filter)
            sub_socket_->setsockopt(ZMQ_SUBSCRIBE, "", 0);
            
            // Initialize request socket for commands
            req_socket_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_REQ);
            req_socket_->connect(req_endpoint_);
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Initialization error: " << e.what() << std::endl;
            return false;
        }
    }

    void Start() {
        if (running_) return;
        
        running_ = true;
        monitor_thread_ = std::thread(&PacketMonitor::MonitorPackets, this);
        
        std::cout << "Packet monitor started. Commands:" << std::endl;
        std::cout << "  status - Get plugin status" << std::endl;
        std::cout << "  stats  - Get statistics" << std::endl;
        std::cout << "  filter <packet_id> - Filter specific packet" << std::endl;
        std::cout << "  quit   - Exit monitor" << std::endl;
        std::cout << std::endl;
        
        // Command loop
        std::string input;
        while (running_ && std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                break;
            } else if (input == "status") {
                SendCommand("status", {});
            } else if (input == "stats") {
                SendCommand("stats", {});
            } else if (input.substr(0, 6) == "filter") {
                if (input.length() > 7) {
                    std::string packet_id = input.substr(7);
                    nlohmann::json params;
                    params["packet_id"] = packet_id;
                    SendCommand("filter", params);
                }
            } else if (!input.empty()) {
                std::cout << "Unknown command: " << input << std::endl;
            }
        }
        
        Stop();
    }

    void Stop() {
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }

private:
    std::string sub_endpoint_;
    std::string req_endpoint_;
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> sub_socket_;
    std::unique_ptr<zmq::socket_t> req_socket_;
    std::thread monitor_thread_;
    std::atomic<bool> running_;
    
    // Statistics
    std::atomic<uint64_t> packets_received_{0};
    std::chrono::steady_clock::time_point start_time_;

    void MonitorPackets() {
        start_time_ = std::chrono::steady_clock::now();
        
        while (running_) {
            try {
                zmq::multipart_t multipart;
                
                // Set receive timeout
                sub_socket_->set(zmq::sockopt::rcvtimeo, 100);
                
                if (multipart.recv(*sub_socket_, static_cast<int>(zmq::recv_flags::dontwait))) {
                    if (multipart.size() >= 2) {
                        std::string topic = multipart[0].to_string();
                        std::string json_data = multipart[1].to_string();
                        
                        ProcessPacket(topic, json_data);
                        packets_received_++;
                    }
                }
                
            } catch (const std::exception& e) {
                if (running_) {
                    std::cerr << "Monitor error: " << e.what() << std::endl;
                }
            }
        }
    }

    void ProcessPacket(const std::string& topic, const std::string& json_data) {
        try {
            auto packet = nlohmann::json::parse(json_data);
            
            // Extract key information
            uint64_t timestamp = packet["timestamp"];
            std::string direction = packet["direction"];
            std::string packet_type = packet["packet_type"];
            uint16_t packet_id = packet["packet_id"];
            std::string packet_name = packet["packet_name"];
            uint32_t size = packet["size"];
            
            // Format timestamp
            auto time_point = std::chrono::system_clock::time_point(
                std::chrono::milliseconds(timestamp));
            auto time_t = std::chrono::system_clock::to_time_t(time_point);
            
            // Print packet information
            std::cout << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
                      << " [" << direction << "] "
                      << "0x" << std::hex << std::setw(4) << std::setfill('0') << packet_id 
                      << std::dec << " (" << packet_name << ") "
                      << "Size: " << size << " bytes"
                      << std::endl;
            
            // Show metadata if interesting
            auto metadata = packet["metadata"];
            bool injected = metadata["injected"];
            bool blocked = metadata["blocked"];
            
            if (injected || blocked) {
                std::cout << "  -> Flags: ";
                if (injected) std::cout << "INJECTED ";
                if (blocked) std::cout << "BLOCKED ";
                std::cout << std::endl;
            }
            
            // Show periodic statistics
            static auto last_stats = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (now - last_stats > std::chrono::seconds(10)) {
                ShowStatistics();
                last_stats = now;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Packet processing error: " << e.what() << std::endl;
        }
    }

    void SendCommand(const std::string& command, const nlohmann::json& params) {
        try {
            nlohmann::json cmd_json;
            cmd_json["command"] = command;
            cmd_json["params"] = params;
            
            std::string cmd_str = cmd_json.dump();
            
            zmq::message_t request(cmd_str.size());
            memcpy(request.data(), cmd_str.c_str(), cmd_str.size());
            
            // Set send/receive timeout
            req_socket_->set(zmq::sockopt::sndtimeo, 5000);
            req_socket_->set(zmq::sockopt::rcvtimeo, 5000);
            
            req_socket_->send(request, zmq::send_flags::none);
            
            zmq::message_t reply;
            if (req_socket_->recv(reply, zmq::recv_flags::none)) {
                std::string response(static_cast<char*>(reply.data()), reply.size());
                
                try {
                    auto response_json = nlohmann::json::parse(response);
                    std::cout << "Response: " << response_json.dump(2) << std::endl;
                } catch (...) {
                    std::cout << "Response: " << response << std::endl;
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Command error: " << e.what() << std::endl;
        }
    }

    void ShowStatistics() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        
        double rate = duration.count() > 0 ? 
            static_cast<double>(packets_received_) / duration.count() : 0.0;
        
        std::cout << "--- Statistics ---" << std::endl;
        std::cout << "Runtime: " << duration.count() << "s" << std::endl;
        std::cout << "Packets received: " << packets_received_ << std::endl;
        std::cout << "Average rate: " << std::fixed << std::setprecision(2) 
                  << rate << " packets/sec" << std::endl;
        std::cout << "------------------" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "FFXI Packet Monitor - ZMQ Validation Client" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    std::string sub_endpoint = "tcp://localhost:5555";
    std::string req_endpoint = "tcp://localhost:5556";
    
    // Parse command line arguments
    if (argc >= 2) {
        sub_endpoint = argv[1];
    }
    if (argc >= 3) {
        req_endpoint = argv[2];
    }
    
    std::cout << "Connecting to:" << std::endl;
    std::cout << "  Packet stream: " << sub_endpoint << std::endl;
    std::cout << "  Command interface: " << req_endpoint << std::endl;
    std::cout << std::endl;
    
    PacketMonitor monitor(sub_endpoint, req_endpoint);
    
    if (!monitor.Initialize()) {
        std::cerr << "Failed to initialize monitor" << std::endl;
        return 1;
    }
    
    std::cout << "Monitor initialized successfully" << std::endl;
    monitor.Start();
    
    std::cout << "Monitor stopped" << std::endl;
    return 0;
}