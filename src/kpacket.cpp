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
#include "kpacket.hpp"
#include <optional>

namespace kpacket
{
    /**
     * Constructor and Deconstructor
     */
    plugin::plugin(void)
        : core_{nullptr}
        , log_{nullptr}
        , device_{nullptr}
        , id_{0}
    {}
    plugin::~plugin(void)
    {}

    auto plugin::GetName(void) const -> const char*
    {
        return "kpacket";
    }

    auto plugin::GetAuthor(void) const -> const char*
    {
        return "Ashita Development Team";
    }

    auto plugin::GetDescription(void) const -> const char*
    {
        return "FFXI packet relay plugin for Ashita v4 using ZeroMQ.";
    }

    auto plugin::GetLink(void) const -> const char*
    {
        return "https://www.ashitaxi.com/";
    }

    auto plugin::GetVersion(void) const -> double
    {
        return 1.0f;
    }

    auto plugin::GetInterfaceVersion(void) const -> double
    {
        return ASHITA_INTERFACE_VERSION;
    }

    auto plugin::GetPriority(void) const -> int32_t
    {
        return 0;
    }

    auto plugin::GetFlags(void) const -> uint32_t
    {
        return (uint32_t)Ashita::PluginFlags::All;
    }

    auto plugin::Initialize(IAshitaCore* core, ILogManager* logger, const uint32_t id) -> bool
    {
        this->core_ = core;
        this->log_  = logger;
        this->id_   = id;

        try {
            kpacket::ZmqRelay::Config config;
            config.pub_endpoint = "tcp://*:5555";
            config.rep_endpoint = "tcp://*:5556";  
            config.push_endpoint = "tcp://*:5557";
            
            zmq_relay_ = std::make_unique<kpacket::ZmqRelay>(config);
            
            zmq_relay_->SetPacketInjector([this](uint16_t pid, const std::vector<uint8_t>& pdata) -> bool {
                this->log_->Logf(0, "kpacket", 
                    "Packet injection requested: ID=0x%04X, Size=%zu", pid, pdata.size());
                return false;
            });

            zmq_relay_->SetPlayerNameProvider([this]() -> std::optional<std::string> {
                if (!this->core_) {
                    return std::nullopt;
                }

                const auto mm = this->core_->GetMemoryManager();
                if (!mm) {
                    return std::nullopt;
                }

                const auto party = mm->GetParty();
                if (!party) {
                    return std::nullopt;
                }

                const char* name = party->GetMemberName(0);
                if (name && name[0] != '\0') {
                    return std::string(name);
                }

                return std::nullopt;
            });
            
            if (!zmq_relay_->Initialize()) {
                this->log_->Logf(1, "kpacket", "Failed to initialize ZMQ relay system");
                return false;
            }
            
            this->log_->Logf(0, "kpacket", "ZMQ relay system initialized successfully");
            
        } catch (const std::exception& e) {
            this->log_->Logf(1, "kpacket", 
                "ZMQ relay initialization error: %s", e.what());
            return false;
        }

        return true;
    }

    auto plugin::Release(void) -> void
    {
        if (zmq_relay_) {
            zmq_relay_->Shutdown();
            zmq_relay_.reset();
            this->log_->Logf(0, "kpacket", "ZMQ relay system shutdown");
        }
    }

    auto plugin::HandleEvent(const char* event_name, const void* event_data, const uint32_t event_size) -> void
    {
        UNREFERENCED_PARAMETER(event_name);
        UNREFERENCED_PARAMETER(event_data);
        UNREFERENCED_PARAMETER(event_size);
    }

    auto plugin::HandleCommand(int32_t mode, const char* command, bool injected) -> bool
    {
        UNREFERENCED_PARAMETER(mode);
        UNREFERENCED_PARAMETER(injected);

        std::vector<std::string> args{};
        const auto count = Ashita::Commands::GetCommandArgs(command, &args);

        HANDLECOMMAND("/example")
        {
            this->core_->GetChatManager()->Write(1, false, "[kpacket] Command executed: /example");
            return true;
        }

        HANDLECOMMAND("/example2")
        {
            if (count >= 2 && args[1] == "test")
            {
                this->core_->GetChatManager()->Write(1, false, "[kpacket] Command executed: /example2 test");
                return true;
            }
            return true;
        }

        HANDLECOMMAND("/example3")
        {
            if (mode == static_cast<int32_t>(Ashita::CommandMode::Macro))
            {
                this->core_->GetChatManager()->Write(1, false, "[kpacket] Command executed: /example3");
                return true;
            }
            return true;
        }

        return false;
    }

    auto plugin::HandleIncomingText(int32_t mode, bool indent, const char* message, int32_t* modified_mode, bool* modified_indent, char* modified_message, bool injected, bool blocked) -> bool
    {
        UNREFERENCED_PARAMETER(mode);
        UNREFERENCED_PARAMETER(indent);
        UNREFERENCED_PARAMETER(message);
        UNREFERENCED_PARAMETER(modified_mode);
        UNREFERENCED_PARAMETER(blocked);

        if (modified_message == nullptr || injected)
            return false;

        const auto msg = std::string(modified_message);
        if (msg.find("indent") != std::string::npos)
            *modified_indent = true;
        if (msg.find("block") != std::string::npos)
            return true;
        if (msg.find("derp") != std::string::npos)
        {
            std::string str = "[Derp] ";
            str += modified_message;
            ::strcpy_s(modified_message, 4096, str.c_str());
        }

        const auto mex = *modified_mode & 0xFFFFFF00;
        const auto mid = *modified_mode & 0x000000FF;
        if (mid == 206)
            *modified_mode = mex | 5;

        return false;
    }

    auto plugin::HandleOutgoingText(int32_t mode, const char* message, int32_t* modified_mode, char* modified_message, bool injected, bool blocked) -> bool
    {
        UNREFERENCED_PARAMETER(mode);
        UNREFERENCED_PARAMETER(message);
        UNREFERENCED_PARAMETER(modified_mode);
        UNREFERENCED_PARAMETER(blocked);
        if (!injected && ::_strnicmp(modified_message, "/wave", 5) == 0)
            return true;
        return false;
    }

    auto plugin::HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t size_chunk, const uint8_t* data_chunk, bool injected, bool blocked) -> bool
    {
        if (zmq_relay_ && zmq_relay_->IsRunning()) {
            try {
                auto packet_info = kpacket::PacketParser::CreatePacketInfo(
                    id, size, data, kpacket::Direction::INCOMING, 
                    injected, blocked, size_chunk, data_chunk
                );
                zmq_relay_->PublishPacket(packet_info);
            } catch (const std::exception& e) {
                this->log_->Logf(1, "kpacket", 
                    "Failed to relay incoming packet 0x%04X: %s", id, e.what());
            }
        }

        if (id == 0x005A)
        {
            if (modified[0x10] == 0x1D)
                modified[0x10] = 0x08;
            if (modified[0x10] == 0x0C)
                return true;
        }

        return false;
    }

    auto plugin::HandleOutgoingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t size_chunk, const uint8_t* data_chunk, bool injected, bool blocked) -> bool
    {
        if (zmq_relay_ && zmq_relay_->IsRunning()) {
            try {
                auto packet_info = kpacket::PacketParser::CreatePacketInfo(
                    id, size, data, kpacket::Direction::OUTGOING, 
                    injected, blocked, size_chunk, data_chunk
                );
                zmq_relay_->PublishPacket(packet_info);
            } catch (const std::exception& e) {
                this->log_->Logf(1, "kpacket", 
                    "Failed to relay outgoing packet 0x%04X: %s", id, e.what());
            }
        }

        if (id == 0x005D)
        {
            if (modified[0x0A] == 0x18)
                modified[0x0A] = 0x08;
        }

        return false;
    }

    auto plugin::Direct3DInitialize(IDirect3DDevice8* device) -> bool
    {
        this->device_ = device;
        return true;
    }

    auto plugin::Direct3DBeginScene(bool is_rendering_backbuffer) -> void
    {
        UNREFERENCED_PARAMETER(is_rendering_backbuffer);
    }

    auto plugin::Direct3DEndScene(bool is_rendering_backbuffer) -> void
    {
        UNREFERENCED_PARAMETER(is_rendering_backbuffer);
    }

    auto plugin::Direct3DPresent(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) -> void
    {
        UNREFERENCED_PARAMETER(pSourceRect);
        UNREFERENCED_PARAMETER(pDestRect);
        UNREFERENCED_PARAMETER(hDestWindowOverride);
        UNREFERENCED_PARAMETER(pDirtyRegion);
    }

    auto plugin::Direct3DSetRenderState(D3DRENDERSTATETYPE State, DWORD* Value) -> bool
    {
        UNREFERENCED_PARAMETER(State);
        UNREFERENCED_PARAMETER(Value);
        return false;
    }

    auto plugin::Direct3DDrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) -> bool
    {
        UNREFERENCED_PARAMETER(PrimitiveType);
        UNREFERENCED_PARAMETER(StartVertex);
        UNREFERENCED_PARAMETER(PrimitiveCount);
        return false;
    }

    auto plugin::Direct3DDrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount) -> bool
    {
        UNREFERENCED_PARAMETER(PrimitiveType);
        UNREFERENCED_PARAMETER(minIndex);
        UNREFERENCED_PARAMETER(NumVertices);
        UNREFERENCED_PARAMETER(startIndex);
        UNREFERENCED_PARAMETER(primCount);
        return false;
    }

    auto plugin::Direct3DDrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) -> bool
    {
        UNREFERENCED_PARAMETER(PrimitiveType);
        UNREFERENCED_PARAMETER(PrimitiveCount);
        UNREFERENCED_PARAMETER(pVertexStreamZeroData);
        UNREFERENCED_PARAMETER(VertexStreamZeroStride);
        return false;
    }

    auto plugin::Direct3DDrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) -> bool
    {
        UNREFERENCED_PARAMETER(PrimitiveType);
        UNREFERENCED_PARAMETER(MinVertexIndex);
        UNREFERENCED_PARAMETER(NumVertexIndices);
        UNREFERENCED_PARAMETER(PrimitiveCount);
        UNREFERENCED_PARAMETER(pIndexData);
        UNREFERENCED_PARAMETER(IndexDataFormat);
        UNREFERENCED_PARAMETER(pVertexStreamZeroData);
        UNREFERENCED_PARAMETER(VertexStreamZeroStride);
        return false;
    }

} // namespace kpacket

__declspec(dllexport) auto __stdcall expCreatePlugin(const char* args) -> IPlugin*
{
    UNREFERENCED_PARAMETER(args);
    return new kpacket::plugin();
}

__declspec(dllexport) auto __stdcall expGetInterfaceVersion(void) -> double
{
    return ASHITA_INTERFACE_VERSION;
}


