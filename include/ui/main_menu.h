/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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
#ifndef MAIN_MENU_H_INCLUDED
#define MAIN_MENU_H_INCLUDED

#include "ui/gameui.h"
#include "ui/shaded_container.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class MainMenu final : public Ui
{
private:
    class BackgroundElement : public ShadedContainer
    {
    public:
        BackgroundElement()
            : ShadedContainer(
                  -Display::scaleX(), Display::scaleX(), -Display::scaleY(), Display::scaleY())
        {
        }
        virtual void layout() override
        {
            minX = -Display::scaleX();
            maxX = Display::scaleX();
            minY = -Display::scaleY();
            maxY = Display::scaleY();
            ShadedContainer::layout();
        }
    };
    std::shared_ptr<Element> setupMainMenu();
    std::shared_ptr<Element> setupSettingsMenu();
    std::shared_ptr<Element> setupBackgroundCameraSelectionMenu();
    enum class MenuState
    {
        MainMenu_,
        SettingsMenu,
        BackgroundCameraSelectionMenu
    };
    MenuState menuState = MenuState::MainMenu_;
    bool needChangeMenuState = false;

public:
    virtual void reset() override
    {
        menuState = MenuState::MainMenu_;
        needChangeMenuState = false;
        std::shared_ptr<Element> focusedElement = setupMainMenu();
        Ui::reset();
        setFocus(focusedElement);
    }
    virtual bool handleQuit(QuitEvent &event) override
    {
        get(shared_from_this())->quit();
        return Ui::handleQuit(event);
    }
    virtual void handleFinish() override
    {
        std::vector<std::shared_ptr<Element>> elements(begin(), end());
        for(std::shared_ptr<Element> e : elements)
        {
            remove(e);
        }
        Ui::handleFinish();
    }
    virtual void move(double deltaTime) override
    {
        if(needChangeMenuState)
        {
            needChangeMenuState = false;
            std::vector<std::shared_ptr<Element>> elements(begin(), end());
            for(std::shared_ptr<Element> e : elements)
            {
                remove(e);
            }
            switch(menuState)
            {
            case MenuState::MainMenu_:
                setFocus(setupMainMenu());
                break;
            case MenuState::SettingsMenu:
                setFocus(setupSettingsMenu());
                break;
            case MenuState::BackgroundCameraSelectionMenu:
                setFocus(setupBackgroundCameraSelectionMenu());
                break;
            }
        }
        Ui::move(deltaTime);
    }
};
}
}
}

#endif // MAIN_MENU_H_INCLUDED
