#ifndef UI_ITEM_H_INCLUDED
#define UI_ITEM_H_INCLUDED

#include "ui/element.h"
#include "item/item.h"
#include "util/util.h"

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
        #error finish
    }
};
}
}
}

#endif // UI_ITEM_H_INCLUDED
