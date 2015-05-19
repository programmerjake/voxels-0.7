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
#ifndef GAMEUI_H_INCLUDED
#define GAMEUI_H_INCLUDED

#include "ui/ui.h"
#include "input/input.h"
#include "player/player.h"
#include "platform/platform.h"
#include "util/math_constants.h"
#include "ui/item.h"
#include "util/game_version.h"
#include "ui/image.h"
#include "texture/texture_atlas.h"
#include "world/view_point.h"
#include "ui/dynamic_label.h"
#include <mutex>
#include <sstream>
#include <deque>
#include "audio/audio_scheduler.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class GameUi : public Ui
{
    GameUi(const GameUi &) = delete;
    GameUi &operator =(const GameUi &) = delete;
private:
    AudioScheduler audioScheduler;
    std::shared_ptr<PlayingAudio> playingAudio;
    std::shared_ptr<World> world;
    WorldLockManager lock_manager;
    std::shared_ptr<ViewPoint> viewPoint;
    std::shared_ptr<GameInput> gameInput;
    std::weak_ptr<Player> playerW;
    bool addedUi = false;
    const Entity *playerEntity;
    std::shared_ptr<Ui> dialog = nullptr;
    std::shared_ptr<Ui> newDialog = nullptr;
    std::mutex newDialogLock;
    bool isWDown = false;
    bool isADown = false;
    bool isSDown = false;
    bool isDDown = false;
    void calculateMoveDirection()
    {
        VectorF v = VectorF(0);
        VectorF forward(0, 0, -1);
        VectorF left(-1, 0, 0);
        if(isWDown)
            v += forward;
        if(isSDown)
            v -= forward;
        if(isADown)
            v += left;
        if(isDDown)
            v -= left;
        if(gameInput->paused.get())
            v = VectorF(0);
        gameInput->moveDirectionPlayerRelative.set(v * 3.5f);
    }
    float deltaViewTheta = 0;
    float deltaViewPhi = 0;
    void startInventoryDialog();
    void setDialogWorldAndLockManager();
public:
    std::atomic<float> blockDestructProgress;
    GameUi(Renderer &renderer)
        : audioScheduler(),
        playingAudio(),
        world(),
        lock_manager(),
        viewPoint(),
        gameInput(),
        playerW(),
        playerEntity(),
        newDialogLock(),
        blockDestructProgress(-1.0f)
    {
    }
    void initWorld()
    {
        PositionF startingPosition = PositionF(0.5f, World::SeaLevel + 8.5f, 0.5f, Dimension::Overworld);
        world = std::make_shared<World>();
        viewPoint = std::make_shared<ViewPoint>(*world, startingPosition, GameVersion::DEBUG ? 32 : 48);
        std::shared_ptr<Player> player = Player::make(L"default-player-name", this, *world);
        playerW = player;
        gameInput = player->gameInput;
        if(GameVersion::DEBUG)
            gameInput->paused.set(true);
        playerEntity = Entities::builtin::PlayerEntity::addToWorld(*world, lock_manager, startingPosition, player);
    }
    ~GameUi()
    {
        if(playingAudio)
            playingAudio->stop();
        lock_manager.clear();
        dialog = nullptr;
        newDialog = nullptr;
        gameInput = nullptr;
        viewPoint = nullptr;
        world = nullptr;
    }
    virtual void move(double deltaTime) override
    {
        if(!dialog)
        {
            std::unique_lock<std::mutex> lockIt(newDialogLock);
            if(newDialog != nullptr)
            {
                dialog = newDialog;
                lockIt.unlock();
                setDialogWorldAndLockManager();
                add(dialog);
                dialog->reset();
            }
        }
        if(dialog)
        {
            setFocus(dialog);
        }
        if(dialog || gameInput->paused.get())
        {
            deltaViewTheta = 0;
            deltaViewPhi = 0;
        }
        else
        {
            float viewTheta = gameInput->viewTheta.get();
            float viewPhi = gameInput->viewPhi.get();
            deltaViewTheta *= 0.5f;
            deltaViewPhi *= 0.5f;
            viewTheta += deltaViewTheta;
            viewPhi += deltaViewPhi;
            viewTheta = std::fmod(viewTheta, 2 * M_PI);
            viewPhi = limit<float>(viewPhi, -M_PI / 2, M_PI / 2);
            gameInput->viewTheta.set(viewTheta);
            gameInput->viewPhi.set(viewPhi);
        }
        Display::grabMouse(!dialog && !gameInput->paused.get());
        Ui::move(deltaTime);
        world->move(deltaTime, lock_manager);
        if(dialog && dialog->isDone())
        {
            remove(dialog);
            dialog = nullptr;
            Display::grabMouse(!gameInput->paused.get());
            std::unique_lock<std::mutex> lockIt(newDialogLock);
            newDialog = nullptr;
        }
        if(playingAudio && !playingAudio->isPlaying())
        {
            playingAudio = nullptr;
        }
        if(!playingAudio)
        {
            PositionF p = playerW.lock()->getPosition();
            float height = p.y;
            float relativeHeight = height / World::SeaLevel;
            playingAudio = audioScheduler.next(world->getTimeOfDayInSeconds(), world->dayDurationInSeconds, relativeHeight, p.d, 0.1f);
        }
    }
    bool setDialog(std::shared_ptr<Ui> newDialog)
    {
        assert(newDialog);
        std::unique_lock<std::mutex> lockIt(newDialogLock);
        if(this->newDialog)
            return false;
        this->newDialog = newDialog;
        return true;
    }
    virtual bool handleTouchUp(TouchUpEvent &event) override
    {
        if(Ui::handleTouchUp(event))
            return true;
        if(dialog)
            return true;
        #warning implement
        return true;
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        if(Ui::handleTouchDown(event))
            return true;
        if(dialog)
            return true;
        #warning implement
        return true;
    }
    virtual bool handleTouchMove(TouchMoveEvent &event) override
    {
        if(Ui::handleTouchMove(event))
            return true;
        if(dialog)
            return true;
        #warning implement
        return true;
    }
    virtual bool handleMouseUp(MouseUpEvent &event) override
    {
        if(Ui::handleMouseUp(event))
            return true;
        if(event.button == MouseButton_Left)
        {
            gameInput->attack.set(false);
            return true;
        }
        if(dialog)
            return true;
        if(event.button == MouseButton_Right)
        {
            return true;
        }
        return true;
    }
    virtual bool handleMouseDown(MouseDownEvent &event) override
    {
        if(Ui::handleMouseDown(event))
            return true;
        if(dialog)
            return true;
        if(event.button == MouseButton_Left)
        {
            gameInput->attack.set(true);
            return true;
        }
        if(event.button == MouseButton_Right)
        {
            if(!dialog && !gameInput->paused.get())
            {
                EventArguments args;
                gameInput->action(args);
            }
            return true;
        }
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        if(!dialog && !gameInput->paused.get())
        {
            VectorF deltaPos = Display::transformMouseTo3D(event.x + event.deltaX, event.y + event.deltaY) - Display::transformMouseTo3D(event.x, event.y);
            deltaViewTheta -= deltaPos.x;
            deltaViewPhi += deltaPos.y;
        }
        if(Ui::handleMouseMove(event))
            return true;
        #warning implement
        return true;
    }
    virtual bool handleMouseScroll(MouseScrollEvent &event) override
    {
        if(Ui::handleMouseScroll(event))
            return true;
        if(dialog)
            return true;
        int direction = event.scrollX;
        if(direction == 0)
            direction = event.scrollY;
        EventArguments args;
        if(direction < 0)
            gameInput->hotBarMoveLeft(args);
        else if(direction > 0)
            gameInput->hotBarMoveRight(args);
        return true;
    }
    virtual bool handleKeyUp(KeyUpEvent &event) override
    {
        if(Ui::handleKeyUp(event))
            return true;
        switch(event.key)
        {
        case KeyboardKey::Space:
        {
            gameInput->jump.set(false);
            return true;
        }
        case KeyboardKey::W:
        {
            isWDown = false;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::A:
        {
            isADown = false;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::S:
        {
            isSDown = false;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::D:
        {
            isDDown = false;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::Escape:
        case KeyboardKey::E:
        {
            return true;
        }
        case KeyboardKey::Num1:
        case KeyboardKey::Num2:
        case KeyboardKey::Num3:
        case KeyboardKey::Num4:
        case KeyboardKey::Num5:
        case KeyboardKey::Num6:
        case KeyboardKey::Num7:
        case KeyboardKey::Num8:
        case KeyboardKey::Num9:
        {
            return true;
        }
        default:
            return false;
        }
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        if(Ui::handleKeyDown(event))
            return true;
        if(dialog)
            return false;
        switch(event.key)
        {
        case KeyboardKey::Space:
        {
            gameInput->jump.set(true);
            return true;
        }
        case KeyboardKey::W:
        {
            isWDown = true;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::A:
        {
            isADown = true;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::S:
        {
            isSDown = true;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::D:
        {
            isDDown = true;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::Q:
        {
            EventArguments args;
            gameInput->drop(args);
            return true;
        }
        case KeyboardKey::E:
        {
            startInventoryDialog();
            return true;
        }
        case KeyboardKey::Escape:
        {
            gameInput->paused.set(!gameInput->paused.get());
            return true;
        }
        case KeyboardKey::F12:
        {
            //setStackTraceDumpingEnabled(!getStackTraceDumpingEnabled());
            return true;
        }
        case KeyboardKey::Num1:
        case KeyboardKey::Num2:
        case KeyboardKey::Num3:
        case KeyboardKey::Num4:
        case KeyboardKey::Num5:
        case KeyboardKey::Num6:
        case KeyboardKey::Num7:
        case KeyboardKey::Num8:
        case KeyboardKey::Num9:
        {
            HotBarSelectEventArguments args((std::size_t)event.key - (std::size_t)KeyboardKey::Num1);
            gameInput->hotBarSelect(args);
            return true;
        }
        case KeyboardKey::F7:
        case KeyboardKey::F8:
        {
            if(gameInput->paused.get())
                return false;
            std::int32_t viewDistance = viewPoint->getViewDistance();
            if(event.key == KeyboardKey::F7)
            {
                viewDistance -= 16;
                if(viewDistance < 16)
                    viewDistance = 16;
            }
            else
            {
                viewDistance += 16;
                if(viewDistance > 256)
                    viewDistance = 256;
            }
            viewPoint->setViewDistance(viewDistance);
            return true;
        }
        default:
            return false;
        }
    }
    virtual bool handleKeyPress(KeyPressEvent &event) override
    {
        if(Ui::handleKeyPress(event))
            return true;
        if(dialog)
            return false;
        #warning implement
        return false;
    }
    virtual bool handleMouseMoveOut(MouseEvent &event) override
    {
        if(Ui::handleMouseMoveOut(event))
            return true;
        if(dialog)
            return true;
        #warning implement
        return true;
    }
    virtual bool handleMouseMoveIn(MouseEvent &event) override
    {
        if(Ui::handleMouseMoveIn(event))
            return true;
        if(dialog)
            return true;
        #warning implement
        return true;
    }
    virtual bool handleTouchMoveOut(TouchEvent &event) override
    {
        if(Ui::handleTouchMoveOut(event))
            return true;
        if(dialog)
            return true;
        #warning implement
        return true;
    }
    virtual bool handleTouchMoveIn(TouchEvent &event) override
    {
        if(Ui::handleTouchMoveIn(event))
            return true;
        if(dialog)
            return true;
        #warning implement
        return true;
    }
    virtual void reset() override
    {
        if(!this->world)
        {
            initWorld();
        }
        std::shared_ptr<Player> player = playerW.lock();
        assert(player);
        std::shared_ptr<World> world = this->world;
        assert(world);
        WorldLockManager &lock_manager = this->lock_manager;
        if(!addedUi)
        {
            addedUi = true;
            float simYRes = 240;
            float scale = 2 / simYRes;
            float hotBarItemSize = 20 * scale;
            std::size_t hotBarSize = player->items.itemStacks.size();
            float hotBarWidth = hotBarItemSize * hotBarSize;
            add(std::make_shared<ImageElement>(TextureAtlas::Crosshairs.td(), -1.0f / 40, 1.0f / 40, -1.0f / 40, 1.0f / 40));
            for(std::size_t i = 0; i < hotBarSize; i++)
            {
                add(std::make_shared<HotBarItem>(i * hotBarItemSize - hotBarWidth * 0.5f, (i + 1) * hotBarItemSize - hotBarWidth * 0.5f, -1, -1 + hotBarItemSize, std::shared_ptr<ItemStack>(player, &player->items.itemStacks[i][0]), [player, i]()->bool
                {
                    return player->currentItemIndex == i;
                }, &player->itemsLock));
            }
            add(std::make_shared<DynamicLabel>([player, world, &lock_manager](double deltaTime)->std::wstring
            {
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
                        BlockIterator bi = world->getBlockIterator(c.blockPosition);
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
            }, -1.0f, 1.0f, 0.8f, 1, GrayscaleF(1)));
        }
        Ui::reset();
    }
protected:
    virtual void clear(Renderer &renderer) override;
};
}
}
}

#endif // GAMEUI_H_INCLUDED
