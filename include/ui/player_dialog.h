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
        : player(player), backgroundImage(backgroundImage)
    {
    }
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
                                             std::shared_ptr<ItemStack>(player, &player->items.itemStacks[x][0])));
            }
            // add the rest of the player's items
            for(int x = 0; x < (int)player->items.itemStacks.size(); x++)
            {
                for(int y = 1; y < (int)player->items.itemStacks[0].size(); y++) // start at 1 to skip hotbar
                {
                    add(std::make_shared<UiItem>(imageGetPositionX(5 + 18 * x), imageGetPositionX(5 + 18 * x + 16),
                                                 imageGetPositionY(28 + 18 * ((int)player->items.itemStacks[0].size() - y - 1)), imageGetPositionY(28 + 18 * ((int)player->items.itemStacks[0].size() - y - 1) + 16),
                                                 std::shared_ptr<ItemStack>(player, &player->items.itemStacks[x][y])));
                }
            }
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
        if(event.key == KeyboardKey::Q || event.key == KeyboardKey::Escape)
        {
            quit();
            return true;
        }
        return Ui::handleKeyDown(event);
    }
};
}
}
}

#endif // PLAYER_DIALOG_H_INCLUDED
