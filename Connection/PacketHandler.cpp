#include "PacketHandler.hpp"
#pragma warning(push, 0)
#include <uwebsockets/App.h>
#pragma warning(pop)
#include "../Game/Map.hpp"
#include "../Game/Game.hpp"
#include "../Player/Player.hpp"
#include "../Modules/Utils.hpp"
#include "../Modules/Logger.hpp"
#include "../Protocol/Protocol.hpp"
#include "../Protocol/Protocol_4.hpp"
#include "../Protocol/Protocol_5.hpp"
#include "../Protocol/Protocol_6.hpp"
#include "../Protocol/Protocol_7.hpp"
#include "../Protocol/Protocol_8.hpp"
#include "../Protocol/Protocol_9.hpp"
#include "../Protocol/Protocol_10.hpp"
#include "../Protocol/Protocol_11.hpp"
#include "../Protocol/Protocol_12.hpp"
#include "../Protocol/Protocol_13.hpp"
#include "../Protocol/Protocol_14.hpp"
#include "../Protocol/Protocol_15.hpp"
#include "../Protocol/Protocol_16.hpp"
#include "../Protocol/Protocol_17.hpp"
#include "../Protocol/Protocol_18.hpp"

PacketHandler::PacketHandler(Player *owner) :
    player(owner) {
}

void PacketHandler::sendPacket(Buffer &buffer) const {
    const std::vector<unsigned char> &buf = buffer.getBuffer();
    if (buf.empty() || !player->socket)
        return;
    player->socket->send(std::string_view((char*)&buf[0], buf.size()), uWS::BINARY);
    buffer.clear();
}

void PacketHandler::onPacket(std::vector<unsigned char> &packet) {
    Buffer buffer(packet);
    
    // Process OpCode
    switch ((OpCode)buffer.readUInt8()) {
        case OpCode::SPAWN:
            if (player->state() != PlayerState::PLAYING) {
                if (player->protocolNum < 6)
                    player->setFullName(buffer.readStrNull_UCS2(), true);
                else
                    player->setFullName(buffer.readStrNull_UTF8());
                player->onSpawn();
            }
            break;
        case OpCode::SPECTATE:
            onSpectate();
            break;
        case OpCode::FACEBOOK_DATA:
            break;
        case OpCode::SET_TARGET:
            onTarget({ (double)buffer.readInt32_LE(), (double)buffer.readInt32_LE() });
            break;
        case OpCode::SPLIT:
            onSplit();
            break;
        case OpCode::QKEY_PRESSED:
            onQKey();
            break;
        case OpCode::QKEY_RELEASED:
            break;
        case OpCode::EJECT:
            onEject();
            break;
        case OpCode::CAPTCHA_RESPONSE:
            Logger::info("Captcha Response packet received.");
            break;
        case OpCode::MOBILE_DATA:
            break;
        case OpCode::PONG:
            Logger::info("Pong packet received.");
            break;
        case OpCode::ESTABLISH_CONNECTION:
            onEstablishedConnection(buffer.readUInt32_LE());
            break;
        case OpCode::CONNECTION_KEY:
            onConnectionKey();
            break;
    }
}

void PacketHandler::onSpectate() const noexcept {
    Logger::info("Spectate packet received.");
    player->onSpectate();
}
void PacketHandler::onTarget(const Vec2 &mouse) const noexcept {
    player->onTarget(mouse);
}
void PacketHandler::onSplit() const noexcept {
    player->onSplit();
}
void PacketHandler::onQKey() const noexcept {
    player->onQKey();
}
void PacketHandler::onEject() const noexcept {
    player->onEject();
}
void PacketHandler::onEstablishedConnection(unsigned protocol) const noexcept {
    Logger::info("Establish Connection packet received.");
    Logger::info("Protocol version: " + std::to_string(protocol));
    if (protocol < cfg::server_minSupportedProtocol ||
        protocol > cfg::server_maxSupportedProtocol) {
        player->socket->end(1002, "Unsupported protocol");
        return;
    }
    switch (protocol) {
        case 4:  { player->protocol = new Protocol_4(player);  break; }
        case 5:  { player->protocol = new Protocol_5(player);  break; }
        case 6:  { player->protocol = new Protocol_6(player);  break; }
        case 7:  { player->protocol = new Protocol_7(player);  break; }
        case 8:  { player->protocol = new Protocol_8(player);  break; }
        case 9:  { player->protocol = new Protocol_9(player);  break; }
        case 10: { player->protocol = new Protocol_10(player); break; }
        case 11: { player->protocol = new Protocol_11(player); break; }
        case 12: { player->protocol = new Protocol_12(player); break; }
        case 13: { player->protocol = new Protocol_13(player); break; }
        case 14: { player->protocol = new Protocol_14(player); break; }
        case 15: { player->protocol = new Protocol_15(player); break; }
        case 16: { player->protocol = new Protocol_16(player); break; }
        case 17: { player->protocol = new Protocol_17(player); break; }
        case 18: { player->protocol = new Protocol_18(player); break; }
        default: { 
            player->protocol = new Protocol_4(player); 
            Logger::warn("Protocol assumed as 4.");
        }
    }
    player->protocolNum = protocol;
    player->server->clients.push_back(player);
}
void PacketHandler::onConnectionKey() const noexcept {
    Logger::info("Connection Key packet received.");
    sendPacket(player->protocol->clearAll());
    sendPacket(player->protocol->setBorder());
}

PacketHandler::~PacketHandler() {
}