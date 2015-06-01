/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "ui/gameui.h"
#include "render/generate.h"
#include "texture/texture_atlas.h"
#include "world/view_point.h"
#include "ui/inventory.h"
#include "ui/player_dialog.h"
#include "ui/main_menu.h"
#include <vector>
#include "stream/stream.h"
#include "util/logging.h"
#include "platform/thread_name.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
namespace
{
Mesh makeSelectionMesh()
{
    TextureDescriptor td = TextureAtlas::Selection.td();
    return transform(Matrix::translate(-0.5, -0.5, -0.5).concat(Matrix::scale(1.01)).concat(Matrix::translate(0.5, 0.5, 0.5)), Generate::unitBox(td, td, td, td, td, td));
}
Mesh getSelectionMesh()
{
    static thread_local Mesh mesh = makeSelectionMesh();
    return mesh;
}
std::vector<Mesh> makeDestructionMeshes()
{
    std::vector<Mesh> destructionMeshes;
    destructionMeshes.reserve((std::size_t)TextureAtlas::DeleteFrameCount());
    for(int i = 0; i < TextureAtlas::DeleteFrameCount(); i++)
    {
        TextureDescriptor td = TextureAtlas::Delete(i).td();
        destructionMeshes.emplace_back();
        destructionMeshes.back().append(transform(Matrix::translate(-0.52, -0.5, -0.5).concat(Matrix::scale(0.99f)).concat(Matrix::translate(0.5, 0.5, 0.5)), Generate::unitBox(td, TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor())));
        destructionMeshes.back().append(transform(Matrix::translate(-0.48, -0.5, -0.5).concat(Matrix::scale(0.99f)).concat(Matrix::translate(0.5, 0.5, 0.5)), Generate::unitBox(TextureDescriptor(), td, TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor())));
        destructionMeshes.back().append(transform(Matrix::translate(-0.5, -0.52, -0.5).concat(Matrix::scale(0.99f)).concat(Matrix::translate(0.5, 0.5, 0.5)), Generate::unitBox(TextureDescriptor(), TextureDescriptor(), td, TextureDescriptor(), TextureDescriptor(), TextureDescriptor())));
        destructionMeshes.back().append(transform(Matrix::translate(-0.5, -0.48, -0.5).concat(Matrix::scale(0.99f)).concat(Matrix::translate(0.5, 0.5, 0.5)), Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), td, TextureDescriptor(), TextureDescriptor())));
        destructionMeshes.back().append(transform(Matrix::translate(-0.5, -0.5, -0.52).concat(Matrix::scale(0.99f)).concat(Matrix::translate(0.5, 0.5, 0.5)), Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), td, TextureDescriptor())));
        destructionMeshes.back().append(transform(Matrix::translate(-0.5, -0.5, -0.48).concat(Matrix::scale(0.99f)).concat(Matrix::translate(0.5, 0.5, 0.5)), Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), td)));
    }
    return std::move(destructionMeshes);
}
Mesh getDestructionMesh(float progress)
{
    static thread_local std::vector<Mesh> meshes = makeDestructionMeshes();
    int index = ifloor(progress * (int)(meshes.size() - 1));
    index = limit<int>(index, 0, meshes.size() - 1);
    return meshes[index];
}
void renderSunOrMoon(Renderer &renderer, Matrix orientationTransform, VectorF position, TextureDescriptor td, float extent = 0.1f)
{
    VectorF upVector = extent * normalize(cross(position, VectorF(0, 0, 1)));
    VectorF rightVector = extent * normalize(cross(position, upVector));

    VectorF nxny = position - upVector - rightVector;
    VectorF nxpy = position + upVector - rightVector;
    VectorF pxny = position - upVector + rightVector;
    VectorF pxpy = position + upVector + rightVector;
    ColorF c = colorizeIdentity();
    renderer << transform(inverse(orientationTransform), Generate::quadrilateral(td,
                                                                 nxny, c,
                                                                 pxny, c,
                                                                 pxpy, c,
                                                                 nxpy, c));
}
}
void GameUi::clear(Renderer &renderer)
{
    std::shared_ptr<Player> player = playerW.lock();
    if(!player)
    {
        background = GrayscaleF(0.4f);
        Ui::clear(renderer);
        return;
    }
    PositionF playerPosition = player->getPosition();
    background = world->getSkyColor(playerPosition);
    Ui::clear(renderer);
    Matrix tform = player->getViewTransform();
    viewPoint->setPosition(playerPosition);
    renderer << RenderLayer::Opaque;
    if(hasSun(playerPosition.d))
        renderSunOrMoon(renderer, player->getWorldOrientationTransform(), world->getSunPosition(), TextureAtlas::Sun.td());
    if(hasMoon(playerPosition.d))
    {
        TextureDescriptor moonTD;
        switch(world->getVisibleMoonPhase())
        {
        case 0:
            moonTD = TextureAtlas::Moon0.td();
            break;
        case 1:
            moonTD = TextureAtlas::Moon1.td();
            break;
        case 2:
            moonTD = TextureAtlas::Moon2.td();
            break;
        case 3:
            moonTD = TextureAtlas::Moon3.td();
            break;
        case 4:
            moonTD = TextureAtlas::Moon4.td();
            break;
        case 5:
            moonTD = TextureAtlas::Moon5.td();
            break;
        case 6:
            moonTD = TextureAtlas::Moon6.td();
            break;
        default:
            moonTD = TextureAtlas::Moon7.td();
            break;
        }
        renderSunOrMoon(renderer, player->getWorldOrientationTransform(), world->getMoonPosition(), moonTD);
    }
    RayCasting::Collision collision = player->castRay(*world, lock_manager, RayCasting::BlockCollisionMaskDefault);
    Mesh additionalObjects;
    if(collision.valid())
    {
        Matrix selectionBoxTransform;
        bool drawSelectionBox = false;
        bool drawDestructionBox = false;
        switch(collision.type)
        {
        case RayCasting::Collision::Type::Block:
        {
            BlockIterator bi = world->getBlockIterator(collision.blockPosition, lock_manager.tls);
            Block b = bi.get(lock_manager);
            if(b.good())
            {
                selectionBoxTransform = b.descriptor->getSelectionBoxTransform(b).concat(Matrix::translate(collision.blockPosition));
                drawSelectionBox = true;
                drawDestructionBox = true;
            }
            break;
        }
        case RayCasting::Collision::Type::Entity:
        {
            Entity *entity = collision.entity;
            if(entity != nullptr && entity->good())
            {
                selectionBoxTransform = entity->descriptor->getSelectionBoxTransform(*entity);
                drawSelectionBox = true;
            }
        }
        case RayCasting::Collision::Type::None:
            break;
        }
        if(drawSelectionBox)
        {
            additionalObjects.append(transform(selectionBoxTransform, getSelectionMesh()));
        }
        if(drawDestructionBox)
        {
            float progress = blockDestructProgress.load(std::memory_order_relaxed);
            if(progress >= 0)
            {
                additionalObjects.append(transform(selectionBoxTransform, getDestructionMesh(progress)));
            }
        }
    }
    renderer << start_overlay;
    viewPoint->render(renderer, tform, lock_manager, additionalObjects);
    renderer << start_overlay << reset_render_layer;
}
void GameUi::startInventoryDialog()
{
    if(world)
        setDialog(std::make_shared<PlayerInventory>(playerW.lock()));
}
void GameUi::setDialogWorldAndLockManager()
{
    std::shared_ptr<PlayerDialog> playerDialog = std::dynamic_pointer_cast<PlayerDialog>(dialog);
    if(playerDialog != nullptr && world != nullptr)
        playerDialog->setWorldAndLockManager(*world, lock_manager);
}

void GameUi::setWorld(std::shared_ptr<World> world)
{
    this->world = world;
    std::shared_ptr<Player> player = nullptr;
    for(std::shared_ptr<Player> p : world->players().lock())
    {
        player = p;
        break;
    }
    assert(player != nullptr);
    playerEntity = player->getPlayerEntity();
    viewPoint = std::make_shared<ViewPoint>(*world, playerEntity->physicsObject->getPosition(), GameVersion::DEBUG ? 32 : 48);
    playerW = player;
    player->gameInput->copy(*gameInput);
    gameInput = player->gameInput;
    player->gameUi = this;
    addWorldUi();
}

void GameUi::clearWorld(bool stopSound)
{
    if(!world)
        return;
    if(stopSound)
    {
        if(playingAudio)
            playingAudio->stop();
        playingAudio = nullptr;
    }
    audioScheduler.reset();
    lock_manager.clear();
    while(!worldDependantElements.empty())
    {
        std::shared_ptr<Element> e = std::move(worldDependantElements.back());
        worldDependantElements.pop_back();
        remove(e);
    }
    world->paused(false);
    std::shared_ptr<World> theWorld = world; // to extend life
    std::shared_ptr<Player> player = playerW.lock();
    player->gameUi = nullptr;
    player = nullptr;
    world = nullptr;
    viewPoint = nullptr;
    playerEntity = nullptr;
    playerW = std::weak_ptr<Player>();
    std::shared_ptr<GameInput> newGameInput = std::make_shared<GameInput>();
    newGameInput->copy(*gameInput);
    gameInput = newGameInput;
}

void GameUi::addWorldUi()
{
    if(!addedUi || !world)
        return;
    float simYRes = 240;
    float scale = 2 / simYRes;
    float hotBarItemSize = 20 * scale;
    std::shared_ptr<Player> player = playerW.lock();
    std::size_t hotBarSize = player->items.itemStacks.size();
    float hotBarWidth = hotBarItemSize * hotBarSize;
    std::shared_ptr<Element> crosshairs = std::make_shared<ImageElement>(TextureAtlas::Crosshairs.td(), -1.0f / 40, 1.0f / 40, -1.0f / 40, 1.0f / 40);
    add(crosshairs);
    worldDependantElements.push_back(crosshairs);
    for(std::size_t i = 0; i < hotBarSize; i++)
    {
        std::shared_ptr<Element> e = std::make_shared<HotBarItem>(i * hotBarItemSize - hotBarWidth * 0.5f, (i + 1) * hotBarItemSize - hotBarWidth * 0.5f, -1, -1 + hotBarItemSize, std::shared_ptr<ItemStack>(player, &player->items.itemStacks[i][0]), [this, i]()->bool
        {
            std::shared_ptr<Player> player = playerW.lock();
            if(player == nullptr)
                return false;
            return player->currentItemIndex == i;
        }, &player->itemsLock);
        add(e);
        worldDependantElements.push_back(e);
    }
    std::shared_ptr<Element> fpsDisplay = std::make_shared<DynamicLabel>([this](double deltaTime)->std::wstring
    {
        std::shared_ptr<Player> player = playerW.lock();
        static thread_local std::deque<double> samples;
        samples.push_back(deltaTime);
        double totalSampleTime = 0;
        double minDeltaTime = deltaTime, maxDeltaTime = deltaTime;
        for(double v : samples)
        {
            totalSampleTime += v;
            if(v > minDeltaTime)
                minDeltaTime = v;
            if(v < maxDeltaTime)
                maxDeltaTime = v;
        }
        double averageDeltaTime = totalSampleTime / samples.size();
        while(totalSampleTime > 5 && !samples.empty())
        {
            totalSampleTime -= samples.front();
            samples.pop_front();
        }
        std::wostringstream ss;
        ss << L"FPS:";
        ss.setf(std::ios::fixed);
        ss.precision(1);
        ss.width(5);
        if(maxDeltaTime > 0)
            ss << 1.0 / maxDeltaTime;
        else
            ss << L"     ";
        ss << L"/";
        ss.width(5);
        if(averageDeltaTime > 0)
            ss << 1.0 / averageDeltaTime;
        else
            ss << L"     ";
        ss << L"/";
        ss.width(5);
        if(minDeltaTime > 0)
            ss << 1.0 / minDeltaTime;
        else
            ss << L"     ";
        ss << L"\n";
        RayCasting::Collision c = player->castRay(*world, lock_manager, RayCasting::BlockCollisionMaskDefault);
        if(c.valid())
        {
            switch(c.type)
            {
            case RayCasting::Collision::Type::None:
                break;
            case RayCasting::Collision::Type::Block:
            {
                BlockIterator bi = world->getBlockIterator(c.blockPosition, lock_manager.tls);
                Block b = bi.get(lock_manager);
                ss << L"Block :\n";
                if(b.good())
                {
                    std::wstring v = b.descriptor->getDescription(bi, lock_manager);
                    const wchar_t *const splittingCharacters = L",= ()_";
                    const std::size_t lineLength = 30, lineLengthVariance = 5;
                    const std::size_t searchStartPos = lineLength - lineLengthVariance;
                    const std::size_t searchEndPos = lineLength + lineLengthVariance;
                    while(v.size() > lineLength)
                    {
                        std::size_t splitPos = v.find_first_of(splittingCharacters, searchStartPos);
                        if(splitPos == std::wstring::npos || splitPos > searchEndPos)
                            splitPos = lineLength;
                        ss << v.substr(0, splitPos + 1) << L"\n";
                        v.erase(0, splitPos + 1);
                    }
                    v.resize(searchEndPos, L' ');
                    ss << v;
                }
                else
                {
                    ss << L"null_block";
                }
                break;
            }
            case RayCasting::Collision::Type::Entity:
                break;
            }
        }
        return ss.str();
    }, -1.0f, 1.0f, 0.8f, 1, GrayscaleF(1));
    add(fpsDisplay);
    worldDependantElements.push_back(fpsDisplay);
}

void GameUi::createNewWorld()
{
    clearWorld();
    generatingWorld = true;
    generatedWorld = nullptr;
    worldGenerateThread = std::thread([this]()
    {
        setThreadName(L"world generate");
        TLS tls;
        WorldLockManager lock_manager(tls);
        try
        {
            generatedWorld = std::make_shared<World>(&abortWorldCreation);
            PositionF startingPosition = PositionF(0.5f, World::SeaLevel + 8.5f, 0.5f, Dimension::Overworld);
            std::shared_ptr<Player> newPlayer = Player::make(L"default-player-name", nullptr, generatedWorld);
            Entities::builtin::PlayerEntity::addToWorld(*generatedWorld, lock_manager, startingPosition, newPlayer);
        }
        catch(WorldConstructionAborted &)
        {
        }
        generatingWorld = false;
    });
    if(addedUi)
    {
        std::shared_ptr<Element> e = std::make_shared<Label>(L"Creating World...", -0.7f, 0.7f, -0.3f, 0.3f);
        worldCreationMessage = e;
        add(e);
    }
}

void GameUi::loadWorld(std::wstring fileName)
{
    clearWorld();
    generatingWorld = true;
    generatedWorld = nullptr;
    worldGenerateThread = std::thread([this, fileName]()
    {
        setThreadName(L"load world");
        try
        {
            stream::FileReader reader(fileName);
            generatedWorld = World::read(reader);
        }
        catch(stream::IOException &e)
        {
            getDebugLog() << L"World Load Error : " << e.what() << postnl;
        }
        generatingWorld = false;
    });
    if(addedUi)
    {
        std::shared_ptr<Element> e = std::make_shared<Label>(L"Loading World...", -0.7f, 0.7f, -0.3f, 0.3f);
        worldCreationMessage = e;
        add(e);
    }
}

void GameUi::addUi()
{
    addWorldUi();
    mainMenu = std::make_shared<MainMenu>();
    setDialog(mainMenu);
}

GameUi::GameUi(Renderer &renderer, TLS &tls)
    : audioScheduler(),
    playingAudio(),
    world(),
    lock_manager(tls),
    viewPoint(),
    gameInput(std::make_shared<GameInput>()),
    playerW(),
    playerEntity(),
    newDialogLock(),
    worldDependantElements(),
    generatingWorld(false),
    worldGenerateThread(),
    generatedWorld(nullptr),
    worldCreationMessage(),
    abortWorldCreation(false),
    mainMenu(),
    mainMenuSong(std::make_shared<Audio>(L"menu-theme.ogg", true)),
    blockDestructProgress(-1.0f)
{
    gameInput->paused.set(true);
}

void GameUi::startMainMenu()
{
    setDialog(mainMenu);
}

}
}
}
