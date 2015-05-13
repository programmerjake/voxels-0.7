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
#ifndef PLAYER_DIALOG_H_INCLUDED
#define PLAYER_DIALOG_H_INCLUDED

#include "ui/ui.h"
#include "ui/shaded_container.h"
#include "platform/platform.h"
#include "player/player.h"
#include "ui/image.h"
#include "ui/item.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class PlayerDialog : public Ui
{
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
protected:
    static constexpr int imageXRes = 170;
    static constexpr int imageYRes = 151;
    static constexpr float imageScale = 2 / 240.0f;
    static constexpr float imageWidth = imageScale * imageXRes;
    static constexpr float imageHeight = imageScale * imageYRes;
    static constexpr float imageMinX = -0.5f * imageWidth;
    static constexpr float imageMaxX = 0.5f * imageWidth;
    static constexpr float imageMinY = -0.5f * imageHeight;
    static constexpr float imageMaxY = 0.5f * imageHeight;
    static constexpr float imageGetPositionX(float pixelX)
    {
        return imageMinX + pixelX * imageScale;
    }
    static constexpr float imageGetPositionY(float pixelY)
    {
        return imageMinY + pixelY * imageScale;
    }
    const std::shared_ptr<Player> player;
    TextureDescriptor backgroundImage;
    std::shared_ptr<UiItem> selectedItem = nullptr;
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
        if(!addedElements)
        {
            addedElements = true;
            add(std::make_shared<BackgroundElement>());
            backgroundImageElement = std::make_shared<ImageElement>(backgroundImage, imageMinX, imageMaxX, imageMinY, imageMaxY);
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
            addElements();
        }
        Ui::reset();
    }
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
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
        VectorF position = Display::transformMouseTo3D(event.x, event.y);
        std::pair<std::shared_ptr<ItemStack>, std::recursive_mutex *> itemStack = getItemStackFromPosition(position);
        if(std::get<0>(itemStack) == nullptr)
            return false;
        std::unique_lock<std::recursive_mutex> theLock;
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
                            #warning finish
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
                selectedItem = std::make_shared<UiItem>(position.x - 8 * imageScale, position.x + 8 * imageScale,
                                                        position.y - 8 * imageScale, position.y + 8 * imageScale, selectedItemItemStack);
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
        if(selectedItem)
        {
            VectorF position = Display::transformMouseTo3D(event.x, event.y);
            selectedItem->moveCenterTo(position.x, position.y);
        }
        return Ui::handleMouseMove(event);
    }
    virtual void setWorldAndLockManager(World &world, WorldLockManager &lock_manager)
    {
    }
};
}
}
}

#endif // PLAYER_DIALOG_H_INCLUDED
