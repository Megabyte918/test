#include "Player.hpp"
#include "Entities/Map.hpp"
#include "Modules/Logger.hpp"
#include "Packets/Packet.hpp"
#include "Packets/AddNode.hpp"
#include "Packets/ClearAll.hpp"
#include "Packets/UpdateNodes.hpp"
#include "Packets/UpdateViewport.hpp"

Player::Player(uWS::WebSocket<uWS::SERVER> *_socket) :
    socket(_socket),
    packetHandler(PacketHandler(this)) {
}

const std::string &Player::getSkinName() const noexcept {
    return skinName;
}
const std::string &Player::getCellName() const noexcept {
    return cellName;
}
void Player::setSkinName(const std::string &name) noexcept {
    if (skinName != name)
        skinName = !name.size() ? "" : name;
}
void Player::setCellName(const std::string &name) noexcept {
    if (cellName == name)
        return;
    if (!name.size() || (name.size() == 1 && name[0] == 0))
        cellName = "An unnamed cell";
    else
        cellName = name;
}

double Player::getScore() const noexcept {
    return score;
}
Vector2 Player::getMouse() const noexcept {
    return mouse;
}
const PlayerState &Player::getState() const noexcept {
    return state;
}
const Vector2 &Player::getCenter() const noexcept {
    return center;
}

void Player::update() {
    if (state == PlayerState::PLAYING) {
        updateViewBox();
        updateVisibleNodes();
    }
}
void Player::updateScale() {
    double totalSize = 0;
    for (Entity *cell : cells)
        totalSize += cell->getSize();

    if (totalSize == 0) {
        score = 0;
        scale = config["player"]["minViewBoxScale"];
    } else {
        score = toMass(totalSize);
        scale = std::pow(std::min(64 / totalSize, 1.0), 0.4);
    }
}
void Player::updateCenter() {
    if (cells.empty()) return;

    Vector2 total;

    for (Entity *cell : cells)
        total += cell->getPosition();

    center = (center + (total / cells.size())) / 2;
}
void Player::updateViewBox() {
    Vector2 baseResolution(config["player"]["viewBoxWidth"], config["player"]["viewBoxHeight"]);

    filteredScale = (9 * filteredScale + std::max(scale, (double)config["player"]["minViewBoxScale"])) / 10;
    Vector2 viewPort = baseResolution / filteredScale;

    viewBox.update(center.x, center.y, viewPort.x, viewPort.y);
}
void Player::updateVisibleNodes() {
    std::vector<Entity*> newVisibleNodes, delNodes, eatNodes, addNodes, updNodes;

    for (Collidable *obj : Map::quadTree.getObjectsInBound(viewBox))
        newVisibleNodes.push_back(std::any_cast<Entity*>(obj->data));

    for (Entity *entity : newVisibleNodes) {
        if (std::find(visibleNodes.begin(), visibleNodes.end(), entity) == visibleNodes.end())
            addNodes.push_back(entity);
        else if (entity->needsUpdate)
            updNodes.push_back(entity);
    }
    for (Entity *entity : visibleNodes) {
        if (!entity->isRemoved) continue;
        if (entity->getKillerId() != 0)
            eatNodes.push_back(entity);
        delNodes.push_back(entity);
    }
    visibleNodes = newVisibleNodes;

    // Send packet
    if (eatNodes.size() + updNodes.size() + delNodes.size() + addNodes.size() > 0)
        packetHandler.sendPacket(UpdateNodes(this, eatNodes, updNodes, delNodes, addNodes));
}

void Player::onSpawn(std::string &name) noexcept {
    if (state == PlayerState::PLAYING) return;
    //cells.reserve(config::maxPlayerCells);

    // Remove whitespaces
    /*for (unsigned i = 0; i < name.size(); ++i)
        name.erase(name.begin() + i + 1);*/
    name.pop_back();

    // Get skin name
    std::string skin;
    size_t skinStart = name.find('<');
    size_t skinEnd = name.find('>');
    if (skinStart != skinEnd && skinStart != -1 && skinEnd != -1)
        skin = name.substr(skinStart + 1, skinEnd - 1);

    setSkinName(skin.substr(0, (unsigned)config["player"]["maxNameLength"]));
    setCellName(name.substr(skin.size() == 0 ? 0 : skinEnd + 1, (unsigned)config["player"]["maxNameLength"]));

    PlayerCell *firstCell = Map::spawnPlayerCell();
    cells.push_back(firstCell);
    firstCell->owner = this;

    state = PlayerState::PLAYING;
    mouse = firstCell->getPosition();
    updateScale(); // Initial update
    updateCenter(); // Initial update
    updateViewBox(); // Initial update
    updateVisibleNodes(); // Initial update
    packetHandler.sendPacket(ClearAll());
    packetHandler.sendPacket(AddNode(firstCell->getNodeId()));
}

void Player::onSpectate() noexcept {
    if (state != PlayerState::PLAYING)
        state = PlayerState::SPECTATING;
}
void Player::onTarget(const Vector2 &target) noexcept {
    if (state == PlayerState::DEAD) return;
    if (mouse != target) mouse = target;

    for (Entity *cell : cells)
        cell->move();

    updateCenter();

    packetHandler.sendPacket(UpdateViewport(center, scale));
}
void Player::onSplit() noexcept {
    if (state != PlayerState::PLAYING || cells.size() >= (unsigned)config["player"]["maxCells"])
        return;
    unsigned count = 0;
    for (Collidable *obj : Map::quadTree.getObjectsInBound(viewBox)) {
        std::any_cast<Entity*>(obj->data)->setColor({ 0, 0, 0 });
        ++count;
    }
    Logger::print(count);
}
void Player::onQKey() noexcept {
    if (state == PlayerState::SPECTATING)
        state = PlayerState::FREEROAM;
    else if (state == PlayerState::FREEROAM)
        state = PlayerState::SPECTATING;
}
void Player::onEject() noexcept {
    if (state != PlayerState::PLAYING)
        return;
}

void Player::onDisconnection() noexcept {
    state = PlayerState::DISCONNECTED;
    cells.clear();
}

Player::~Player() {
    onDisconnection();
}
