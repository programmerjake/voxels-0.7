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
#ifndef UI_ITEM_H_INCLUDED
#define UI_ITEM_H_INCLUDED

#include "ui/element.h"
#include "item/item.h"
#include "util/util.h"
#include <cassert>
#include <sstream>
#include "render/text.h"
#include "texture/texture_atlas.h"
#include <mutex>

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class UiItem : public Element
{
    UiItem(const UiItem &) = delete;
    UiItem operator=(const UiItem &) = delete;

private:
    std::shared_ptr<ItemStack> itemStack;
    std::recursive_mutex *itemStackLock;

public:
    std::shared_ptr<ItemStack> getItemStack() const
    {
        return itemStack;
    }
    std::recursive_mutex *getItemStackLock() const
    {
        return itemStackLock;
    }
    UiItem(float minX,
           float maxX,
           float minY,
           float maxY,
           std::shared_ptr<ItemStack> itemStack,
           std::recursive_mutex *itemStackLock = nullptr)
        : Element(minX, maxX, minY, maxY), itemStack(itemStack), itemStackLock(itemStackLock)
    {
    }

protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
        float backgroundZ = interpolate<float>(0.95f, minZ, maxZ);
        assert(backgroundZ / minZ > ItemDescriptor::maxRenderZ / ItemDescriptor::minRenderZ);
        float scale = backgroundZ / ItemDescriptor::maxRenderZ;
        std::unique_lock<std::recursive_mutex> theLock;
        if(itemStackLock)
            theLock = std::unique_lock<std::recursive_mutex>(*itemStackLock);
        ItemStack is = *itemStack;
        if(itemStackLock)
            theLock.unlock();
        if(!is.good())
            return;
        Item item = is.item;
        Mesh dest;
        item.descriptor->render(item, dest, minX, maxX, minY, maxY);
        renderer << RenderLayer::Opaque << transform(Transform::scale(scale), dest);
        if(is.count > 1)
        {
            std::wostringstream os;
            os << L"\n";
            os << (unsigned)is.count;
            std::wstring text = os.str();
            Text::TextProperties textProperties = Text::defaultTextProperties;
            ColorF textColor = RGBF(1, 1, 1);
            float textWidth = Text::width(text, textProperties);
            float textHeight = Text::height(text, textProperties);
            if(textWidth == 0)
                textWidth = 1;
            if(textHeight == 0)
                textHeight = 1;
            float textScale = (maxY - minY) / textHeight;
            textScale = std::min<float>(textScale, (maxX - minX) / textWidth);
            float xOffset = -1 * textWidth, yOffset = 0 * textHeight;
            xOffset = textScale * xOffset + maxX;
            yOffset = textScale * yOffset + minY;
            renderer << transform(Transform::scale(textScale)
                                      .concat(Transform::translate(xOffset, yOffset, -1))
                                      .concat(Transform::scale(minZ)),
                                  Text::mesh(text, textColor, textProperties));
        }
    }
};

class UiItemWithBorder : public Element
{
    UiItemWithBorder(const UiItemWithBorder &) = delete;
    UiItemWithBorder &operator=(const UiItemWithBorder &) = delete;

private:
    std::shared_ptr<ItemStack> itemStack;
    std::recursive_mutex *itemStackLock;

public:
    std::shared_ptr<ItemStack> getItemStack() const
    {
        return itemStack;
    }
    std::recursive_mutex *getItemStackLock() const
    {
        return itemStackLock;
    }
    std::function<bool()> isSelected;
    UiItemWithBorder(float minX,
                     float maxX,
                     float minY,
                     float maxY,
                     std::shared_ptr<ItemStack> itemStack,
                     std::function<bool()> isSelected,
                     std::recursive_mutex *itemStackLock = nullptr)
        : Element(minX, maxX, minY, maxY),
          itemStack(itemStack),
          itemStackLock(itemStackLock),
          isSelected(isSelected)
    {
    }

protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
        float borderZ = interpolate<float>(0.975f, minZ, maxZ);
        ColorF backgroundColor = RGBF(0.5f, 0.5f, 0.5f);
        if(isSelected != nullptr && isSelected())
            backgroundColor = RGBF(0.5f, 1.0f, 0.5f);
        renderer << Generate::quadrilateral(TextureAtlas::HotBarBox.td(),
                                            VectorF(minX * borderZ, minY * borderZ, -borderZ),
                                            backgroundColor,
                                            VectorF(maxX * borderZ, minY * borderZ, -borderZ),
                                            backgroundColor,
                                            VectorF(maxX * borderZ, maxY * borderZ, -borderZ),
                                            backgroundColor,
                                            VectorF(minX * borderZ, maxY * borderZ, -borderZ),
                                            backgroundColor);
        float centerX = 0.5f * (minX + maxX);
        float centerY = 0.5f * (minY + maxY);
        float width = maxX - minX;
        float height = maxY - minY;
        float innerXOffset = (0.5f * 16.0f / 20.0f) * width;
        float innerYOffset = (0.5f * 16.0f / 20.0f) * height;
        float innerMinX = centerX - innerXOffset;
        float innerMaxX = centerX + innerXOffset;
        float innerMinY = centerY - innerYOffset;
        float innerMaxY = centerY + innerYOffset;
        float backgroundZ = interpolate<float>(0.95f, minZ, maxZ);
        assert(backgroundZ / minZ > ItemDescriptor::maxRenderZ / ItemDescriptor::minRenderZ);
        float scale = backgroundZ / ItemDescriptor::maxRenderZ;
        std::unique_lock<std::recursive_mutex> theLock;
        if(itemStackLock)
            theLock = std::unique_lock<std::recursive_mutex>(*itemStackLock);
        ItemStack is = *itemStack;
        if(itemStackLock)
            theLock.unlock();
        if(!is.good())
            return;
        Item item = is.item;
        Mesh dest;
        item.descriptor->render(item, dest, innerMinX, innerMaxX, innerMinY, innerMaxY);
        renderer << RenderLayer::Opaque << transform(Transform::scale(scale), dest);
        if(is.count > 1)
        {
            std::wostringstream os;
            os << L"\n";
            os << L"\n";
            os << (unsigned)is.count;
            std::wstring text = os.str();
            Text::TextProperties textProperties = Text::defaultTextProperties;
            ColorF textColor = RGBF(1, 1, 1);
            float textWidth = Text::width(text, textProperties);
            float textHeight = Text::height(text, textProperties);
            if(textWidth == 0)
                textWidth = 1;
            if(textHeight == 0)
                textHeight = 1;
            float textScale = (innerMaxY - innerMinY) / textHeight;
            textScale = std::min<float>(textScale, (innerMaxX - innerMinX) / textWidth);
            float xOffset = -1 * textWidth, yOffset = 0 * textHeight;
            xOffset = textScale * xOffset + innerMaxX;
            yOffset = textScale * yOffset + innerMinY;
            renderer << transform(Transform::scale(textScale)
                                      .concat(Transform::translate(xOffset, yOffset, -1))
                                      .concat(Transform::scale(minZ)),
                                  Text::mesh(text, textColor, textProperties));
        }
    }
};

class HotBarItem : public UiItemWithBorder
{
private:
    std::function<void()> onSelect;

public:
    HotBarItem(float minX,
               float maxX,
               float minY,
               float maxY,
               std::shared_ptr<ItemStack> itemStack,
               std::function<bool()> isSelected,
               std::function<void()> onSelect,
               std::recursive_mutex *itemStackLock = nullptr)
        : UiItemWithBorder(minX, maxX, minY, maxY, itemStack, isSelected, itemStackLock),
          onSelect(onSelect)
    {
    }
    virtual void layout() override
    {
        moveBy(0, getParent()->minY - minY);
        UiItemWithBorder::layout();
    }
    virtual bool handleTouchDown(TouchDownEvent &event) override
    {
        onSelect();
        ignore_unused_variable_warning(event);
        return true;
    }
};
}
}
}

#endif // UI_ITEM_H_INCLUDED
