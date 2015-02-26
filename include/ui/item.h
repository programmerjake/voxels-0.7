#ifndef UI_ITEM_H_INCLUDED
#define UI_ITEM_H_INCLUDED

#include "ui/element.h"
#include "item/item.h"
#include "util/util.h"
#include <cassert>
#include <sstream>
#include "render/text.h"
#include "texture/texture_atlas.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
class UiItem : public Element
{
private:
    std::shared_ptr<ItemStack> itemStack;
public:
    std::shared_ptr<ItemStack> getItemStack() const
    {
        return itemStack;
    }
    UiItem(float minX, float maxX, float minY, float maxY, std::shared_ptr<ItemStack> itemStack)
        : Element(minX, maxX, minY, maxY), itemStack(itemStack)
    {
    }
protected:
    virtual void render(Renderer &renderer, float minZ, float maxZ, bool hasFocus) override
    {
        float backgroundZ = interpolate<float>(0.95f, minZ, maxZ);
        assert(backgroundZ / minZ > ItemDescriptor::maxRenderZ / ItemDescriptor::minRenderZ);
        float scale = backgroundZ / ItemDescriptor::maxRenderZ;
        if(!itemStack->good())
            return;
        Item item = itemStack->item;
        Mesh dest;
        item.descriptor->render(item, dest, minX, maxX, minY, maxY);
        renderer << RenderLayer::Opaque << transform(Matrix::scale(scale), dest);
        if(itemStack->count > 1)
        {
            std::wostringstream os;
            os << L"\n";
            os << (unsigned)itemStack->count;
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
            renderer << transform(Matrix::scale(textScale).concat(Matrix::translate(xOffset, yOffset, -1)).concat(Matrix::scale(minZ)), Text::mesh(text, textColor, textProperties));
        }
    }
};

class UiItemWithBorder : public Element
{
private:
    std::shared_ptr<ItemStack> itemStack;
public:
    std::shared_ptr<ItemStack> getItemStack() const
    {
        return itemStack;
    }
    std::function<bool()> isSelected;
    UiItemWithBorder(float minX, float maxX, float minY, float maxY, std::shared_ptr<ItemStack> itemStack, std::function<bool()> isSelected)
        : Element(minX, maxX, minY, maxY), itemStack(itemStack), isSelected(isSelected)
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
                                                  VectorF(minX * borderZ, minY * borderZ, -borderZ), backgroundColor,
                                                  VectorF(maxX * borderZ, minY * borderZ, -borderZ), backgroundColor,
                                                  VectorF(maxX * borderZ, maxY * borderZ, -borderZ), backgroundColor,
                                                  VectorF(minX * borderZ, maxY * borderZ, -borderZ), backgroundColor);
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
        if(!itemStack->good())
            return;
        Item item = itemStack->item;
        Mesh dest;
        item.descriptor->render(item, dest, innerMinX, innerMaxX, innerMinY, innerMaxY);
        renderer << RenderLayer::Opaque << transform(Matrix::scale(scale), dest);
        if(itemStack->count > 1)
        {
            std::wostringstream os;
            os << L"\n";
            os << L"\n";
            os << (unsigned)itemStack->count;
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
            renderer << transform(Matrix::scale(textScale).concat(Matrix::translate(xOffset, yOffset, -1)).concat(Matrix::scale(minZ)), Text::mesh(text, textColor, textProperties));
        }
    }
};
}
}
}

#endif // UI_ITEM_H_INCLUDED
