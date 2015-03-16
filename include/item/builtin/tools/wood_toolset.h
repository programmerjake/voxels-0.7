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
#ifndef WOOD_TOOLSET_H_INCLUDED
#define WOOD_TOOLSET_H_INCLUDED

#include "item/builtin/tools/simple_toolset.h"
#include "util/global_instance_maker.h"
#include "texture/texture_atlas.h"
#include "util/wood_descriptor.h"
#include "block/builtin/woods.h"

namespace programmerjake
{
namespace voxels
{
namespace Items
{
namespace builtin
{
namespace tools
{
class WoodToolset;
class WoodPickaxe final : public SimplePickaxe
{
    friend class WoodToolset;
private:
    class WoodPickaxeRecipe final : public SimplePickaxeRecipe
    {
        using SimplePickaxeRecipe::SimplePickaxeRecipe;
        virtual bool itemsMatch(const Item &patternItem, const Item &inputItem) const override
        {
            if(dynamic_cast<const Items::builtin::WoodPlanks *>(patternItem.descriptor) != nullptr &&
               dynamic_cast<const Items::builtin::WoodPlanks *>(inputItem.descriptor) != nullptr)
                return true;
            return patternItem == inputItem;
        }

    };
    WoodPickaxe(std::wstring toolsetName, TextureDescriptor td, ToolLevel toolLevel, float mineDurationFactor, unsigned maxDamageValue, Item recipeMaterial)
        : SimplePickaxe(toolsetName, td, toolLevel, mineDurationFactor, maxDamageValue)
    {
        recipe = new WoodPickaxeRecipe(this, recipeMaterial, 1);
    }
};
class WoodAxe final : public SimpleAxe
{
    friend class WoodToolset;
private:
    class WoodAxeRecipe final : public SimpleAxeRecipe
    {
        using SimpleAxeRecipe::SimpleAxeRecipe;
        virtual bool itemsMatch(const Item &patternItem, const Item &inputItem) const override
        {
            if(dynamic_cast<const Items::builtin::WoodPlanks *>(patternItem.descriptor) != nullptr &&
               dynamic_cast<const Items::builtin::WoodPlanks *>(inputItem.descriptor) != nullptr)
                return true;
            return patternItem == inputItem;
        }

    };
    WoodAxe(std::wstring toolsetName, TextureDescriptor td, ToolLevel toolLevel, float mineDurationFactor, unsigned maxDamageValue, Item recipeMaterial)
        : SimpleAxe(toolsetName, td, toolLevel, mineDurationFactor, maxDamageValue)
    {
        recipe = new WoodAxeRecipe(this, recipeMaterial, 1);
    }
};
class WoodShovel final : public SimpleShovel
{
    friend class WoodToolset;
private:
    class WoodShovelRecipe final : public SimpleShovelRecipe
    {
        using SimpleShovelRecipe::SimpleShovelRecipe;
        virtual bool itemsMatch(const Item &patternItem, const Item &inputItem) const override
        {
            if(dynamic_cast<const Items::builtin::WoodPlanks *>(patternItem.descriptor) != nullptr &&
               dynamic_cast<const Items::builtin::WoodPlanks *>(inputItem.descriptor) != nullptr)
                return true;
            return patternItem == inputItem;
        }

    };
    WoodShovel(std::wstring toolsetName, TextureDescriptor td, ToolLevel toolLevel, float mineDurationFactor, unsigned maxDamageValue, Item recipeMaterial)
        : SimpleShovel(toolsetName, td, toolLevel, mineDurationFactor, maxDamageValue)
    {
        recipe = new WoodShovelRecipe(this, recipeMaterial, 1);
    }
};
class WoodHoe final : public SimpleHoe
{
    friend class WoodToolset;
private:
    class WoodHoeRecipe final : public SimpleHoeRecipe
    {
        using SimpleHoeRecipe::SimpleHoeRecipe;
        virtual bool itemsMatch(const Item &patternItem, const Item &inputItem) const override
        {
            if(dynamic_cast<const Items::builtin::WoodPlanks *>(patternItem.descriptor) != nullptr &&
               dynamic_cast<const Items::builtin::WoodPlanks *>(inputItem.descriptor) != nullptr)
                return true;
            return patternItem == inputItem;
        }

    };
    WoodHoe(std::wstring toolsetName, TextureDescriptor td, ToolLevel toolLevel, float mineDurationFactor, unsigned maxDamageValue, Item recipeMaterial)
        : SimpleHoe(toolsetName, td, toolLevel, mineDurationFactor, maxDamageValue)
    {
        recipe = new WoodHoeRecipe(this, recipeMaterial, 1);
    }
};
class WoodToolset final : public SimpleToolset
{
    friend class global_instance_maker<WoodToolset>;
private:
    static const Pickaxe *makePickaxe(std::wstring toolsetName, ToolLevel toolLevel, float mineDurationFactor, unsigned maxDamage, Item recipeMaterial, TextureDescriptor td)
    {
        if(td)
            return new WoodPickaxe(toolsetName, td, toolLevel, mineDurationFactor, maxDamage, recipeMaterial);
        return nullptr;
    }
    static const Axe *makeAxe(std::wstring toolsetName, ToolLevel toolLevel, float mineDurationFactor, unsigned maxDamage, Item recipeMaterial, TextureDescriptor td)
    {
        if(td)
            return new WoodAxe(toolsetName, td, toolLevel, mineDurationFactor, maxDamage, recipeMaterial);
        return nullptr;
    }
    static const Shovel *makeShovel(std::wstring toolsetName, ToolLevel toolLevel, float mineDurationFactor, unsigned maxDamage, Item recipeMaterial, TextureDescriptor td)
    {
        if(td)
            return new WoodShovel(toolsetName, td, toolLevel, mineDurationFactor, maxDamage, recipeMaterial);
        return nullptr;
    }
    static const Hoe *makeHoe(std::wstring toolsetName, ToolLevel toolLevel, float mineDurationFactor, unsigned maxDamage, Item recipeMaterial, TextureDescriptor td)
    {
        if(td)
            return new WoodHoe(toolsetName, td, toolLevel, mineDurationFactor, maxDamage, recipeMaterial);
        return nullptr;
    }
    static std::wstring name()
    {
        return L"builtin.wood";
    }
    WoodToolset()
        : SimpleToolset(name(), ToolLevel_Wood, 1.0f / 2.0f, nullptr, nullptr, nullptr, nullptr)
    {
        std::wstring toolsetName = name();
        const unsigned maxDamage = 60;
        Item recipeMaterial = Item(Woods::builtin::Oak::descriptor()->getPlanksItemDescriptor());
        pickaxe = makePickaxe(toolsetName, toolLevel, mineDurationFactor, maxDamage, recipeMaterial, TextureAtlas::WoodPickaxe.td());
        axe = makeAxe(toolsetName, toolLevel, mineDurationFactor, maxDamage, recipeMaterial, TextureAtlas::WoodAxe.td());
        shovel = makeShovel(toolsetName, toolLevel, mineDurationFactor, maxDamage, recipeMaterial, TextureAtlas::WoodShovel.td());
        hoe = makeHoe(toolsetName, toolLevel, mineDurationFactor, maxDamage, recipeMaterial, TextureAtlas::WoodHoe.td());
    }
public:
    static const WoodToolset *pointer()
    {
        return global_instance_maker<WoodToolset>::getInstance();
    }
    static const SimpleToolset *descriptor()
    {
        return pointer();
    }
};
}
}
}
}
}

#endif // STONE_TOOLSET_H_INCLUDED
