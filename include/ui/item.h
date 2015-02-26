#ifndef UI_ITEM_H_INCLUDED
#define UI_ITEM_H_INCLUDED

#include "ui/element.h"
#include "item/item.h"
#include "util/util.h"
#include <cassert>
#include <sstream>
#include "render/text.h"

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
            os.width(2);
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
            float textScale = 0.5 * (maxY - minY) / textHeight;
            textScale = std::min<float>(textScale, (maxX - minX) / textWidth);
            float xOffset = -0.5 * textWidth, yOffset = -0.5 * textHeight;
            xOffset = textScale * xOffset + 0.5 * (minX + maxX);
            yOffset = textScale * yOffset + 0.5 * (minY + maxY);
            renderer << transform(Matrix::scale(textScale).concat(Matrix::translate(xOffset, yOffset, -1)).concat(Matrix::scale(minZ)), Text::mesh(text, textColor, textProperties));
        }
    }
};
}
}
}

#endif // UI_ITEM_H_INCLUDED
