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

#ifndef KPACKET_HPP_INCLUDED
#define KPACKET_HPP_INCLUDED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/**
 * Main Ashita SDK Include
 */
#include "Ashita.h"
#include "zmq_relay.hpp"
#include "packet_types.hpp"
#include <memory>

namespace kpacket
{
    class plugin final : public IPlugin
    {
        IAshitaCore* core_;
        ILogManager* log_;
        IDirect3DDevice8* device_;
        uint32_t id_;
        
        // ZMQ Relay System
        std::unique_ptr<kpacket::ZmqRelay> zmq_relay_;

    public:
        plugin(void);
        ~plugin(void) override;

        // Properties (Plugin Information)
        auto GetName(void) const -> const char* override;
        auto GetAuthor(void) const -> const char* override;
        auto GetDescription(void) const -> const char* override;
        auto GetLink(void) const -> const char* override;
        auto GetVersion(void) const -> double override;
        auto GetInterfaceVersion(void) const -> double override;
        auto GetPriority(void) const -> int32_t override;
        auto GetFlags(void) const -> uint32_t override;

        // Methods
        auto Initialize(IAshitaCore* core, ILogManager* logger, uint32_t id) -> bool override;
        auto Release(void) -> void override;

        // Event Callbacks: PluginManager
        auto HandleEvent(const char* eventName, const void* eventData, const uint32_t eventSize) -> void override;

        // Event Callbacks: ChatManager
        auto HandleCommand(int32_t mode, const char* command, bool injected) -> bool override;
        auto HandleIncomingText(int32_t mode, bool indent, const char* message, int32_t* modifiedMode, bool* modifiedIndent, char* modifiedMessage, bool injected, bool blocked) -> bool override;
        auto HandleOutgoingText(int32_t mode, const char* message, int32_t* modifiedMode, char* modifiedMessage, bool injected, bool blocked) -> bool override;

        // Event Callbacks: PacketManager
        auto HandleIncomingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked) -> bool override;
        auto HandleOutgoingPacket(uint16_t id, uint32_t size, const uint8_t* data, uint8_t* modified, uint32_t sizeChunk, const uint8_t* dataChunk, bool injected, bool blocked) -> bool override;

        // Event Callbacks: Direct3D
        auto Direct3DInitialize(IDirect3DDevice8* device) -> bool override;
        auto Direct3DBeginScene(bool isRenderingBackBuffer) -> void override;
        auto Direct3DEndScene(bool isRenderingBackBuffer) -> void override;
        auto Direct3DPresent(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) -> void override;
        auto Direct3DSetRenderState(D3DRENDERSTATETYPE State, DWORD* Value) -> bool override;
        auto Direct3DDrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) -> bool override;
        auto Direct3DDrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount) -> bool override;
        auto Direct3DDrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) -> bool override;
        auto Direct3DDrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) -> bool override;
    };

} // namespace kpacket

#endif // KPACKET_HPP_INCLUDED


