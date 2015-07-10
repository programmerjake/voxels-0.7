/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
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
#include "util/tls.h"
#include "ui/button.h"

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
    if(backgroundCamera)
    {
        if(viewMatrixOverridden)
        {
            viewMatrixOverridden = false;
            tform = overridingViewMatrix;
        }
        int width, height;
        backgroundCamera->getSize(width, height);
        if(!backgroundCameraBuffer || (int)backgroundCameraBuffer.width() != width || (int)backgroundCameraBuffer.height() != height)
        {
            backgroundCameraBuffer = Image(width, height);
        }
        backgroundCamera->readFrameIntoImage(backgroundCameraBuffer);
        Image newBackgroundCameraBuffer = backgroundCameraBuffer;
        if(virtualRealityCallbacks)
        {
            virtualRealityCallbacks->transformBackgroundImage(newBackgroundCameraBuffer);
            assert(newBackgroundCameraBuffer);
            width = newBackgroundCameraBuffer.width();
            height = newBackgroundCameraBuffer.height();
        }
        if(!backgroundCameraTexture || (int)backgroundCameraTexture.width() < width || (int)backgroundCameraTexture.height() < height)
        {
            int adjustedWidth = 1;
            while(adjustedWidth < width)
                adjustedWidth *= 2;
            int adjustedHeight = 1;
            while(adjustedHeight < height)
                adjustedHeight *= 2;
            backgroundCameraTexture = Image(adjustedWidth, adjustedHeight);
        }
        backgroundCameraTexture.copyRect(0, backgroundCameraTexture.height() - height, width, height, newBackgroundCameraBuffer, 0, 0);
        ColorF colorizeColor = GrayscaleAF(1, 1);
        renderer << Generate::quadrilateral(TextureDescriptor(backgroundCameraTexture, 0, width / (float)backgroundCameraTexture.width(), 0, height / (float)backgroundCameraTexture.height()),
                                            VectorF(minX, minY, -1), colorizeColor,
                                            VectorF(maxX, minY, -1), colorizeColor,
                                            VectorF(maxX, maxY, -1), colorizeColor,
                                            VectorF(minX, maxY, -1), colorizeColor);
    }
    else
    {
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
    if(virtualRealityCallbacks)
    {
        renderer << start_overlay << reset_render_layer;
        virtualRealityCallbacks->render(renderer);
    }
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
    std::shared_ptr<ImageElement> crosshairs = std::make_shared<ImageElement>(TextureAtlas::Crosshairs.td(), -1.0f / 40, 1.0f / 40, -1.0f / 40, 1.0f / 40);
    crosshairsW = crosshairs;
    add(crosshairs);
    worldDependantElements.push_back(crosshairs);
    for(std::size_t i = 0; i < hotBarSize; i++)
    {
        std::shared_ptr<Element> e = std::make_shared<HotBarItem>(static_cast<float>(i) * hotBarItemSize - hotBarWidth * 0.5f, static_cast<float>(i + 1) * hotBarItemSize - hotBarWidth * 0.5f, -1.0f, -1.0f + hotBarItemSize, std::shared_ptr<ItemStack>(player, &player->items.itemStacks[i][0]), [this, i]()->bool
        {
            std::shared_ptr<Player> player = playerW.lock();
            if(player == nullptr)
                return false;
            return player->currentItemIndex == i;
        }, [this, i]()
        {
            HotBarSelectEventArguments args(i);
            gameInput->hotBarSelect(args);
        }, &player->itemsLock);
        add(e);
        worldDependantElements.push_back(e);
    }
    class FPSDisplay final : public Label
    {
        FPSDisplay(const FPSDisplay &) = delete;
        FPSDisplay &operator =(const FPSDisplay &) = delete;
    private:
        GameUi *const gameUi;
    public:
        FPSDisplay(GameUi *gameUi)
            : Label(L"uninitialized", -1.0f, 1.0f, 0.8f, 1, GrayscaleF(1)),
            gameUi(gameUi)
        {
        }
        virtual void move(double deltaTime) override
        {
            std::shared_ptr<Player> player = gameUi->playerW.lock();
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
            RayCasting::Collision c = player->castRay(*gameUi->world, gameUi->lock_manager, RayCasting::BlockCollisionMaskDefault);
            if(c.valid())
            {
                switch(c.type)
                {
                case RayCasting::Collision::Type::None:
                    break;
                case RayCasting::Collision::Type::Block:
                {
                    BlockIterator bi = gameUi->world->getBlockIterator(c.blockPosition, gameUi->lock_manager.tls);
                    Block b = bi.get(gameUi->lock_manager);
                    ss << L"Block :\n";
                    if(b.good())
                    {
                        std::wstring v = b.descriptor->getDescription(bi, gameUi->lock_manager);
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
            text = ss.str();
            Label::move(deltaTime);
        }
        virtual void layout() override
        {
            moveBy(0, getParent()->maxY - maxY);
            Label::layout();
        }
    };
    std::shared_ptr<Element> fpsDisplay = std::make_shared<FPSDisplay>(this);
    add(fpsDisplay);
    worldDependantElements.push_back(fpsDisplay);
    if(Display::needTouchControls())
    {
        class UpArrowButton final : public Button
        {
            UpArrowButton(const UpArrowButton &) = delete;
            UpArrowButton &operator =(const UpArrowButton &) = delete;
        private:
            GameUi *const gameUi;
            bool wasPressed = false;
        public:
            UpArrowButton(GameUi *gameUi)
                : Button(L"\u25B2",
                         Display::getTouchControlSize() - Display::scaleX(),
                         2.0f * Display::getTouchControlSize() - Display::scaleX(),
                         0.5f * Display::getTouchControlSize(),
                         1.5f * Display::getTouchControlSize()),
                gameUi(gameUi)
            {
                pressed.onChange.bind([this](EventArguments &)->Event::ReturnType
                {
                    if(pressed.get())
                    {
                        if(wasPressed)
                            return Event::Propagate;
                        wasPressed = true;
                        this->gameUi->pressForwardCount++;
                        this->gameUi->calculateMoveDirection();
                    }
                    else
                    {
                        if(!wasPressed)
                            return Event::Propagate;
                        wasPressed = false;
                        if(--this->gameUi->pressForwardCount < 0)
                            this->gameUi->pressForwardCount = 0;
                        this->gameUi->calculateMoveDirection();
                    }
                    return Event::Propagate;
                });
            }
            virtual void layout() override
            {
                moveBy(getParent()->minX + Display::getTouchControlSize() - minX, 0.0f);
                Button::layout();
            }
            virtual void reset() override
            {
                wasPressed = false;
                Button::reset();
            }
            virtual bool canHaveKeyboardFocus() const override
            {
                return false;
            }
        };
        std::shared_ptr<Element> upArrowButton = std::make_shared<UpArrowButton>(this);
        add(upArrowButton);
        worldDependantElements.push_back(upArrowButton);

        class DownArrowButton final : public Button
        {
            DownArrowButton(const DownArrowButton &) = delete;
            DownArrowButton &operator =(const DownArrowButton &) = delete;
        private:
            GameUi *const gameUi;
            bool wasPressed = false;
        public:
            DownArrowButton(GameUi *gameUi)
                : Button(L"\u25BC",
                         Display::getTouchControlSize() - Display::scaleX(),
                         2.0f * Display::getTouchControlSize() - Display::scaleX(),
                         -1.5f * Display::getTouchControlSize(),
                         -0.5f * Display::getTouchControlSize()),
                gameUi(gameUi)
            {
                pressed.onChange.bind([this](EventArguments &)->Event::ReturnType
                {
                    if(pressed.get())
                    {
                        if(wasPressed)
                            return Event::Propagate;
                        wasPressed = true;
                        this->gameUi->pressBackwardCount++;
                        this->gameUi->calculateMoveDirection();
                    }
                    else
                    {
                        if(!wasPressed)
                            return Event::Propagate;
                        wasPressed = false;
                        if(--this->gameUi->pressBackwardCount < 0)
                            this->gameUi->pressBackwardCount = 0;
                        this->gameUi->calculateMoveDirection();
                    }
                    return Event::Propagate;
                });
            }
            virtual void layout() override
            {
                moveBy(getParent()->minX + Display::getTouchControlSize() - minX, 0.0f);
                Button::layout();
            }
            virtual void reset() override
            {
                wasPressed = false;
                Button::reset();
            }
            virtual bool canHaveKeyboardFocus() const override
            {
                return false;
            }
        };
        std::shared_ptr<Element> downArrowButton = std::make_shared<DownArrowButton>(this);
        add(downArrowButton);
        worldDependantElements.push_back(downArrowButton);

        class LeftArrowButton final : public Button
        {
            LeftArrowButton(const LeftArrowButton &) = delete;
            LeftArrowButton &operator =(const LeftArrowButton &) = delete;
        private:
            GameUi *const gameUi;
            bool wasPressed = false;
        public:
            LeftArrowButton(GameUi *gameUi)
                : Button(L"\u25C4",
                         -Display::scaleX(),
                         Display::getTouchControlSize() - Display::scaleX(),
                         -0.5f * Display::getTouchControlSize(),
                         0.5f * Display::getTouchControlSize()),
                gameUi(gameUi)
            {
                pressed.onChange.bind([this](EventArguments &)->Event::ReturnType
                {
                    if(pressed.get())
                    {
                        if(wasPressed)
                            return Event::Propagate;
                        wasPressed = true;
                        this->gameUi->pressLeftCount++;
                        this->gameUi->calculateMoveDirection();
                    }
                    else
                    {
                        if(!wasPressed)
                            return Event::Propagate;
                        wasPressed = false;
                        if(--this->gameUi->pressLeftCount < 0)
                            this->gameUi->pressLeftCount = 0;
                        this->gameUi->calculateMoveDirection();
                    }
                    return Event::Propagate;
                });
            }
            virtual void layout() override
            {
                moveBy(getParent()->minX - minX, 0.0f);
                Button::layout();
            }
            virtual void reset() override
            {
                wasPressed = false;
                Button::reset();
            }
            virtual bool canHaveKeyboardFocus() const override
            {
                return false;
            }
        };
        std::shared_ptr<Element> leftArrowButton = std::make_shared<LeftArrowButton>(this);
        add(leftArrowButton);
        worldDependantElements.push_back(leftArrowButton);

        class RightArrowButton final : public Button
        {
            RightArrowButton(const RightArrowButton &) = delete;
            RightArrowButton &operator =(const RightArrowButton &) = delete;
        private:
            GameUi *const gameUi;
            bool wasPressed = false;
        public:
            RightArrowButton(GameUi *gameUi)
                : Button(L"\u25BA",
                         2.0f * Display::getTouchControlSize() - Display::scaleX(),
                         3.0f * Display::getTouchControlSize() - Display::scaleX(),
                         -0.5f * Display::getTouchControlSize(),
                         0.5f * Display::getTouchControlSize()),
                gameUi(gameUi)
            {
                pressed.onChange.bind([this](EventArguments &)->Event::ReturnType
                {
                    if(pressed.get())
                    {
                        if(wasPressed)
                            return Event::Propagate;
                        wasPressed = true;
                        this->gameUi->pressRightCount++;
                        this->gameUi->calculateMoveDirection();
                    }
                    else
                    {
                        if(!wasPressed)
                            return Event::Propagate;
                        wasPressed = false;
                        if(--this->gameUi->pressRightCount < 0)
                            this->gameUi->pressRightCount = 0;
                        this->gameUi->calculateMoveDirection();
                    }
                    return Event::Propagate;
                });
            }
            virtual void layout() override
            {
                moveBy(getParent()->minX + 2.0f * Display::getTouchControlSize() - minX, 0.0f);
                Button::layout();
            }
            virtual void reset() override
            {
                wasPressed = false;
                Button::reset();
            }
            virtual bool canHaveKeyboardFocus() const override
            {
                return false;
            }
        };
        std::shared_ptr<Element> rightArrowButton = std::make_shared<RightArrowButton>(this);
        add(rightArrowButton);
        worldDependantElements.push_back(rightArrowButton);

        class JumpButton final : public Button
        {
            JumpButton(const JumpButton &) = delete;
            JumpButton &operator =(const JumpButton &) = delete;
        private:
            GameUi *const gameUi;
            bool wasPressed = false;
        public:
            JumpButton(GameUi *gameUi)
                : Button(L"\u2666",
                         Display::getTouchControlSize() - Display::scaleX(),
                         2.0f * Display::getTouchControlSize() - Display::scaleX(),
                         -0.5f * Display::getTouchControlSize(),
                         0.5f * Display::getTouchControlSize()),
                gameUi(gameUi)
            {
                pressed.onChange.bind([this](EventArguments &)->Event::ReturnType
                {
                    if(pressed.get())
                    {
                        if(wasPressed)
                            return Event::Propagate;
                        wasPressed = true;
                        this->gameUi->handleJumpDown();
                    }
                    else
                    {
                        if(!wasPressed)
                            return Event::Propagate;
                        wasPressed = false;
                        this->gameUi->handleJumpUp();
                    }
                    return Event::Propagate;
                });
            }
            virtual void layout() override
            {
                moveBy(getParent()->minX + Display::getTouchControlSize() - minX, 0.0f);
                Button::layout();
            }
            virtual void reset() override
            {
                wasPressed = false;
                Button::reset();
            }
            virtual bool canHaveKeyboardFocus() const override
            {
                return false;
            }
        };
        std::shared_ptr<Element> jumpButton = std::make_shared<JumpButton>(this);
        add(jumpButton);
        worldDependantElements.push_back(jumpButton);

        class PauseButton final : public Button
        {
            PauseButton(const PauseButton &) = delete;
            PauseButton &operator =(const PauseButton &) = delete;
        private:
            GameUi *const gameUi;
        public:
            PauseButton(GameUi *gameUi)
                : Button(L"Menu",
                         Display::scaleX() - Display::getTouchControlSize(),
                         Display::scaleX(),
                         Display::scaleY() - Display::getTouchControlSize(),
                         Display::scaleY()),
                gameUi(gameUi)
            {
                click.bind([this](EventArguments &)->Event::ReturnType
                {
                    this->gameUi->startMainMenu();
                    return Event::Propagate;
                });
            }
            virtual void layout() override
            {
                moveTopRightTo(getParent()->maxX, getParent()->maxY);
                Button::layout();
            }
            virtual bool canHaveKeyboardFocus() const override
            {
                return false;
            }
        };
        std::shared_ptr<Element> pauseButton = std::make_shared<PauseButton>(this);
        add(pauseButton);
        worldDependantElements.push_back(pauseButton);

        class DropButton final : public Button
        {
            DropButton(const DropButton &) = delete;
            DropButton &operator =(const DropButton &) = delete;
        private:
            GameUi *const gameUi;
        public:
            DropButton(GameUi *gameUi)
                : Button(L"Drop",
                         -Display::scaleX(),
                         Display::getTouchControlSize() - Display::scaleX(),
                         0.5f * Display::getTouchControlSize(),
                         1.5f * Display::getTouchControlSize()),
                gameUi(gameUi)
            {
                click.bind([this](EventArguments &)->Event::ReturnType
                {
                    EventArguments args;
                    this->gameUi->gameInput->drop(args);
                    return Event::Propagate;
                });
            }
            virtual void layout() override
            {
                moveBy(getParent()->minX - minX, 0.0f);
                Button::layout();
            }
            virtual bool canHaveKeyboardFocus() const override
            {
                return false;
            }
        };
        std::shared_ptr<Element> dropButton = std::make_shared<DropButton>(this);
        add(dropButton);
        worldDependantElements.push_back(dropButton);

        class SneakButton final : public Button
        {
            SneakButton(const SneakButton &) = delete;
            SneakButton &operator =(const SneakButton &) = delete;
        private:
            GameUi *const gameUi;
            bool wasPressed = false;
        public:
            SneakButton(GameUi *gameUi)
                : Button(L"Sneak",
                         Display::scaleX() - Display::getTouchControlSize(),
                         Display::scaleX(),
                         Display::scaleY() - Display::getTouchControlSize() * 2.5f,
                         Display::scaleY() - Display::getTouchControlSize() * 1.5f),
                gameUi(gameUi)
            {
                pressed.onChange.bind([this](EventArguments &)->Event::ReturnType
                {
                    if(pressed.get())
                    {
                        if(wasPressed)
                            return Event::Propagate;
                        wasPressed = true;
                        if(this->gameUi->pressSneakCount++ == 0)
                            this->gameUi->gameInput->sneak.set(true);
                    }
                    else
                    {
                        if(!wasPressed)
                            return Event::Propagate;
                        wasPressed = false;
                        if(--this->gameUi->pressSneakCount <= 0)
                        {
                            this->gameUi->gameInput->sneak.set(false);
                            this->gameUi->pressSneakCount = 0;
                        }
                    }
                    return Event::Propagate;
                });
            }
            virtual void layout() override
            {
                moveTopRightTo(getParent()->maxX, getParent()->maxY - Display::getTouchControlSize() * 1.5f);
                Button::layout();
            }
            virtual bool canHaveKeyboardFocus() const override
            {
                return false;
            }
            virtual void reset() override
            {
                wasPressed = false;
                Button::reset();
            }
        };
        std::shared_ptr<Element> sneakButton = std::make_shared<SneakButton>(this);
        add(sneakButton);
        worldDependantElements.push_back(sneakButton);

        class InventoryButton final : public Button
        {
            InventoryButton(const InventoryButton &) = delete;
            InventoryButton &operator =(const InventoryButton &) = delete;
        private:
            GameUi *const gameUi;
        public:
            InventoryButton(GameUi *gameUi)
                : Button(L"\u2219\u2219\u2219",
                         Display::scaleX() - Display::getTouchControlSize(),
                         Display::scaleX(),
                         -Display::scaleY(),
                         Display::getTouchControlSize() - Display::scaleY()),
                gameUi(gameUi)
            {
                click.bind([this](EventArguments &)->Event::ReturnType
                {
                    this->gameUi->startInventoryDialog();
                    return Event::Propagate;
                });
            }
            virtual void layout() override
            {
                moveBottomRightTo(getParent()->maxX, getParent()->minY);
                Button::layout();
            }
            virtual bool canHaveKeyboardFocus() const override
            {
                return false;
            }
        };
        std::shared_ptr<Element> inventoryButton = std::make_shared<InventoryButton>(this);
        add(inventoryButton);
        worldDependantElements.push_back(inventoryButton);
    }
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
        TLS tls;
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
    touches(),
    backgroundCamera(),
    backgroundCameraTexture(),
    backgroundCameraBuffer(),
    virtualRealityCallbacks(),
    crosshairsW(),
    blockDestructProgress(-1.0f)
{
    gameInput->paused.set(true);
}

void GameUi::startMainMenu()
{
    setDialog(mainMenu);
}

void GameUi::handleSetViewMatrix(Matrix viewMatrix)
{
#if 1
    overridingViewMatrix = viewMatrix;
    viewMatrixOverridden = true;
#else
    float viewTheta = gameInput->viewTheta.get();
    float viewPhi = gameInput->viewPhi.get();
    float viewPsi = gameInput->viewPsi.get();
    // assume that viewMatrix is a rotation and/or a translation matrix
	Matrix worldOrientationMatrix = inverse(viewMatrix);
	VectorF viewPoint = transform(worldOrientationMatrix, VectorF(0)) - Player::getViewPositionOffset();
	std::shared_ptr<Player> player = playerW.lock();
    if(player)
    {
        PositionF playerPosition;
        playerPosition = player->getPosition();
        player->warpToPosition(PositionF(viewPoint, playerPosition.d));
    }
	VectorF backVector = normalize(worldOrientationMatrix.applyNoTranslate(VectorF(0, 0, 1)));
	VectorF upVector = normalize(worldOrientationMatrix.applyNoTranslate(VectorF(0, 1, 0)));
	VectorF backVectorXZ = backVector;
	backVectorXZ.y = 0;
	viewPhi = std::atan2(-backVector.y, abs(backVectorXZ));
	if (absSquared(backVectorXZ) >= 1e-8)
	{
		viewTheta = std::atan2(backVector.x, backVector.z);
		upVector =
			transform(Matrix::rotateY(-viewTheta).concat(Matrix::rotateX(-viewPhi)), upVector);
		viewPsi = std::atan2(-upVector.x, upVector.y);
	}
	else
	{
		viewPsi = 0;
	    if(viewPhi < 0)
	        upVector = -upVector;
	    viewTheta = std::atan2(upVector.x, upVector.z);
	}
    gameInput->viewTheta.set(viewTheta);
    gameInput->viewPhi.set(viewPhi);
    gameInput->viewPsi.set(viewPsi);
#endif
}

}
}
}
