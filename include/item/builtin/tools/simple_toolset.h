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
#ifndef SIMPLE_TOOLSET_H_INCLUDED
#define SIMPLE_TOOLSET_H_INCLUDED

#include "item/builtin/tools/tools.h"
#include "recipe/builtin/pattern.h"
#include "item/builtin/wood.h" // for stick

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
class SimplePickaxe : public Pickaxe
{
    SimplePickaxe(const SimplePickaxe &) = delete;
    SimplePickaxe &operator=(const SimplePickaxe &) = delete;

protected:
    const ToolLevel toolLevel;
    const float mineDurationFactor;
    RecipeDescriptorPointer recipe;
    const unsigned maxDamageValue;
    class SimplePickaxeRecipe : public Recipes::builtin::PatternRecipe<3, 3>
    {
        SimplePickaxeRecipe(const SimplePickaxeRecipe &) = delete;
        SimplePickaxeRecipe &operator=(const SimplePickaxeRecipe &) = delete;

    protected:
        const unsigned outputCount;
        const ItemDescriptorPointer itemDescriptor;
        virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
        {
            if(input.getRecipeBlock().good()
               && !input.getRecipeBlock().descriptor->isToolForCrafting())
                return false;
            output = RecipeOutput(ItemStack(Item(itemDescriptor), outputCount));
            return true;
        }

    public:
        SimplePickaxeRecipe(ItemDescriptorPointer itemDescriptor,
                            Item recipeMaterial,
                            unsigned outputCount)
            : PatternRecipe(checked_array<Item, 3 * 3>{
                  recipeMaterial,
                  recipeMaterial,
                  recipeMaterial,
                  Item(),
                  Item(Items::builtin::Stick::descriptor()),
                  Item(),
                  Item(),
                  Item(Items::builtin::Stick::descriptor()),
                  Item(),
              }),
              outputCount(outputCount),
              itemDescriptor(itemDescriptor)
        {
        }
    };
    SimplePickaxe(std::wstring toolsetName,
                  TextureDescriptor td,
                  ToolLevel toolLevel,
                  float mineDurationFactor,
                  unsigned maxDamageValue)
        : Pickaxe(L"builtin.tools.simple_pickaxe(toolset=" + toolsetName + L")", td),
          toolLevel(toolLevel),
          mineDurationFactor(mineDurationFactor),
          recipe(nullptr),
          maxDamageValue(maxDamageValue)
    {
    }

public:
    SimplePickaxe(std::wstring toolsetName,
                  TextureDescriptor td,
                  ToolLevel toolLevel,
                  float mineDurationFactor,
                  unsigned maxDamageValue,
                  Item recipeMaterial)
        : SimplePickaxe(toolsetName, td, toolLevel, mineDurationFactor, maxDamageValue)
    {
        recipe = new SimplePickaxeRecipe(this, recipeMaterial, 1);
    }
    virtual ~SimplePickaxe()
    {
        delete recipe;
    }
    virtual float getMineDurationFactor(Item item) const override
    {
        return mineDurationFactor;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return toolLevel;
    }
    virtual unsigned maxDamage() const override
    {
        return maxDamageValue;
    }
};

class SimpleAxe : public Axe
{
    SimpleAxe(const SimpleAxe &) = delete;
    SimpleAxe &operator=(const SimpleAxe &) = delete;

protected:
    const ToolLevel toolLevel;
    const float mineDurationFactor;
    RecipeDescriptorPointer recipe;
    const unsigned maxDamageValue;
    class SimpleAxeRecipe : public Recipes::builtin::PatternRecipe<2, 3>
    {
        SimpleAxeRecipe(const SimpleAxeRecipe &) = delete;
        SimpleAxeRecipe &operator=(const SimpleAxeRecipe &) = delete;

    protected:
        const unsigned outputCount;
        const ItemDescriptorPointer itemDescriptor;
        virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
        {
            if(input.getRecipeBlock().good()
               && !input.getRecipeBlock().descriptor->isToolForCrafting())
                return false;
            output = RecipeOutput(ItemStack(Item(itemDescriptor), outputCount));
            return true;
        }

    public:
        SimpleAxeRecipe(ItemDescriptorPointer itemDescriptor,
                        Item recipeMaterial,
                        unsigned outputCount)
            : PatternRecipe(checked_array<Item, 2 * 3>{
                  recipeMaterial,
                  recipeMaterial,
                  recipeMaterial,
                  Item(Items::builtin::Stick::descriptor()),
                  Item(),
                  Item(Items::builtin::Stick::descriptor()),
              }),
              outputCount(outputCount),
              itemDescriptor(itemDescriptor)
        {
        }
    };
    SimpleAxe(std::wstring toolsetName,
              TextureDescriptor td,
              ToolLevel toolLevel,
              float mineDurationFactor,
              unsigned maxDamageValue)
        : Axe(L"builtin.tools.simple_axe(toolset=" + toolsetName + L")", td),
          toolLevel(toolLevel),
          mineDurationFactor(mineDurationFactor),
          recipe(nullptr),
          maxDamageValue(maxDamageValue)
    {
    }

public:
    SimpleAxe(std::wstring toolsetName,
              TextureDescriptor td,
              ToolLevel toolLevel,
              float mineDurationFactor,
              unsigned maxDamageValue,
              Item recipeMaterial)
        : SimpleAxe(toolsetName, td, toolLevel, mineDurationFactor, maxDamageValue)
    {
        recipe = new SimpleAxeRecipe(this, recipeMaterial, 1);
    }
    virtual ~SimpleAxe()
    {
        delete recipe;
    }
    virtual float getMineDurationFactor(Item item) const override
    {
        return mineDurationFactor;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return toolLevel;
    }
    virtual unsigned maxDamage() const override
    {
        return maxDamageValue;
    }
};

class SimpleShovel : public Shovel
{
    SimpleShovel(const SimpleShovel &) = delete;
    SimpleShovel &operator=(const SimpleShovel &) = delete;

protected:
    const ToolLevel toolLevel;
    const float mineDurationFactor;
    RecipeDescriptorPointer recipe;
    const unsigned maxDamageValue;
    class SimpleShovelRecipe : public Recipes::builtin::PatternRecipe<1, 3>
    {
        SimpleShovelRecipe(const SimpleShovelRecipe &) = delete;
        SimpleShovelRecipe &operator=(const SimpleShovelRecipe &) = delete;

    protected:
        const unsigned outputCount;
        const ItemDescriptorPointer itemDescriptor;
        virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
        {
            if(input.getRecipeBlock().good()
               && !input.getRecipeBlock().descriptor->isToolForCrafting())
                return false;
            output = RecipeOutput(ItemStack(Item(itemDescriptor), outputCount));
            return true;
        }

    public:
        SimpleShovelRecipe(ItemDescriptorPointer itemDescriptor,
                           Item recipeMaterial,
                           unsigned outputCount)
            : PatternRecipe(checked_array<Item, 1 * 3>{
                  recipeMaterial,
                  Item(Items::builtin::Stick::descriptor()),
                  Item(Items::builtin::Stick::descriptor()),
              }),
              outputCount(outputCount),
              itemDescriptor(itemDescriptor)
        {
        }
    };
    SimpleShovel(std::wstring toolsetName,
                 TextureDescriptor td,
                 ToolLevel toolLevel,
                 float mineDurationFactor,
                 unsigned maxDamageValue)
        : Shovel(L"builtin.tools.simple_shovel(toolset=" + toolsetName + L")", td),
          toolLevel(toolLevel),
          mineDurationFactor(mineDurationFactor),
          recipe(nullptr),
          maxDamageValue(maxDamageValue)
    {
    }

public:
    SimpleShovel(std::wstring toolsetName,
                 TextureDescriptor td,
                 ToolLevel toolLevel,
                 float mineDurationFactor,
                 unsigned maxDamageValue,
                 Item recipeMaterial)
        : SimpleShovel(toolsetName, td, toolLevel, mineDurationFactor, maxDamageValue)
    {
        recipe = new SimpleShovelRecipe(this, recipeMaterial, 1);
    }
    virtual ~SimpleShovel()
    {
        delete recipe;
    }
    virtual float getMineDurationFactor(Item item) const override
    {
        return mineDurationFactor;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return toolLevel;
    }
    virtual unsigned maxDamage() const override
    {
        return maxDamageValue;
    }
};

class SimpleHoe : public Hoe
{
    SimpleHoe(const SimpleHoe &) = delete;
    SimpleHoe &operator=(const SimpleHoe &) = delete;

protected:
    const ToolLevel toolLevel;
    const float mineDurationFactor;
    RecipeDescriptorPointer recipe;
    const unsigned maxDamageValue;
    class SimpleHoeRecipe : public Recipes::builtin::PatternRecipe<2, 3>
    {
        SimpleHoeRecipe(const SimpleHoeRecipe &) = delete;
        SimpleHoeRecipe &operator=(const SimpleHoeRecipe &) = delete;

    protected:
        const unsigned outputCount;
        const ItemDescriptorPointer itemDescriptor;
        virtual bool fillOutput(const RecipeInput &input, RecipeOutput &output) const override
        {
            if(input.getRecipeBlock().good()
               && !input.getRecipeBlock().descriptor->isToolForCrafting())
                return false;
            output = RecipeOutput(ItemStack(Item(itemDescriptor), outputCount));
            return true;
        }

    public:
        SimpleHoeRecipe(ItemDescriptorPointer itemDescriptor,
                        Item recipeMaterial,
                        unsigned outputCount)
            : PatternRecipe(checked_array<Item, 2 * 3>{
                  recipeMaterial,
                  recipeMaterial,
                  Item(),
                  Item(Items::builtin::Stick::descriptor()),
                  Item(),
                  Item(Items::builtin::Stick::descriptor()),
              }),
              outputCount(outputCount),
              itemDescriptor(itemDescriptor)
        {
        }
    };
    SimpleHoe(std::wstring toolsetName,
              TextureDescriptor td,
              ToolLevel toolLevel,
              float mineDurationFactor,
              unsigned maxDamageValue)
        : Hoe(L"builtin.tools.simple_hoe(toolset=" + toolsetName + L")", td),
          toolLevel(toolLevel),
          mineDurationFactor(mineDurationFactor),
          recipe(nullptr),
          maxDamageValue(maxDamageValue)
    {
    }

public:
    SimpleHoe(std::wstring toolsetName,
              TextureDescriptor td,
              ToolLevel toolLevel,
              float mineDurationFactor,
              unsigned maxDamageValue,
              Item recipeMaterial)
        : SimpleHoe(toolsetName, td, toolLevel, mineDurationFactor, maxDamageValue)
    {
        recipe = new SimpleHoeRecipe(this, recipeMaterial, 1);
    }
    virtual ~SimpleHoe()
    {
        delete recipe;
    }
    virtual float getMineDurationFactor(Item item) const override
    {
        return mineDurationFactor;
    }
    virtual ToolLevel getToolLevel() const override
    {
        return toolLevel;
    }
    virtual unsigned maxDamage() const override
    {
        return maxDamageValue;
    }
};

FIXME_MESSAGE(add more tools to toolset)

class SimpleToolset
{
    SimpleToolset(const SimpleToolset &) = delete;
    const SimpleToolset &operator=(const SimpleToolset &) = delete;

public:
    const std::wstring toolsetName;
    const ToolLevel toolLevel;
    const float mineDurationFactor;

protected:
    const Pickaxe *pickaxe;
    const Axe *axe;
    const Shovel *shovel;
    const Hoe *hoe;
    static const Pickaxe *makePickaxe(std::wstring toolsetName,
                                      ToolLevel toolLevel,
                                      float mineDurationFactor,
                                      unsigned maxDamage,
                                      Item recipeMaterial,
                                      TextureDescriptor td)
    {
        if(td)
            return new SimplePickaxe(
                toolsetName, td, toolLevel, mineDurationFactor, maxDamage, recipeMaterial);
        return nullptr;
    }
    static const Axe *makeAxe(std::wstring toolsetName,
                              ToolLevel toolLevel,
                              float mineDurationFactor,
                              unsigned maxDamage,
                              Item recipeMaterial,
                              TextureDescriptor td)
    {
        if(td)
            return new SimpleAxe(
                toolsetName, td, toolLevel, mineDurationFactor, maxDamage, recipeMaterial);
        return nullptr;
    }
    static const Shovel *makeShovel(std::wstring toolsetName,
                                    ToolLevel toolLevel,
                                    float mineDurationFactor,
                                    unsigned maxDamage,
                                    Item recipeMaterial,
                                    TextureDescriptor td)
    {
        if(td)
            return new SimpleShovel(
                toolsetName, td, toolLevel, mineDurationFactor, maxDamage, recipeMaterial);
        return nullptr;
    }
    static const Hoe *makeHoe(std::wstring toolsetName,
                              ToolLevel toolLevel,
                              float mineDurationFactor,
                              unsigned maxDamage,
                              Item recipeMaterial,
                              TextureDescriptor td)
    {
        if(td)
            return new SimpleHoe(
                toolsetName, td, toolLevel, mineDurationFactor, maxDamage, recipeMaterial);
        return nullptr;
    }

public:
    SimpleToolset(std::wstring toolsetName,
                  ToolLevel toolLevel,
                  float mineDurationFactor,
                  const Pickaxe *pickaxe,
                  const Axe *axe,
                  const Shovel *shovel,
                  const Hoe *hoe)
        : toolsetName(toolsetName),
          toolLevel(toolLevel),
          mineDurationFactor(mineDurationFactor),
          pickaxe(pickaxe),
          axe(axe),
          shovel(shovel),
          hoe(hoe)
    {
    }
    SimpleToolset(std::wstring toolsetName,
                  ToolLevel toolLevel,
                  float mineDurationFactor,
                  unsigned maxDamage,
                  Item recipeMaterial,
                  TextureDescriptor pickaxeTexture,
                  TextureDescriptor axeTexture,
                  TextureDescriptor shovelTexture,
                  TextureDescriptor hoeTexture)
        : toolsetName(toolsetName),
          toolLevel(toolLevel),
          mineDurationFactor(mineDurationFactor),
          pickaxe(),
          axe(),
          shovel(),
          hoe()
    {
        pickaxe = makePickaxe(
            toolsetName, toolLevel, mineDurationFactor, maxDamage, recipeMaterial, pickaxeTexture);
        axe = makeAxe(
            toolsetName, toolLevel, mineDurationFactor, maxDamage, recipeMaterial, axeTexture);
        shovel = makeShovel(
            toolsetName, toolLevel, mineDurationFactor, maxDamage, recipeMaterial, shovelTexture);
        hoe = makeHoe(
            toolsetName, toolLevel, mineDurationFactor, maxDamage, recipeMaterial, hoeTexture);
    }
    virtual ~SimpleToolset()
    {
        delete pickaxe;
        delete axe;
        delete shovel;
        delete hoe;
    }
    const Pickaxe *getPickaxe() const
    {
        return pickaxe;
    }
    const Axe *getAxe() const
    {
        return axe;
    }
    const Shovel *getShovel() const
    {
        return shovel;
    }
    const Hoe *getHoe() const
    {
        return hoe;
    }
};
}
}
}
}
}

#endif // SIMPLE_TOOLSET_H_INCLUDED
