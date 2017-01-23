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
#ifndef ORE_VEIN_H_INCLUDED
#define ORE_VEIN_H_INCLUDED

#include "generate/decorator/mineral_vein.h"
#include "block/builtin/ores.h"
#include "util/global_instance_maker.h"

namespace programmerjake
{
namespace voxels
{
namespace Decorators
{
namespace builtin
{
class CoalOreVeinDecorator : public MineralVeinDecorator
{
    friend class global_instance_maker<CoalOreVeinDecorator>;

private:
    CoalOreVeinDecorator() : MineralVeinDecorator(L"builtin.coal_ore_vein", 17, 0, 128, 20, 800)
    {
    }

protected:
    virtual Block getOreBlock() const override
    {
        return Block(Blocks::builtin::CoalOre::descriptor());
    }

public:
    static const CoalOreVeinDecorator *pointer()
    {
        return global_instance_maker<CoalOreVeinDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
};

class IronOreVeinDecorator : public MineralVeinDecorator
{
    friend class global_instance_maker<IronOreVeinDecorator>;

private:
    IronOreVeinDecorator() : MineralVeinDecorator(L"builtin.iron_ore_vein", 9, 0, 64, 20, 700)
    {
    }

protected:
    virtual Block getOreBlock() const override
    {
        return Block(Blocks::builtin::IronOre::descriptor());
    }

public:
    static const IronOreVeinDecorator *pointer()
    {
        return global_instance_maker<IronOreVeinDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
};

class GoldOreVeinDecorator : public MineralVeinDecorator
{
    friend class global_instance_maker<GoldOreVeinDecorator>;

private:
    GoldOreVeinDecorator() : MineralVeinDecorator(L"builtin.gold_ore_vein", 9, 0, 32, 2, 600)
    {
    }

protected:
    virtual Block getOreBlock() const override
    {
        return Block(Blocks::builtin::GoldOre::descriptor());
    }

public:
    static const GoldOreVeinDecorator *pointer()
    {
        return global_instance_maker<GoldOreVeinDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
};

class RedstoneOreVeinDecorator : public MineralVeinDecorator
{
    friend class global_instance_maker<RedstoneOreVeinDecorator>;

private:
    RedstoneOreVeinDecorator()
        : MineralVeinDecorator(L"builtin.redstone_ore_vein", 8, 0, 16, 8, 700)
    {
    }

protected:
    virtual Block getOreBlock() const override
    {
        return Block(Blocks::builtin::RedstoneOre::descriptor());
    }

public:
    static const RedstoneOreVeinDecorator *pointer()
    {
        return global_instance_maker<RedstoneOreVeinDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
};

class DiamondOreVeinDecorator : public MineralVeinDecorator
{
    friend class global_instance_maker<DiamondOreVeinDecorator>;

private:
    DiamondOreVeinDecorator() : MineralVeinDecorator(L"builtin.diamond_ore_vein", 8, 0, 16, 1, 500)
    {
    }

protected:
    virtual Block getOreBlock() const override
    {
        return Block(Blocks::builtin::DiamondOre::descriptor());
    }

public:
    static const DiamondOreVeinDecorator *pointer()
    {
        return global_instance_maker<DiamondOreVeinDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
};

class LapisLazuliOreVeinDecorator : public MineralVeinDecorator
{
    friend class global_instance_maker<LapisLazuliOreVeinDecorator>;

private:
    LapisLazuliOreVeinDecorator()
        : MineralVeinDecorator(L"builtin.lapis_lazuli_ore_vein", 7, 0, 32, 1, 650)
    {
    }

protected:
    virtual Block getOreBlock() const override
    {
        return Block(Blocks::builtin::LapisLazuliOre::descriptor());
    }

public:
    static const LapisLazuliOreVeinDecorator *pointer()
    {
        return global_instance_maker<LapisLazuliOreVeinDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
};

class EmeraldOreVeinDecorator : public MineralVeinDecorator
{
    friend class global_instance_maker<EmeraldOreVeinDecorator>;

private:
    EmeraldOreVeinDecorator() : MineralVeinDecorator(L"builtin.emerald_ore_vein", 1, 0, 16, 0, 400)
    {
    }

protected:
    virtual Block getOreBlock() const override
    {
        return Block(Blocks::builtin::RedstoneOre::descriptor());
    }

public:
    static const EmeraldOreVeinDecorator *pointer()
    {
        return global_instance_maker<EmeraldOreVeinDecorator>::getInstance();
    }
    static DecoratorDescriptorPointer descriptor()
    {
        return pointer();
    }
};
}
}
}
}

#endif // ORE_VEIN_H_INCLUDED
