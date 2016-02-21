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
#include "ui/main_menu.h"
#include "ui/button.h"
#include "ui/checkbox.h"
#include "platform/platform.h"
#include "stream/stream.h"
#include "util/logging.h"
#include "ui/label.h"
#include "ui/shaded_container.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
std::shared_ptr<Element> MainMenu::setupMainMenu()
{
    std::wstring fileName = L"VoxelsGame.vw";
    add(std::make_shared<BackgroundElement>());
    std::shared_ptr<GameUi> gameUi = std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
    std::shared_ptr<Button> newWorldButton =
        std::make_shared<Button>(L"New World", -0.8f, 0.8f, 0.7f, 0.95f);
    add(newWorldButton);
    newWorldButton->click.bind([this](EventArguments &) -> Event::ReturnType
                               {
                                   std::shared_ptr<GameUi> gameUi =
                                       std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
                                   quit();
                                   gameUi->clearWorld();
                                   gameUi->createNewWorld();
                                   return Event::ReturnType::Propagate;
                               });
    std::shared_ptr<Button> loadWorldButton =
        std::make_shared<Button>(L"Load World", -0.8f, 0.8f, 0.4f, 0.65f);
    add(loadWorldButton);
    loadWorldButton->click.bind([this, fileName](EventArguments &) -> Event::ReturnType
                                {
                                    std::shared_ptr<GameUi> gameUi =
                                        std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
                                    quit();
                                    gameUi->clearWorld();
                                    gameUi->loadWorld(fileName);
                                    return Event::ReturnType::Propagate;
                                });
    std::shared_ptr<Element> focusedElement = newWorldButton;
    if(gameUi->world)
    {
        std::shared_ptr<Button> returnToWorldButton =
            std::make_shared<Button>(L"Return To World", -0.8f, 0.8f, 0.1f, 0.35f);
        add(returnToWorldButton);
        returnToWorldButton->click.bind([this](EventArguments &) -> Event::ReturnType
                                        {
                                            std::shared_ptr<GameUi> gameUi =
                                                std::dynamic_pointer_cast<GameUi>(
                                                    get(shared_from_this()));
                                            quit();
                                            gameUi->gameInput->paused.set(false);
                                            return Event::ReturnType::Propagate;
                                        });
        std::shared_ptr<Button> saveWorldButton =
            std::make_shared<Button>(L"Save World", -0.8f, 0.8f, -0.2f, 0.05f);
        add(saveWorldButton);
        saveWorldButton->click.bind(
            [this, fileName](EventArguments &) -> Event::ReturnType
            {
                std::shared_ptr<GameUi> gameUi =
                    std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
                try
                {
                    stream::MemoryWriter writer;
                    gameUi->world->write(writer, gameUi->lock_manager);
                    auto pfwriter = createOrWriteUserSpecificFile(fileName);
                    pfwriter->writeBytes(writer.getBuffer().data(), writer.getBuffer().size());
                }
                catch(stream::IOException &e)
                {
                    getDebugLog() << L"Save Error : " << e.what() << postnl;
                }
                return Event::ReturnType::Propagate;
            });
        focusedElement = returnToWorldButton;
    }
    std::shared_ptr<Button> settingsButton =
        std::make_shared<Button>(L"Settings", -0.8f, 0.8f, -0.65f, -0.4f);
    add(settingsButton);
    settingsButton->click.bind([this](EventArguments &) -> Event::ReturnType
                               {
                                   menuState = MenuState::SettingsMenu;
                                   needChangeMenuState = true;
                                   return Event::ReturnType::Propagate;
                               });
    std::shared_ptr<Button> quitButton =
        std::make_shared<Button>(L"Quit", -0.8f, 0.8f, -0.95f, -0.7f);
    add(quitButton);
    quitButton->click.bind([this](EventArguments &) -> Event::ReturnType
                           {
                               get(shared_from_this())->quit();
                               return Event::ReturnType::Propagate;
                           });
    return focusedElement;
}

std::shared_ptr<Element> MainMenu::setupSettingsMenu()
{
    add(std::make_shared<BackgroundElement>());
    std::shared_ptr<GameUi> gameUi = std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
    GameUi &gameUiR = *gameUi;
#ifdef DEBUG_VERSION
    FIXME_MESSAGE(testing vector font)
    Text::TextProperties vectorFont = Text::defaultTextProperties;
    vectorFont.font = Text::getVectorFont();
    std::wstring demoString =
        L"\u2611\u263A\u263B\u2665\u2666\u2663\u2660\u2022\u25D8\u25CB\u25D9\u2642\u2640\u266A"
        L"\u266B\u263C\n";
    demoString +=
        L"\u25BA\u25C4\u2195\u203C\u00B6\u00A7\u25AC\u21A8\u2191\u2193\u2192\u2190\u221F\u2194"
        L"\u25B2\u25BC\n";
    demoString += L" !\"#$%&\'()*+,-./\n";
    demoString += L"0123456789:;<=>\?\n";
    demoString += L"@ABCDEFGHIJKLMNO\n";
    demoString += L"PQRSTUVWXYZ[\\]^_\n";
    demoString += L"`abcdefghijklmno\n";
    demoString += L"pqrstuvwxyz{|}~\u2302\n";
    demoString +=
        L"\u00C7\u00FC\u00E9\u00E2\u00E4\u00E0\u00E5\u00E7\u00EA\u00EB\u00E8\u00EF\u00EE\u00EC"
        L"\u00C4\u00C5\n";
    demoString +=
        L"\u00C9\u00E6\u00C6\u00F4\u00F6\u00F2\u00FB\u00F9\u00FF\u00D6\u00DC\u00A2\u00A3\u00A5"
        L"\u20A7\u0192\n";
    demoString +=
        L"\u00E1\u00ED\u00F3\u00FA\u00F1\u00D1\u00AA\u00BA\u00BF\u2310\u00AC\u00BD\u00BC\u00A1"
        L"\u00AB\u00BB\n";
    demoString +=
        L"\u2591\u2592\u2593\u2502\u2524\u2561\u2562\u2556\u2555\u2563\u2551\u2557\u255D\u255C"
        L"\u255B\u2510\n";
    demoString +=
        L"\u2514\u2534\u252C\u251C\u2500\u253C\u255E\u255F\u255A\u2554\u2569\u2566\u2560\u2550"
        L"\u256C\u2567\n";
    demoString +=
        L"\u2568\u2564\u2565\u2559\u2558\u2552\u2553\u256B\u256A\u2518\u250C\u2588\u2584\u258C"
        L"\u2590\u2580\n";
    demoString +=
        L"\u03B1\u00DF\u0393\u03C0\u03A3\u03C3\u00B5\u03C4\u03A6\u0398\u03A9\u03B4\u221E\u03C6"
        L"\u03B5\u2229\n";
    demoString +=
        L"\u2261\u00B1\u2265\u2264\u2320\u2321\u00F7\u2248\u00B0\u2219\u00B7\u221A\u207F\u00B2"
        L"\u25A0\u2610";
    std::shared_ptr<Container> textContainer =
        std::make_shared<ShadedContainer>(-1.05f, 1.05f, 0.3f, 1.0f, GrayscaleAF(0.0f, 0.75f));
    textContainer->add(
        std::make_shared<Label>(demoString, -1.0f, -0.1f, 0.35f, 0.95f, GrayscaleF(0.5f)));
    textContainer->add(std::make_shared<Label>(
        demoString, 0.1f, 1.0f, 0.35f, 0.95f, RGBF(0.0f, 1.0f, 1.0f), vectorFont));
    textContainer->add(std::make_shared<Label>(
        demoString, 0.1f, 1.0f, 0.35f, 0.95f, RGBAF(1.0f, 0.0f, 0.0f, 0.5f), vectorFont));
    add(textContainer);
#endif
    std::shared_ptr<CheckBox> creativeModeCheckBox =
        std::make_shared<CheckBox>(L"Creative Mode", -0.8f, 0.8f, -0.05f, 0.2f);
    add(creativeModeCheckBox);
    CheckBox &creativeModeCheckBoxR = *creativeModeCheckBox;
    auto gameInput = gameUi->gameInput;
    auto creativeMode = std::shared_ptr<MonitoredBool>(gameInput, &gameInput->isCreativeMode);
    creativeModeCheckBox->checked.set(creativeMode->cget());
    creativeModeCheckBox->checked.onChange.bind(
        [&creativeModeCheckBoxR, creativeMode](EventArguments &) -> Event::ReturnType
        {
            creativeMode->set(creativeModeCheckBoxR.checked.cget());
            return Event::ReturnType::Propagate;
        });
    std::shared_ptr<CheckBox> backgroundCameraCheckBox =
        std::make_shared<CheckBox>(L"Background Camera", -0.8f, 0.8f, -0.35f, -0.1f);
    add(backgroundCameraCheckBox);
    CheckBox &backgroundCameraCheckBoxR = *backgroundCameraCheckBox;
    backgroundCameraCheckBox->checked.set(gameUi->hasBackgroundCamera());
    backgroundCameraCheckBox->checked.onChange.bind(
        [&backgroundCameraCheckBoxR, &gameUiR, this](EventArguments &) -> Event::ReturnType
        {
            class ReentryProtector
            {
                ReentryProtector(const ReentryProtector &) = delete;
                ReentryProtector &operator=(const ReentryProtector &) = delete;

            private:
                static bool &getReentryFlag()
                {
                    struct RetvalTag
                    {
                    };
                    thread_local_variable<bool, RetvalTag> retval(TLS::getSlow(), false);
                    return retval.get();
                }

            public:
                const bool isReentry;
                ReentryProtector() : isReentry(getReentryFlag())
                {
                    getReentryFlag() = true;
                }
                ~ReentryProtector()
                {
                    getReentryFlag() = isReentry;
                }
            };
            ReentryProtector reentryProtector;
            if(reentryProtector.isReentry)
                return Event::ReturnType::Propagate;
            if(backgroundCameraCheckBoxR.checked.get())
            {
                const std::vector<const VideoInputDevice *> &videoInputDevices =
                    getVideoInputDeviceList();
                std::unique_ptr<VideoInput> videoInput;
                if(videoInputDevices.size() == 1)
                    videoInput = videoInputDevices.front()->makeVideoInput();
                else if(videoInputDevices.size() > 1)
                {
                    menuState = MenuState::BackgroundCameraSelectionMenu;
                    needChangeMenuState = true;
                    return Event::ReturnType::Propagate;
                }
                if(videoInput)
                    gameUiR.setBackgroundCamera(std::move(videoInput));
                else
                    backgroundCameraCheckBoxR.checked.set(false);
            }
            else
            {
                gameUiR.setBackgroundCamera(nullptr);
            }
            return Event::ReturnType::Propagate;
        });
    std::shared_ptr<CheckBox> fullScreenCheckBox =
        std::make_shared<CheckBox>(L"Full Screen", -0.8f, 0.8f, -0.65f, -0.4f);
    add(fullScreenCheckBox);
    CheckBox &fullScreenCheckBoxR = *fullScreenCheckBox;
    fullScreenCheckBox->checked.set(Display::fullScreen());
    fullScreenCheckBox->checked.onChange.bind(
        [&fullScreenCheckBoxR](EventArguments &) -> Event::ReturnType
        {
            Display::fullScreen(fullScreenCheckBoxR.checked.get());
            return Event::ReturnType::Propagate;
        });
    std::shared_ptr<Button> backButton =
        std::make_shared<Button>(L"Back", -0.8f, 0.8f, -0.95f, -0.7f);
    add(backButton);
    backButton->click.bind([this](EventArguments &) -> Event::ReturnType
                           {
                               menuState = MenuState::MainMenu_;
                               needChangeMenuState = true;
                               return Event::ReturnType::Propagate;
                           });
    return backButton;
}

std::shared_ptr<Element> MainMenu::setupBackgroundCameraSelectionMenu()
{
    add(std::make_shared<BackgroundElement>());
    std::shared_ptr<GameUi> gameUi = std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
    GameUi &gameUiR = *gameUi;
    float topY = 0.95f;
    for(const VideoInputDevice *videoInputDevice : getVideoInputDeviceList())
    {
        std::wstring name = videoInputDevice->name;
        std::shared_ptr<Button> cameraButton =
            std::make_shared<Button>(name, -0.8f, 0.8f, topY - 0.15f, topY);
        topY -= 0.2f;
        add(cameraButton);
        cameraButton->click.bind(
            [&gameUiR, videoInputDevice, this](EventArguments &) -> Event::ReturnType
            {
                gameUiR.setBackgroundCamera(videoInputDevice->makeVideoInput());
                menuState = MenuState::SettingsMenu;
                needChangeMenuState = true;
                return Event::ReturnType::Propagate;
            });
    }
    std::shared_ptr<Button> backButton =
        std::make_shared<Button>(L"Back", -0.8f, 0.8f, -0.95f, -0.7f);
    add(backButton);
    backButton->click.bind([this](EventArguments &) -> Event::ReturnType
                           {
                               menuState = MenuState::SettingsMenu;
                               needChangeMenuState = true;
                               return Event::ReturnType::Propagate;
                           });
    return backButton;
}
}
}
}
