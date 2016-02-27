/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
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
#include <vector>
#include "audio/audio_scheduler.h"
#include "platform/audio.h"
#include "util/tls.h"
#include "platform/video_input.h"
#include "vr/vr_callbacks.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class MainMenu;

class GameUi : public Ui
{
    friend class MainMenu;
    GameUi(const GameUi &) = delete;
    GameUi &operator=(const GameUi &) = delete;

private:
    struct TouchStruct final
    {
        VectorF startPosition;
        VectorF position;
        enum class State
        {
            Move,
            Attack,
            StartDelay,
        };
        float startDelayTimeLeft;
        State state;
        TouchStruct(VectorF position, bool gotTouchDown)
            : startPosition(position),
              position(position),
              startDelayTimeLeft(gotTouchDown ? 0.3f : 0.0f),
              state(gotTouchDown ? State::StartDelay : State::Move)
        {
        }
    };
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
    int pressForwardCount = 0;
    int pressLeftCount = 0;
    int pressBackwardCount = 0;
    int pressRightCount = 0;
    int pressAttackCount = 0;
    int pressJumpCount = 0;
    int pressSneakCount = 0;
    void resetInput()
    {
        pressForwardCount = 0;
        pressLeftCount = 0;
        pressBackwardCount = 0;
        pressRightCount = 0;
        pressAttackCount = 0;
        pressJumpCount = 0;
        pressSneakCount = 0;
        touches.clear();
        deltaViewTheta = 0;
        deltaViewPhi = 0;
    }
    void calculateMoveDirection()
    {
        VectorF v = VectorF(0);
        VectorF forward(0, 0, -1);
        VectorF left(-1, 0, 0);
        if(pressForwardCount > 0)
            v += forward;
        if(pressBackwardCount > 0)
            v -= forward;
        if(pressLeftCount > 0)
            v += left;
        if(pressRightCount > 0)
            v -= left;
        if(gameInput->paused.get())
            v = VectorF(0);
        gameInput->moveDirectionPlayerRelative.set(v);
    }
    float deltaViewTheta = 0;
    float deltaViewPhi = 0;
    float jumpDoubleTapTimeLeft = 0;
    void startInventoryDialog();
    void setDialogWorldAndLockManager();
    void setWorld(std::shared_ptr<World> world);
    void clearWorld(bool stopSound = true);
    void addWorldUi();
    std::vector<std::shared_ptr<Element>> worldDependantElements;
    std::atomic_bool generatingWorld;
    std::thread worldGenerateThread;
    std::shared_ptr<World> generatedWorld;
    std::weak_ptr<Element> worldCreationMessage;
    std::atomic_bool abortWorldCreation;
    std::shared_ptr<Ui> mainMenu;
    std::shared_ptr<Audio> mainMenuSong;
    std::unordered_map<int, TouchStruct> touches;
    std::unique_ptr<VideoInput> backgroundCamera;
    Image backgroundCameraTexture;
    Image backgroundCameraBuffer;
    std::unique_ptr<VirtualRealityCallbacks> virtualRealityCallbacks;
    Transform overridingViewMatrix = Transform();
    bool viewMatrixOverridden = false;
    std::weak_ptr<ImageElement> crosshairsW;
    void addUi();
    void finalizeCreateWorld()
    {
        worldGenerateThread.join();
        if(abortWorldCreation.exchange(false))
        {
            generatedWorld = nullptr;
            startMainMenu();
            return;
        }
        std::shared_ptr<World> newWorld = generatedWorld;
        if(newWorld == nullptr)
        {
            std::shared_ptr<Element> e = worldCreationMessage.lock();
            if(e)
                remove(e);
            worldCreationMessage = std::weak_ptr<Element>();
            startMainMenu();
            return;
        }
        setWorld(newWorld);
        generatedWorld = nullptr;
        std::shared_ptr<Element> e = worldCreationMessage.lock();
        if(e)
            remove(e);
        worldCreationMessage = std::weak_ptr<Element>();
        if(playingAudio)
            playingAudio->stop();
        playingAudio = nullptr;
        gameInput->paused.set(false);
    }
    void startMainMenu();
    void handleJumpDown()
    {
        if(pressJumpCount++ > 0)
            return;
        gameInput->jump.set(true);
        if(jumpDoubleTapTimeLeft > 0)
        {
            gameInput->fly.set(!gameInput->fly.get() && gameInput->isCreativeMode.get());
        }
        else
            jumpDoubleTapTimeLeft = 0.3f;
    }
    void handleJumpUp()
    {
        if(--pressJumpCount <= 0)
        {
            gameInput->jump.set(false);
            pressJumpCount = 0;
        }
    }
    void handleSetViewMatrix(Transform viewMatrix);

public:
    std::atomic<float> blockDestructProgress;
    GameUi();
    void createNewWorld();
    void loadWorld(std::wstring fileName);
    ~GameUi()
    {
        abortWorldCreation = true;
        if(worldGenerateThread.joinable())
        {
            worldGenerateThread.join();
            generatedWorld = nullptr;
            std::shared_ptr<Element> e = worldCreationMessage.lock();
            if(e)
                remove(e);
        }
        dialog = nullptr;
        newDialog = nullptr;
        clearWorld();
    }
    void setBackgroundCamera(std::unique_ptr<VideoInput> &&newBackgroundCamera)
    {
        backgroundCamera = std::move(newBackgroundCamera);
        if(backgroundCamera && !virtualRealityCallbacks)
        {
            virtualRealityCallbacks = VirtualRealityCallbacks::make();
        }
        else if(!backgroundCamera && virtualRealityCallbacks)
        {
            virtualRealityCallbacks.reset();
        }
    }
    bool hasBackgroundCamera() const
    {
        return backgroundCamera != nullptr;
    }
    const VideoInputDevice *getBackgroundCameraDevice() const
    {
        if(backgroundCamera != nullptr)
            return backgroundCamera->videoInputDevice;
        return nullptr;
    }
    virtual void move(double deltaTime) override
    {
        jumpDoubleTapTimeLeft -= static_cast<float>(deltaTime);
        if(jumpDoubleTapTimeLeft < 0)
            jumpDoubleTapTimeLeft = 0;
        bool needSetViewDirection = true;
        std::shared_ptr<ImageElement> crosshairs = crosshairsW.lock();
        if(crosshairs)
        {
            crosshairs->colorizeColor = colorizeIdentity();
        }
        if(virtualRealityCallbacks)
        {
            std::shared_ptr<Player> player = playerW.lock();
            if(player)
            {
                virtualRealityCallbacks->move(
                    deltaTime,
                    player->getViewTransform(),
                    [this, &needSetViewDirection, crosshairs](Transform viewMatrix)
                    {
                        handleSetViewMatrix(viewMatrix);
                        if(!viewMatrixOverridden)
                            needSetViewDirection = false;
                        if(crosshairs)
                        {
                            crosshairs->colorizeColor = GrayscaleAF(1, 0);
                        }
                    });
            }
        }
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
            resetInput();
        }
        else
        {
            for(std::pair<const int, TouchStruct> &p : touches)
            {
                TouchStruct &touch = std::get<1>(p);
                if(touch.state == TouchStruct::State::StartDelay)
                {
                    touch.startDelayTimeLeft -= static_cast<float>(deltaTime);
                    if(touch.startDelayTimeLeft <= 0)
                    {
                        touch.startDelayTimeLeft = 0;
                        touch.state = TouchStruct::State::Attack;
                        if(pressAttackCount++ == 0)
                        {
                            gameInput->attack.set(true);
                        }
                    }
                }
            }
            if(needSetViewDirection)
            {
                float viewTheta = gameInput->viewTheta.get();
                float viewPhi = gameInput->viewPhi.get();
                deltaViewTheta *= 0.5f;
                deltaViewPhi *= 0.5f;
                viewTheta += deltaViewTheta;
                viewPhi += deltaViewPhi;
                viewTheta = std::fmod(viewTheta, 2 * (float)M_PI);
                viewPhi = limit<float>(viewPhi, -(float)M_PI / 2, (float)M_PI / 2);
                gameInput->viewTheta.set(viewTheta);
                gameInput->viewPhi.set(viewPhi);
                gameInput->viewPsi.set(0.0f);
            }
            else
            {
                deltaViewTheta = 0;
                deltaViewPhi = 0;
            }
        }
        Display::grabMouse(!dialog && world && !gameInput->paused.get());
        Ui::move(deltaTime);
        if(world)
            world->move(deltaTime, lock_manager);
        if(dialog && dialog->isDone())
        {
            dialog->handleFinish();
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
            float volume = 0.1f;
            if(world)
            {
                PositionF p = playerW.lock()->getPosition();
                float height = p.y;
                float relativeHeight = height / World::SeaLevel;
                playingAudio = audioScheduler.next(world->getTimeOfDayInSeconds(),
                                                   world->dayDurationInSeconds,
                                                   relativeHeight,
                                                   p.d,
                                                   volume);
            }
            else
            {
                playingAudio = mainMenuSong->play(volume);
            }
        }
        if(!world && !generatingWorld && worldGenerateThread.joinable())
        {
            finalizeCreateWorld();
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
    void changeDialog(std::shared_ptr<Ui> nextDialog)
    {
        assert(nextDialog);
        std::unique_lock<std::mutex> lockIt(newDialogLock);
        this->newDialog = nextDialog;
    }
    virtual bool handleTouchUp(TouchUpEvent &event) override
    {
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleTouchUp(event))
            return true;
        if(Ui::handleTouchUp(event))
            return true;
        if(dialog)
            return true;
        if(touches.count(event.touchId) == 0)
            return true;
        TouchStruct &touch = touches.at(event.touchId);
        if(touch.state == TouchStruct::State::Attack)
        {
            if(--pressAttackCount <= 0)
            {
                gameInput->attack.set(false);
                pressAttackCount = 0;
            }
        }
        else if(touch.state == TouchStruct::State::StartDelay)
        {
            EventArguments args;
            gameInput->action(args);
        }
        touches.erase(event.touchId);
        return true;
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleTouchDown(event))
            return true;
        if(Ui::handleTouchDown(event))
            return true;
        if(dialog)
            return true;
        VectorF position = Display::transformTouchTo3D(event.x, event.y);
        if(touches.count(event.touchId))
            return true; // ignore duplicates
        touches.emplace(event.touchId, TouchStruct(position, true));
        return true;
    }
    virtual bool handleTouchMove(TouchMoveEvent &event) override
    {
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleTouchMove(event))
            return true;
        if(Ui::handleTouchMove(event))
            return true;
        if(dialog)
            return true;
        if(gameInput->paused.get())
            return true;
        VectorF position = Display::transformTouchTo3D(event.x, event.y);
        VectorF deltaPos =
            Display::transformTouchTo3D(event.x + event.deltaX, event.y + event.deltaY) - position;
        if(touches.count(event.touchId) == 0)
            touches.emplace(event.touchId, TouchStruct(position, false));
        TouchStruct &touch = touches.at(event.touchId);
        touch.position = position;
        const float thresholdDistance = 0.04f;
        if(absSquared(position - touch.startPosition) >= thresholdDistance * thresholdDistance)
        {
            if(touch.state == TouchStruct::State::Attack)
            {
                if(--pressAttackCount <= 0)
                {
                    gameInput->attack.set(false);
                    pressAttackCount = 0;
                }
            }
            captureTouch(event.touchId);
            touch.state = TouchStruct::State::Move;
        }
        if(touch.state == TouchStruct::State::Move)
        {
            deltaViewTheta += deltaPos.x;
            deltaViewPhi -= deltaPos.y;
        }
        return true;
    }
    virtual bool handleMouseUp(MouseUpEvent &event) override
    {
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleMouseUp(event))
            return true;
        if(Ui::handleMouseUp(event))
            return true;
        if(event.button == MouseButton_Left)
        {
            if(--pressAttackCount <= 0)
            {
                gameInput->attack.set(false);
                pressAttackCount = 0;
            }
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
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleMouseDown(event))
            return true;
        if(Ui::handleMouseDown(event))
            return true;
        if(dialog)
            return true;
        if(event.button == MouseButton_Left)
        {
            if(pressAttackCount++ == 0)
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
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleMouseMove(event))
            return true;
        if(!dialog && !gameInput->paused.get())
        {
            VectorF deltaPos =
                Display::transformMouseTo3D(event.x + event.deltaX, event.y + event.deltaY)
                - Display::transformMouseTo3D(event.x, event.y);
            deltaViewTheta -= deltaPos.x;
            deltaViewPhi += deltaPos.y;
        }
        if(Ui::handleMouseMove(event))
            return true;
        FIXME_MESSAGE(implement)
        return true;
    }
    virtual bool handleMouseScroll(MouseScrollEvent &event) override
    {
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleMouseScroll(event))
            return true;
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
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleKeyUp(event))
            return true;
        if(Ui::handleKeyUp(event))
            return true;
        switch(event.key)
        {
        case KeyboardKey::Space:
        {
            handleJumpUp();
            return true;
        }
        case KeyboardKey::LShift:
        {
            if(--pressSneakCount <= 0)
            {
                gameInput->sneak.set(false);
                pressSneakCount = 0;
            }
            return true;
        }
        case KeyboardKey::W:
        {
            pressForwardCount--;
            if(pressForwardCount < 0)
                pressForwardCount = 0;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::A:
        {
            pressLeftCount--;
            if(pressLeftCount < 0)
                pressLeftCount = 0;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::S:
        {
            pressBackwardCount--;
            if(pressBackwardCount < 0)
                pressBackwardCount = 0;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::D:
        {
            pressRightCount--;
            if(pressRightCount < 0)
                pressRightCount = 0;
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
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleKeyDown(event))
            return true;
        if(Ui::handleKeyDown(event))
            return true;
        if(dialog)
            return false;
        switch(event.key)
        {
        case KeyboardKey::Space:
        {
            if(!event.isRepetition)
            {
                handleJumpDown();
            }
            return true;
        }
        case KeyboardKey::LShift:
        {
            if(pressSneakCount++ == 0)
                gameInput->sneak.set(true);
            return true;
        }
        case KeyboardKey::W:
        {
            if(!event.isRepetition)
                pressForwardCount++;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::A:
        {
            if(!event.isRepetition)
                pressLeftCount++;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::S:
        {
            if(!event.isRepetition)
                pressBackwardCount++;
            calculateMoveDirection();
            return true;
        }
        case KeyboardKey::D:
        {
            if(!event.isRepetition)
                pressRightCount++;
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
            if(world || (!generatingWorld && !generatedWorld))
            {
                startMainMenu();
            }
            return true;
        }
        case KeyboardKey::F12:
        {
            bool isCreativeMode = gameInput->isCreativeMode.get();
            gameInput->isCreativeMode.set(!isCreativeMode);
            if(!isCreativeMode)
            {
                gameInput->fly.set(false);
            }
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
            HotBarSelectEventArguments args((std::size_t)event.key
                                            - (std::size_t)KeyboardKey::Num1);
            gameInput->hotBarSelect(args);
            return true;
        }
        case KeyboardKey::F7:
        case KeyboardKey::F8:
        {
            if(gameInput->paused.get() || !world)
                return false;
            if(viewPoint)
            {
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
            }
            return true;
        }
        default:
            return false;
        }
    }
    virtual bool handleTextInput(TextInputEvent &event) override
    {
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleTextInput(event))
            return true;
        if(Ui::handleTextInput(event))
            return true;
        if(dialog)
            return false;
        FIXME_MESSAGE(implement)
        return false;
    }
    virtual bool handleTextEdit(TextEditEvent &event) override
    {
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleTextEdit(event))
            return true;
        if(Ui::handleTextEdit(event))
            return true;
        if(dialog)
            return false;
        FIXME_MESSAGE(implement)
        return false;
    }
    virtual bool handleMouseMoveOut(MouseEvent &event) override
    {
        if(Ui::handleMouseMoveOut(event))
            return true;
        if(dialog)
            return true;
        return true;
    }
    virtual bool handleMouseMoveIn(MouseEvent &event) override
    {
        if(Ui::handleMouseMoveIn(event))
            return true;
        if(dialog)
            return true;
        return true;
    }
    virtual bool handleTouchMoveOut(TouchEvent &event) override
    {
        if(Ui::handleTouchMoveOut(event))
            return true;
        if(dialog)
            return true;
        return true;
    }
    virtual bool handleTouchMoveIn(TouchEvent &event) override
    {
        if(Ui::handleTouchMoveIn(event))
            return true;
        if(dialog)
            return true;
        return true;
    }
    virtual void reset() override
    {
        resetInput();
        if(!addedUi)
        {
            addedUi = true;
            addUi();
        }
        Ui::reset();
    }
    virtual bool handleQuit(QuitEvent &event) override
    {
        if(virtualRealityCallbacks && virtualRealityCallbacks->handleQuit(event))
            return true;
        return Ui::handleQuit(event);
    }
    virtual bool handlePause(PauseEvent &event) override
    {
        if(virtualRealityCallbacks)
            virtualRealityCallbacks->handlePause(event);
        assert(viewPoint.use_count() <= 1);
        viewPoint = nullptr;
        if(world)
            world->paused(true, lock_manager);
        return Ui::handlePause(event);
    }
    virtual bool handleResume(ResumeEvent &event) override
    {
        bool retval = Ui::handleResume(event);
        if(virtualRealityCallbacks)
            virtualRealityCallbacks->handleResume(event);
        if(world)
            world->paused(false, lock_manager);
        return retval;
    }

protected:
    virtual void clear(Renderer &renderer) override;
};
}
}
}

#endif // GAMEUI_H_INCLUDED
