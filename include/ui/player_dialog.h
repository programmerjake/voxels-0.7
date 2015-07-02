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
#ifndef PLAYER_DIALOG_H_INCLUDED
#define PLAYER_DIALOG_H_INCLUDED

#include "ui/ui.h"
#include "ui/shaded_container.h"
#include "platform/platform.h"
#include "player/player.h"
#include "ui/image.h"
#include "ui/item.h"
#include "ui/button.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class PlayerDialog : public Ui
{
    PlayerDialog(const PlayerDialog &) = delete;
    PlayerDialog &operator =(const PlayerDialog &) = delete;
private:
    class BackgroundElement : public ShadedContainer
    {
    public:
        BackgroundElement()
            : ShadedContainer(-Display::scaleX(), Display::scaleX(), -Display::scaleY(), Display::scaleY())
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
    bool addedElements = false;
    std::shared_ptr<ImageElement> backgroundImageElement;
    WorldLockManager *plock_manager = nullptr;
protected:
    static constexpr int imageXRes()
    {
        return 170;
    }
    static constexpr int imageYRes()
    {
        return 151;
    }
    static constexpr float imageScale()
    {
        return 2 / 240.0f;
    }
    static constexpr float imageWidth()
    {
        return imageScale() * imageXRes();
    }
    static constexpr float imageHeight()
    {
        return imageScale() * imageYRes();
    }
    static constexpr float imageMinX()
    {
        return -0.5f * imageWidth();
    }
    static constexpr float imageMaxX()
    {
        return 0.5f * imageWidth();
    }
    static constexpr float imageMinY()
    {
        return -0.5f * imageHeight();
    }
    static constexpr float imageMaxY()
    {
        return 0.5f * imageHeight();
    }
    static constexpr float imageGetPositionX(float pixelX)
    {
        return imageMinX() + pixelX * imageScale();
    }
    static constexpr float imageGetPositionY(float pixelY)
    {
        return imageMinY() + pixelY * imageScale();
    }
    static constexpr float imageGetPositionX(int pixelX)
    {
        return imageGetPositionX(static_cast<float>(pixelX));
    }
    static constexpr float imageGetPositionY(int pixelY)
    {
        return imageGetPositionY(static_cast<float>(pixelY));
    }
    const std::shared_ptr<Player> player;
    TextureDescriptor backgroundImage;
    std::shared_ptr<UiItem> selectedItem = nullptr;
    bool isTouchSelection = false;
    int selectedTouch = -1;
    float touchHoldTimeLeft = 0;
    VectorF touchStartPosition = VectorF(0);
    bool hasTouchMoved = false;
public:
    PlayerDialog(std::shared_ptr<Player> player, TextureDescriptor backgroundImage)
        : backgroundImageElement(), player(player), backgroundImage(backgroundImage)
    {
    }
protected:
    virtual bool canSelectItemStack(std::shared_ptr<UiItem> item) const
    {
        return item != selectedItem;
    }
    virtual unsigned transferItems(std::shared_ptr<ItemStack> sourceItemStack, std::shared_ptr<ItemStack> destItemStack, unsigned transferCount)
    {
        return destItemStack->transfer(*sourceItemStack, transferCount);
    }
    std::pair<std::shared_ptr<ItemStack>, std::recursive_mutex *> getItemStackFromPosition(VectorF position)
    {
        position /= -position.z;
        for(std::shared_ptr<Element> e : *this)
        {
            if(!e->isPointInside(position.x, position.y))
                continue;
            std::shared_ptr<UiItem> item = std::dynamic_pointer_cast<UiItem>(e);
            if(item == nullptr)
                continue;
            if(!canSelectItemStack(item))
                continue;
            return std::pair<std::shared_ptr<ItemStack>, std::recursive_mutex *>(item->getItemStack(), item->getItemStackLock());
        }
        return std::pair<std::shared_ptr<ItemStack>, std::recursive_mutex *>(nullptr, nullptr);
    }
    virtual void addElements()
    {
    }
public:
    virtual void reset() override
    {
        selectedItem = nullptr;
        selectedTouch = -1;
        isTouchSelection = false;
        touchHoldTimeLeft = 0;
        touchStartPosition = VectorF(0);
        hasTouchMoved = false;
        if(!addedElements)
        {
            addedElements = true;
            add(std::make_shared<BackgroundElement>());
            backgroundImageElement = std::make_shared<ImageElement>(backgroundImage, imageMinX(), imageMaxX(), imageMinY(), imageMaxY());
            add(backgroundImageElement);
            // add hot bar items
            for(int x = 0; x < (int)player->items.itemStacks.size(); x++)
            {
                add(std::make_shared<UiItem>(imageGetPositionX(5 + 18 * x), imageGetPositionX(5 + 18 * x + 16),
                                             imageGetPositionY(5), imageGetPositionY(5 + 16),
                                             std::shared_ptr<ItemStack>(player, &player->items.itemStacks[x][0]), &player->itemsLock));
            }
            // add the rest of the player's items
            for(int x = 0; x < (int)player->items.itemStacks.size(); x++)
            {
                for(int y = 1; y < (int)player->items.itemStacks[0].size(); y++) // start at 1 to skip hotbar
                {
                    add(std::make_shared<UiItem>(imageGetPositionX(5 + 18 * x), imageGetPositionX(5 + 18 * x + 16),
                                                 imageGetPositionY(28 + 18 * ((int)player->items.itemStacks[0].size() - y - 1)), imageGetPositionY(28 + 18 * ((int)player->items.itemStacks[0].size() - y - 1) + 16),
                                                 std::shared_ptr<ItemStack>(player, &player->items.itemStacks[x][y]), &player->itemsLock));
                }
            }
            if(Display::needTouchControls())
            {
                class CloseButton final : public Button
                {
                    CloseButton(const CloseButton &) = delete;
                    CloseButton &operator =(const CloseButton &) = delete;
                private:
                    PlayerDialog *const playerDialog;
                public:
                    CloseButton(PlayerDialog *playerDialog)
                        : Button(L"\u2190",
                                 Display::scaleX() - Display::getTouchControlSize(),
                                 Display::scaleX(),
                                 -Display::scaleY(),
                                 Display::getTouchControlSize() - Display::scaleY()),
                        playerDialog(playerDialog)
                    {
                        click.bind([this](EventArguments &)->Event::ReturnType
                        {
                            this->playerDialog->quit();
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
                add(std::make_shared<CloseButton>(this));
            }
            addElements();
        }
        Ui::reset();
    }
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
        if(plock_manager)
            plock_manager->clear();
        backgroundImageElement->image = backgroundImage;
        Ui::render(renderer, minZ, maxZ, hasFocus);
    }
    virtual bool handleKeyDown(KeyDownEvent &event) override
    {
        if(event.key == KeyboardKey::E || event.key == KeyboardKey::Escape)
        {
            quit();
            return true;
        }
        return Ui::handleKeyDown(event);
    }
    virtual bool handleMouseDown(MouseDownEvent &event) override
    {
        if(Ui::handleMouseDown(event))
            return true;
        if(isTouchSelection)
            return true;
        VectorF position = Display::transformMouseTo3D(event.x, event.y);
        std::pair<std::shared_ptr<ItemStack>, std::recursive_mutex *> itemStack = getItemStackFromPosition(position);
        if(std::get<0>(itemStack) == nullptr)
            return true;
        std::unique_lock<std::recursive_mutex> theLock;
        if(plock_manager)
            plock_manager->clear();
        if(std::get<1>(itemStack) != nullptr)
            theLock = std::unique_lock<std::recursive_mutex>(*std::get<1>(itemStack));
        if(selectedItem == nullptr)
        {
            if(!std::get<0>(itemStack)->good())
                return true;
            std::shared_ptr<Player> player = this->player;
            std::shared_ptr<ItemStack> selectedItemItemStack = std::shared_ptr<ItemStack>(new ItemStack(), [player](ItemStack *itemStack)
            {
                if(itemStack->good())
                {
                    for(std::size_t i = 0; i < itemStack->count; i++)
                    {
                        if(player->addItem(itemStack->item) < 1)
                        {
                            FIXME_MESSAGE(finish)
                        }
                    }
                }
                delete itemStack;
            });
            unsigned transferCount = 0;
            switch(event.button)
            {
            case MouseButton_Left:
                transferCount = std::get<0>(itemStack)->count;
                break;
            case MouseButton_Right:
                transferCount = std::get<0>(itemStack)->count;
                transferCount = transferCount / 2 + transferCount % 2;
                break;
            default:
                break;
            }
            transferItems(std::get<0>(itemStack), selectedItemItemStack, transferCount);
            if(selectedItemItemStack->good())
            {
                selectedItem = std::make_shared<UiItem>(position.x - 8 * imageScale(), position.x + 8 * imageScale(),
                                                        position.y - 8 * imageScale(), position.y + 8 * imageScale(), selectedItemItemStack);
                add(selectedItem);
            }
            return true;
        }
        else
        {
            std::shared_ptr<ItemStack> selectedItemItemStack = selectedItem->getItemStack();
            unsigned transferCount = 0;
            switch(event.button)
            {
            case MouseButton_Left:
                transferCount = selectedItemItemStack->count;
                break;
            case MouseButton_Right:
                transferCount = 1;
                break;
            default:
                break;
            }
            transferItems(selectedItemItemStack, std::get<0>(itemStack), transferCount);
            if(!selectedItemItemStack->good())
            {
                remove(selectedItem);
                selectedItem = nullptr;
            }
        }
        return true;
    }
    virtual bool handleMouseMove(MouseMoveEvent &event) override
    {
        if(selectedItem && !isTouchSelection)
        {
            VectorF position = Display::transformMouseTo3D(event.x, event.y);
            selectedItem->moveCenterTo(position.x, position.y);
        }
        return Ui::handleMouseMove(event);
    }
    virtual void setWorldAndLockManager(World &world, WorldLockManager &lock_manager)
    {
        plock_manager = &lock_manager;
    }
protected:
    void handleTouch(VectorF position, bool isLongPress)
    {
        if(selectedItem && !isTouchSelection)
            return;
        std::pair<std::shared_ptr<ItemStack>, std::recursive_mutex *> itemStack = getItemStackFromPosition(position);
        if(std::get<0>(itemStack) == nullptr)
            return;
        std::unique_lock<std::recursive_mutex> theLock;
        if(plock_manager)
            plock_manager->clear();
        if(std::get<1>(itemStack) != nullptr)
            theLock = std::unique_lock<std::recursive_mutex>(*std::get<1>(itemStack));
        if(selectedItem == nullptr)
        {
            if(!std::get<0>(itemStack)->good())
                return;
            std::shared_ptr<Player> player = this->player;
            std::shared_ptr<ItemStack> selectedItemItemStack = std::shared_ptr<ItemStack>(new ItemStack(), [player](ItemStack *itemStack)
            {
                if(itemStack->good())
                {
                    for(std::size_t i = 0; i < itemStack->count; i++)
                    {
                        if(player->addItem(itemStack->item) < 1)
                        {
                            FIXME_MESSAGE(finish)
                        }
                    }
                }
                delete itemStack;
            });
            unsigned transferCount = 0;
            if(isLongPress)
            {
                transferCount = std::get<0>(itemStack)->count;
            }
            else
            {
                transferCount = 1;
            }
            transferItems(std::get<0>(itemStack), selectedItemItemStack, transferCount);
            if(selectedItemItemStack->good())
            {
                selectedItem = std::make_shared<UiItem>(-16 * imageScale(), 16 * imageScale(),
                                                        -16 * imageScale(), 16 * imageScale(), selectedItemItemStack);
                selectedItem->moveTopLeftTo(getParent()->minX, getParent()->maxY);
                isTouchSelection = true;
                add(selectedItem);
            }
        }
        else
        {
            std::shared_ptr<ItemStack> selectedItemItemStack = selectedItem->getItemStack();
            unsigned transferCount = 0;
            if(isLongPress)
            {
                transferCount = selectedItemItemStack->count;
            }
            else
            {
                transferCount = 1;
            }
            transferItems(selectedItemItemStack, std::get<0>(itemStack), transferCount);
            if(!selectedItemItemStack->good())
            {
                remove(selectedItem);
                selectedItem = nullptr;
                isTouchSelection = false;
            }
        }
    }
public:
    virtual bool handleTouchUp(TouchUpEvent &event) override
    {
        if(Ui::handleTouchUp(event))
            return true;
        if(event.touchId == selectedTouch)
        {
            selectedTouch = -1;
            if(!hasTouchMoved)
                handleTouch(touchStartPosition, touchHoldTimeLeft <= 0);
        }
        return true;
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        if(Ui::handleTouchDown(event))
            return true;
        if(selectedTouch != -1 || (selectedItem != nullptr && !isTouchSelection))
            return true;
        VectorF position = Display::transformTouchTo3D(event.x, event.y);
        selectedTouch = event.touchId;
        captureTouch(event.touchId);
        touchHoldTimeLeft = 0.5f;
        touchStartPosition = position;
        hasTouchMoved = false;
        return true;
    }
    virtual void move(double deltaTime) override
    {
        touchHoldTimeLeft -= static_cast<float>(deltaTime);
        if(touchHoldTimeLeft <= 0)
            touchHoldTimeLeft = 0;
        Ui::move(deltaTime);
    }
    virtual bool handleTouchMove(TouchMoveEvent &event) override
    {
        if(Ui::handleTouchMove(event))
            return true;
        if(event.touchId == selectedTouch)
        {
            VectorF position = Display::transformTouchTo3D(event.x, event.y);
            const float thresholdDistance = 0.04f;
            if(absSquared(position - touchStartPosition) >= thresholdDistance * thresholdDistance)
                hasTouchMoved = true;
        }
        return true;
    }
    virtual void layout() override
    {
        Ui::layout();
        if(selectedItem && isTouchSelection)
        {
            selectedItem->moveTopLeftTo(getParent()->minX, getParent()->maxY);
        }
    }
};
}
}
}

#endif // PLAYER_DIALOG_H_INCLUDED
