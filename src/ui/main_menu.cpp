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
#include "ui/main_menu.h"
#include "ui/button.h"
#include "ui/checkbox.h"
#include "platform/platform.h"
#include "stream/stream.h"
#include "util/logging.h"

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
    std::shared_ptr<Button> newWorldButton = std::make_shared<Button>(L"New World", -0.8f, 0.8f, 0.7f, 0.95f);
    add(newWorldButton);
    newWorldButton->click.bind([this](EventArguments &)->Event::ReturnType
    {
        std::shared_ptr<GameUi> gameUi = std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
        quit();
        gameUi->clearWorld();
        gameUi->createNewWorld();
        return Event::ReturnType::Propagate;
    });
    std::shared_ptr<Button> loadWorldButton = std::make_shared<Button>(L"Load World", -0.8f, 0.8f, 0.4f, 0.65f);
    add(loadWorldButton);
    loadWorldButton->click.bind([this, fileName](EventArguments &)->Event::ReturnType
    {
        std::shared_ptr<GameUi> gameUi = std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
        quit();
        gameUi->clearWorld();
        gameUi->loadWorld(fileName);
        return Event::ReturnType::Propagate;
    });
    std::shared_ptr<Element> focusedElement = newWorldButton;
    if(gameUi->world)
    {
        std::shared_ptr<Button> returnToWorldButton = std::make_shared<Button>(L"Return To World", -0.8f, 0.8f, 0.1f, 0.35f);
        add(returnToWorldButton);
        returnToWorldButton->click.bind([this](EventArguments &)->Event::ReturnType
        {
            std::shared_ptr<GameUi> gameUi = std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
            quit();
            gameUi->gameInput->paused.set(false);
            return Event::ReturnType::Propagate;
        });
        std::shared_ptr<Button> saveWorldButton = std::make_shared<Button>(L"Save World", -0.8f, 0.8f, -0.2f, 0.05f);
        add(saveWorldButton);
        saveWorldButton->click.bind([this, fileName](EventArguments &)->Event::ReturnType
        {
            std::shared_ptr<GameUi> gameUi = std::dynamic_pointer_cast<GameUi>(get(shared_from_this()));
            try
            {
                stream::MemoryWriter writer;
                gameUi->world->write(writer, gameUi->lock_manager);
                stream::FileWriter fwriter(fileName);
                fwriter.writeBytes(writer.getBuffer().data(), writer.getBuffer().size());
            }
            catch(stream::IOException &e)
            {
                getDebugLog() << L"Save Error : " << e.what() << postnl;
            }
            return Event::ReturnType::Propagate;
        });
        focusedElement = returnToWorldButton;
    }
    std::shared_ptr<Button> settingsButton = std::make_shared<Button>(L"Settings", -0.8f, 0.8f, -0.65f, -0.4f);
    add(settingsButton);
    settingsButton->click.bind([this](EventArguments &)->Event::ReturnType
    {
        menuState = MenuState::SettingsMenu;
        needChangeMenuState = true;
        return Event::ReturnType::Propagate;
    });
    std::shared_ptr<Button> quitButton = std::make_shared<Button>(L"Quit", -0.8f, 0.8f, -0.95f, -0.7f);
    add(quitButton);
    quitButton->click.bind([this](EventArguments &)->Event::ReturnType
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
    std::shared_ptr<CheckBox> fullScreenCheckBox = std::make_shared<CheckBox>(L"Full Screen", -0.8f, 0.8f, -0.65f, -0.4f);
    add(fullScreenCheckBox);
    MonitoredBool &fullScreenCheckBoxChecked = fullScreenCheckBox->checked;
    fullScreenCheckBoxChecked.set(Display::fullScreen());
    fullScreenCheckBox->checked.onChange.bind([&fullScreenCheckBoxChecked](EventArguments &)->Event::ReturnType
    {
        Display::fullScreen(fullScreenCheckBoxChecked.get());
        return Event::ReturnType::Propagate;
    });
    std::shared_ptr<Button> backButton = std::make_shared<Button>(L"Back", -0.8f, 0.8f, -0.95f, -0.7f);
    add(backButton);
    backButton->click.bind([this](EventArguments &)->Event::ReturnType
    {
        menuState = MenuState::MainMenu;
        needChangeMenuState = true;
        return Event::ReturnType::Propagate;
    });
    return backButton;
}
}
}
}
