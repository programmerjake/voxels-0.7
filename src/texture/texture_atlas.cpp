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
#include "texture/texture_atlas.h"
#include <iostream>
#include <cstdlib>
#include <cassert>

using namespace std;

namespace programmerjake
{
namespace voxels
{
namespace
{
Image loadImage(std::wstring name)
{
    try
    {
        wcout << L"loading " << name << L"..." << flush;
        Image retval = Image(name);
        wcout << L"\r\x1b[K" << flush;
        return retval;
    }
    catch(exception &e)
    {
        cerr << "\r\x1b[Kerror : " << e.what() << endl;
        exit(1);
    }
}
}

checked_array<TextureAtlas::ImageDescriptor, 8> &TextureAtlas::textures()
{
    static checked_array<TextureAtlas::ImageDescriptor, 8> retval =
    {
        TextureAtlas::ImageDescriptor(L"textures.png", 512, 512),
        TextureAtlas::ImageDescriptor(L"inventory.png", 256, 256),
        TextureAtlas::ImageDescriptor(L"workbench.png", 256, 256),
        TextureAtlas::ImageDescriptor(L"chestedit.png", 256, 256),
        TextureAtlas::ImageDescriptor(L"creative.png", 256, 256),
        TextureAtlas::ImageDescriptor(L"dispenserdropper.png", 256, 256),
        TextureAtlas::ImageDescriptor(L"furnace.png", 256, 256),
        TextureAtlas::ImageDescriptor(L"hopper.png", 256, 256),
    };
    return retval;
}

TextureAtlas::TextureLoader TextureAtlas::textureLoader;

Image TextureAtlas::texture(std::size_t textureIndex)
{
    auto &textures_array = textures();
    ImageDescriptor &t = textures_array[textureIndex];
    if(t.image == nullptr)
    {
        for(std::size_t i = textures_array.size() - 1, j = i + 1; j > 0 ; i--, j--)
        {
            textures_array[i].image = new Image(loadImage(textures_array[i].fileName));
        }
    }
    return *t.image;
}

TextureAtlas::TextureLoader::TextureLoader()
{
    for(std::size_t i = 0; i < textures().size(); i++)
    {
        texture(i);
    }
}

const TextureAtlas
    TextureAtlas::ActivatorRailOff = {0, 0, 16, 16, 0},
    TextureAtlas::ActivatorRailOn = {16, 0, 16, 16, 0},
    TextureAtlas::BedHead = {32, 0, 16, 16, 0},
    TextureAtlas::BedFoot = {48, 0, 16, 16, 0},
    TextureAtlas::BedTopSide = {64, 0, 16, 16, 0},
    TextureAtlas::BedTopLeftSide = {80, 0, 16, 16, 0},
    TextureAtlas::BedTopRightSide = {112, 0, 16, 16, 0},
    TextureAtlas::BedBottom = {128, 0, 16, 16, 0},
    TextureAtlas::BedBottomLeftSide = {96, 0, 16, 16, 0},
    TextureAtlas::BedBottomRightSide = {144, 0, 16, 16, 0},
    TextureAtlas::BedBottomSide = {160, 0, 16, 16, 0},
    TextureAtlas::BedItem = {176, 0, 16, 16, 0},
    TextureAtlas::Font8x8 = {128, 128, 128, 128, 0},
    TextureAtlas::Shockwave = {0, 128, 128, 128, 0},
    TextureAtlas::Bedrock = {192, 0, 16, 16, 0},
    TextureAtlas::BirchLeaves = {208, 0, 16, 16, 0},
    TextureAtlas::BirchLeavesBlocked = {320, 112, 16, 16, 0},
    TextureAtlas::BirchPlank = {224, 0, 16, 16, 0},
    TextureAtlas::BirchSapling = {240, 0, 16, 16, 0},
    TextureAtlas::BirchWood = {0, 16, 16, 16, 0},
    TextureAtlas::WoodEnd = {16, 16, 16, 16, 0},
    TextureAtlas::BlazePowder = {32, 16, 16, 16, 0},
    TextureAtlas::BlazeRod = {48, 16, 16, 16, 0},
    TextureAtlas::Bone = {64, 16, 16, 16, 0},
    TextureAtlas::BoneMeal = {80, 16, 16, 16, 0},
    TextureAtlas::Bow = {96, 16, 16, 16, 0},
    TextureAtlas::BrownMushroom = {112, 16, 16, 16, 0},
    TextureAtlas::Bucket = {128, 16, 16, 16, 0},
    TextureAtlas::CactusSide = {144, 16, 16, 16, 0},
    TextureAtlas::CactusBottom = {160, 16, 16, 16, 0},
    TextureAtlas::CactusTop = {176, 16, 16, 16, 0},
    TextureAtlas::CactusGreen = {192, 16, 16, 16, 0},
    TextureAtlas::Charcoal = {368, 112, 16, 16, 0},
    TextureAtlas::ChestSide = {208, 16, 16, 16, 0},
    TextureAtlas::ChestTop = {224, 16, 16, 16, 0},
    TextureAtlas::ChestFront = {240, 16, 16, 16, 0},
    TextureAtlas::Cobblestone = {0, 32, 16, 16, 0},
    TextureAtlas::Coal = {16, 32, 16, 16, 0},
    TextureAtlas::CoalOre = {32, 32, 16, 16, 0},
    TextureAtlas::CocoaSmallSide = {48, 32, 16, 16, 0},
    TextureAtlas::CocoaSmallTop = {64, 32, 16, 16, 0},
    TextureAtlas::CocoaSmallStem = {80, 32, 16, 16, 0},
    TextureAtlas::CocoaMediumSide = {96, 32, 16, 16, 0},
    TextureAtlas::CocoaMediumTop = {112, 32, 16, 16, 0},
    TextureAtlas::CocoaMediumStem = {128, 32, 16, 16, 0},
    TextureAtlas::CocoaLargeSide = {144, 32, 16, 16, 0},
    TextureAtlas::CocoaLargeTop = {160, 32, 16, 16, 0},
    TextureAtlas::CocoaLargeStem = {176, 32, 16, 16, 0},
    TextureAtlas::CocoaBeans = {192, 32, 16, 16, 0},
    TextureAtlas::DeadBush = {208, 32, 16, 16, 0},
    TextureAtlas::DandelionYellow = {224, 32, 16, 16, 0},
    TextureAtlas::Dandelion = {240, 32, 16, 16, 0},
    TextureAtlas::CyanDye = {0, 48, 16, 16, 0},
    TextureAtlas::Delete0 = {16, 48, 16, 16, 0},
    TextureAtlas::Delete1 = {32, 48, 16, 16, 0},
    TextureAtlas::Delete2 = {48, 48, 16, 16, 0},
    TextureAtlas::Delete3 = {64, 48, 16, 16, 0},
    TextureAtlas::Delete4 = {80, 48, 16, 16, 0},
    TextureAtlas::Delete5 = {96, 48, 16, 16, 0},
    TextureAtlas::Delete6 = {112, 48, 16, 16, 0},
    TextureAtlas::Delete7 = {128, 48, 16, 16, 0},
    TextureAtlas::Delete8 = {144, 48, 16, 16, 0},
    TextureAtlas::Delete9 = {160, 48, 16, 16, 0},
    TextureAtlas::DetectorRailOff = {176, 48, 16, 16, 0},
    TextureAtlas::DetectorRailOn = {192, 48, 16, 16, 0},
    TextureAtlas::Diamond = {208, 48, 16, 16, 0},
    TextureAtlas::DiamondAxe = {224, 48, 16, 16, 0},
    TextureAtlas::DiamondHoe = {240, 48, 16, 16, 0},
    TextureAtlas::DiamondOre = {0, 64, 16, 16, 0},
    TextureAtlas::DiamondPickaxe = {16, 64, 16, 16, 0},
    TextureAtlas::DiamondShovel = {32, 64, 16, 16, 0},
    TextureAtlas::Dirt = {48, 64, 16, 16, 0},
    TextureAtlas::DirtMask = {64, 64, 16, 16, 0},
    TextureAtlas::DispenserSide = {80, 64, 16, 16, 0},
    TextureAtlas::DispenserTop = {96, 64, 16, 16, 0},
    TextureAtlas::DropperSide = {112, 64, 16, 16, 0},
    TextureAtlas::DropperTop = {128, 64, 16, 16, 0},
    TextureAtlas::DispenserDropperPistonFurnaceFrame = {144, 64, 16, 16, 0},
    TextureAtlas::Emerald = {160, 48, 16, 16, 0},
    TextureAtlas::EmeraldOre = {176, 64, 16, 16, 0},
    TextureAtlas::PistonBaseSide = {192, 64, 16, 16, 0},
    TextureAtlas::PistonBaseTop = {208, 64, 16, 16, 0},
    TextureAtlas::PistonHeadSide = {224, 64, 16, 16, 0},
    TextureAtlas::PistonHeadFace = {240, 64, 16, 16, 0},
    TextureAtlas::PistonHeadBase = {0, 80, 16, 16, 0},
    TextureAtlas::StickyPistonHeadFace = {16, 80, 16, 16, 0},
    TextureAtlas::FarmlandSide = {32, 80, 16, 16, 0},
    TextureAtlas::FarmlandTop = {48, 80, 16, 16, 0},
    TextureAtlas::Fire0 = {496, 256, 16, 32, 0},
    TextureAtlas::Fire1 = {480, 256, 16, 32, 0},
    TextureAtlas::Fire2 = {464, 256, 16, 32, 0},
    TextureAtlas::Fire3 = {448, 256, 16, 32, 0},
    TextureAtlas::Fire4 = {432, 256, 16, 32, 0},
    TextureAtlas::Fire5 = {416, 256, 16, 32, 0},
    TextureAtlas::Fire6 = {400, 256, 16, 32, 0},
    TextureAtlas::Fire7 = {384, 256, 16, 32, 0},
    TextureAtlas::Fire8 = {368, 256, 16, 32, 0},
    TextureAtlas::Fire9 = {352, 256, 16, 32, 0},
    TextureAtlas::Fire10 = {336, 256, 16, 32, 0},
    TextureAtlas::Fire11 = {320, 256, 16, 32, 0},
    TextureAtlas::Fire12 = {304, 256, 16, 32, 0},
    TextureAtlas::Fire13 = {288, 256, 16, 32, 0},
    TextureAtlas::Fire14 = {272, 256, 16, 32, 0},
    TextureAtlas::Fire15 = {256, 256, 16, 32, 0},
    TextureAtlas::Fire16 = {240, 256, 16, 32, 0},
    TextureAtlas::Fire17 = {224, 256, 16, 32, 0},
    TextureAtlas::Fire18 = {208, 256, 16, 32, 0},
    TextureAtlas::Fire19 = {192, 256, 16, 32, 0},
    TextureAtlas::Fire20 = {176, 256, 16, 32, 0},
    TextureAtlas::Fire21 = {160, 256, 16, 32, 0},
    TextureAtlas::Fire22 = {144, 256, 16, 32, 0},
    TextureAtlas::Fire23 = {128, 256, 16, 32, 0},
    TextureAtlas::Fire24 = {112, 256, 16, 32, 0},
    TextureAtlas::Fire25 = {96, 256, 16, 32, 0},
    TextureAtlas::Fire26 = {80, 256, 16, 32, 0},
    TextureAtlas::Fire27 = {64, 256, 16, 32, 0},
    TextureAtlas::Fire28 = {48, 256, 16, 32, 0},
    TextureAtlas::Fire29 = {32, 256, 16, 32, 0},
    TextureAtlas::Fire30 = {16, 256, 16, 32, 0},
    TextureAtlas::Fire31 = {0, 256, 16, 32, 0},
    TextureAtlas::Flint = {192, 80, 16, 16, 0},
    TextureAtlas::FlintAndSteel = {208, 80, 16, 16, 0},
    TextureAtlas::FurnaceFrontOff = {224, 80, 16, 16, 0},
    TextureAtlas::FurnaceSide = {240, 80, 16, 16, 0},
    TextureAtlas::FurnaceFrontOn = {0, 96, 16, 16, 0},
    TextureAtlas::WorkBenchTop = {16, 96, 16, 16, 0},
    TextureAtlas::WorkBenchSide0 = {32, 96, 16, 16, 0},
    TextureAtlas::WorkBenchSide1 = {48, 96, 16, 16, 0},
    TextureAtlas::Glass = {64, 96, 16, 16, 0},
    TextureAtlas::GoldAxe = {80, 96, 16, 16, 0},
    TextureAtlas::GoldHoe = {96, 96, 16, 16, 0},
    TextureAtlas::GoldIngot = {112, 96, 16, 16, 0},
    TextureAtlas::GoldOre = {128, 96, 16, 16, 0},
    TextureAtlas::GoldPickaxe = {144, 96, 16, 16, 0},
    TextureAtlas::GoldShovel = {160, 96, 16, 16, 0},
    TextureAtlas::GrassMask = {176, 96, 16, 16, 0},
    TextureAtlas::GrassTop = {192, 96, 16, 16, 0},
    TextureAtlas::Gravel = {208, 96, 16, 16, 0},
    TextureAtlas::GrayDye = {224, 96, 16, 16, 0},
    TextureAtlas::Gunpowder = {240, 96, 16, 16, 0},
    TextureAtlas::HopperRim = {0, 112, 16, 16, 0},
    TextureAtlas::HopperInside = {16, 112, 16, 16, 0},
    TextureAtlas::HopperSide = {32, 112, 16, 16, 0},
    TextureAtlas::HopperBigBottom = {48, 112, 16, 16, 0},
    TextureAtlas::HopperMediumBottom = {68, 116, 8, 8, 0},
    TextureAtlas::HopperSmallBottom = {64, 112, 4, 4, 0},
    TextureAtlas::HopperItem = {80, 112, 16, 16, 0},
    TextureAtlas::HotBarBox = {256, 236, 20, 20, 0},
    TextureAtlas::InkSac = {96, 112, 16, 16, 0},
    TextureAtlas::IronAxe = {112, 112, 16, 16, 0},
    TextureAtlas::IronHoe = {128, 112, 16, 16, 0},
    TextureAtlas::IronIngot = {144, 112, 16, 16, 0},
    TextureAtlas::IronOre = {160, 112, 16, 16, 0},
    TextureAtlas::IronPickaxe = {176, 112, 16, 16, 0},
    TextureAtlas::IronShovel = {192, 112, 16, 16, 0},
    TextureAtlas::JungleLeaves = {208, 112, 16, 16, 0},
    TextureAtlas::JungleLeavesBlocked = {352, 112, 16, 16, 0},
    TextureAtlas::JunglePlank = {224, 112, 16, 16, 0},
    TextureAtlas::JungleSapling = {240, 112, 16, 16, 0},
    TextureAtlas::JungleWood = {256, 0, 16, 16, 0},
    TextureAtlas::Ladder = {272, 0, 16, 16, 0},
    TextureAtlas::LapisLazuli = {288, 0, 16, 16, 0},
    TextureAtlas::LapisLazuliOre = {304, 0, 16, 16, 0},
    TextureAtlas::WheatItem = {320, 0, 16, 16, 0},
    TextureAtlas::LavaBucket = {336, 0, 16, 16, 0},
    TextureAtlas::OakLeaves = {352, 0, 16, 16, 0},
    TextureAtlas::OakLeavesBlocked = {304, 112, 16, 16, 0},
    TextureAtlas::LeverBaseBigSide = {368, 0, 8, 3, 0},
    TextureAtlas::LeverBaseSmallSide = {368, 3, 6, 3, 0},
    TextureAtlas::LeverBaseTop = {368, 6, 8, 6, 0},
    TextureAtlas::LeverHandleSide = {372, 0, 2, 8, 0},
    TextureAtlas::LeverHandleTop = {380, 0, 2, 2, 0},
    TextureAtlas::LeverHandleBottom = {378, 0, 2, 2, 0},
    TextureAtlas::LightBlueDye = {384, 0, 16, 16, 0},
    TextureAtlas::LightGrayDye = {400, 0, 16, 16, 0},
    TextureAtlas::LimeDye = {416, 0, 16, 16, 0},
    TextureAtlas::MagentaDye = {432, 0, 16, 16, 0},
    TextureAtlas::MinecartInsideSide = {448, 0, 16, 16, 0},
    TextureAtlas::MinecartInsideBottom = {464, 0, 16, 16, 0},
    TextureAtlas::MinecartItem = {480, 0, 16, 16, 0},
    TextureAtlas::MinecartOutsideLeftRight = {496, 0, 16, 16, 0},
    TextureAtlas::MinecartOutsideFrontBack = {256, 16, 16, 16, 0},
    TextureAtlas::MinecartOutsideBottom = {272, 16, 16, 16, 0},
    TextureAtlas::MinecartOutsideTop = {288, 16, 16, 16, 0},
    TextureAtlas::MinecartWithChest = {304, 16, 16, 16, 0},
    TextureAtlas::MinecartWithHopper = {320, 16, 16, 16, 0},
    TextureAtlas::MinecartWithTNT = {336, 16, 16, 16, 0},
    TextureAtlas::MobSpawner = {352, 16, 16, 16, 0},
    TextureAtlas::OrangeDye = {368, 16, 16, 16, 0},
    TextureAtlas::WaterSide0 = {384, 16, 16, 16, 0},
    TextureAtlas::WaterSide1 = {400, 16, 16, 16, 0},
    TextureAtlas::WaterSide2 = {416, 16, 16, 16, 0},
    TextureAtlas::WaterSide3 = {432, 16, 16, 16, 0},
    TextureAtlas::WaterSide4 = {448, 16, 16, 16, 0},
    TextureAtlas::WaterSide5 = {464, 16, 16, 16, 0},
    TextureAtlas::Obsidian = {480, 16, 16, 16, 0},
    TextureAtlas::ParticleSmoke0 = {256, 128, 8, 8, 0},
    TextureAtlas::ParticleSmoke1 = {264, 128, 8, 8, 0},
    TextureAtlas::ParticleSmoke2 = {272, 128, 8, 8, 0},
    TextureAtlas::ParticleSmoke3 = {280, 128, 8, 8, 0},
    TextureAtlas::ParticleSmoke4 = {288, 128, 8, 8, 0},
    TextureAtlas::ParticleSmoke5 = {296, 128, 8, 8, 0},
    TextureAtlas::ParticleSmoke6 = {304, 128, 8, 8, 0},
    TextureAtlas::ParticleSmoke7 = {312, 128, 8, 8, 0},
    TextureAtlas::ParticleFire0 = {320, 128, 8, 8, 0},
    TextureAtlas::ParticleFire1 = {328, 128, 8, 8, 0},
    TextureAtlas::PinkDye = {496, 16, 16, 16, 0},
    TextureAtlas::PinkStone = {256, 16, 16, 16, 0},
    TextureAtlas::PistonShaft = {276, 244, 2, 12, 0},
    TextureAtlas::OakPlank = {272, 32, 16, 16, 0},
    TextureAtlas::PoweredRailOff = {288, 32, 16, 16, 0},
    TextureAtlas::PoweredRailOn = {304, 32, 16, 16, 0},
    TextureAtlas::PurpleDye = {320, 32, 16, 16, 0},
    TextureAtlas::PurplePortal = {336, 32, 16, 16, 0},
    TextureAtlas::Quartz = {352, 32, 16, 16, 0},
    TextureAtlas::Rail = {368, 32, 16, 16, 0},
    TextureAtlas::RailCurve = {384, 32, 16, 16, 0},
    TextureAtlas::RedMushroom = {400, 32, 16, 16, 0},
    TextureAtlas::BlockOfRedstone = {416, 32, 16, 16, 0},
    TextureAtlas::RedstoneComparatorOff = {432, 32, 16, 16, 0},
    TextureAtlas::RedstoneComparatorOn = {448, 32, 16, 16, 0},
    TextureAtlas::RedstoneComparatorRepeatorSide = {464, 46, 16, 2, 0},
    TextureAtlas::RedstoneShortTorchSideOn = {464, 32, 8, 12, 0},
    TextureAtlas::RedstoneShortTorchSideOff = {472, 32, 8, 12, 0},
    TextureAtlas::RedstoneDust0 = {480, 32, 16, 16, 0},
    TextureAtlas::RedstoneDust1 = {496, 32, 16, 16, 0},
    TextureAtlas::RedstoneDust2Corner = {256, 48, 16, 16, 0},
    TextureAtlas::RedstoneDust2Across = {272, 48, 16, 16, 0},
    TextureAtlas::RedstoneDust3 = {288, 48, 16, 16, 0},
    TextureAtlas::RedstoneDust4 = {304, 48, 16, 16, 0},
    TextureAtlas::RedstoneDustItem = {320, 48, 16, 16, 0},
    TextureAtlas::RedstoneRepeatorBarSide = {336, 48, 12, 2, 0},
    TextureAtlas::RedstoneRepeatorBarTopBottom = {336, 50, 12, 2, 0},
    TextureAtlas::RedstoneRepeatorBarEnd = {336, 52, 2, 2, 0},
    TextureAtlas::RedstoneOre = {352, 48, 16, 16, 0},
    TextureAtlas::ActiveRedstoneOre = {368, 48, 16, 16, 0},
    TextureAtlas::RedstoneRepeatorOff = {384, 48, 16, 16, 0},
    TextureAtlas::RedstoneRepeatorOn = {400, 48, 16, 16, 0},
    TextureAtlas::RedstoneTorchSideOn = {416, 48, 16, 16, 0},
    TextureAtlas::RedstoneTorchSideOff = {432, 48, 16, 16, 0},
    TextureAtlas::RedstoneTorchTopOn = {423, 54, 2, 2, 0},
    TextureAtlas::RedstoneTorchTopOff = {439, 54, 2, 2, 0},
    TextureAtlas::RedstoneTorchBottomOn = {423, 62, 2, 2, 0},
    TextureAtlas::RedstoneTorchBottomOff = {439, 62, 2, 2, 0},
    TextureAtlas::Rose = {448, 48, 16, 16, 0},
    TextureAtlas::RoseRed = {464, 48, 16, 16, 0},
    TextureAtlas::Sand = {480, 48, 16, 16, 0},
    TextureAtlas::OakSapling = {496, 48, 16, 16, 0},
    TextureAtlas::WheatSeeds = {256, 64, 16, 16, 0},
    TextureAtlas::Shears = {272, 64, 16, 16, 0},
    TextureAtlas::Slime = {288, 64, 16, 16, 0},
    TextureAtlas::Snow = {304, 64, 16, 16, 0},
    TextureAtlas::SnowBall = {320, 64, 16, 16, 0},
    TextureAtlas::SnowGrass = {336, 64, 16, 16, 0},
    TextureAtlas::SpruceLeaves = {352, 64, 16, 16, 0},
    TextureAtlas::SpruceLeavesBlocked = {336, 112, 16, 16, 0},
    TextureAtlas::SprucePlank = {368, 64, 16, 16, 0},
    TextureAtlas::SpruceSapling = {384, 64, 16, 16, 0},
    TextureAtlas::SpruceWood = {400, 64, 16, 16, 0},
    TextureAtlas::Stick = {416, 64, 16, 16, 0},
    TextureAtlas::Stone = {432, 64, 16, 16, 0},
    TextureAtlas::StoneAxe = {448, 64, 16, 16, 0},
    TextureAtlas::StoneButtonFace = {464, 64, 6, 4, 0},
    TextureAtlas::StoneButtonLongEdge = {464, 68, 6, 2, 0},
    TextureAtlas::StoneButtonShortEdge = {470, 64, 2, 4, 0},
    TextureAtlas::WoodButtonFace = {464, 72, 6, 4, 0},
    TextureAtlas::WoodButtonLongEdge = {464, 76, 6, 2, 0},
    TextureAtlas::WoodButtonShortEdge = {470, 72, 2, 4, 0},
    TextureAtlas::StoneHoe = {480, 64, 16, 16, 0},
    TextureAtlas::StonePickaxe = {496, 64, 16, 16, 0},
    TextureAtlas::StonePressurePlateFace = {256, 80, 16, 16, 0},
    TextureAtlas::StonePressurePlateSide = {272, 80, 16, 2, 0},
    TextureAtlas::StoneShovel = {288, 80, 16, 16, 0},
    TextureAtlas::StringItem = {304, 80, 16, 16, 0},
    TextureAtlas::TallGrass = {320, 80, 16, 16, 0},
    TextureAtlas::LavaSide0 = {336, 80, 16, 16, 0},
    TextureAtlas::LavaSide1 = {352, 80, 16, 16, 0},
    TextureAtlas::LavaSide2 = {368, 80, 16, 16, 0},
    TextureAtlas::LavaSide3 = {384, 80, 16, 16, 0},
    TextureAtlas::LavaSide4 = {400, 80, 16, 16, 0},
    TextureAtlas::LavaSide5 = {416, 80, 16, 16, 0},
    TextureAtlas::TNTSide = {432, 80, 16, 16, 0},
    TextureAtlas::TNTTop = {448, 80, 16, 16, 0},
    TextureAtlas::TNTBottom = {464, 80, 16, 16, 0},
    TextureAtlas::Blank = {480, 80, 16, 16, 0},
    TextureAtlas::TorchSide = {496, 80, 16, 16, 0},
    TextureAtlas::TorchTop = {503, 86, 2, 2, 0},
    TextureAtlas::TorchBottom = {503, 94, 2, 2, 0},
    TextureAtlas::Vines = {256, 96, 16, 16, 0},
    TextureAtlas::WaterBucket = {272, 96, 16, 16, 0},
    TextureAtlas::SpiderWeb = {288, 96, 16, 16, 0},
    TextureAtlas::WetFarmland = {304, 96, 16, 16, 0},
    TextureAtlas::Wheat0 = {320, 96, 16, 16, 0},
    TextureAtlas::Wheat1 = {336, 96, 16, 16, 0},
    TextureAtlas::Wheat2 = {352, 96, 16, 16, 0},
    TextureAtlas::Wheat3 = {368, 96, 16, 16, 0},
    TextureAtlas::Wheat4 = {384, 96, 16, 16, 0},
    TextureAtlas::Wheat5 = {400, 96, 16, 16, 0},
    TextureAtlas::Wheat6 = {416, 96, 16, 16, 0},
    TextureAtlas::Wheat7 = {432, 96, 16, 16, 0},
    TextureAtlas::OakWood = {448, 96, 16, 16, 0},
    TextureAtlas::WoodAxe = {464, 96, 16, 16, 0},
    TextureAtlas::WoodHoe = {480, 96, 16, 16, 0},
    TextureAtlas::WoodPickaxe = {496, 96, 16, 16, 0},
    TextureAtlas::WoodPressurePlateFace = {256, 112, 16, 16, 0},
    TextureAtlas::WoodPressurePlateSide = {272, 82, 16, 2, 0},
    TextureAtlas::WoodShovel = {272, 112, 16, 16, 0},
    TextureAtlas::Wool = {288, 112, 16, 16, 0},
    TextureAtlas::Selection = {288, 224, 32, 32, 0},
    TextureAtlas::Crosshairs = {256, 224, 12, 12, 0},
    TextureAtlas::Player1HeadFront = {0, 288, 8, 8, 0},
    TextureAtlas::Player1HeadBack = {24, 288, 8, 8, 0},
    TextureAtlas::Player1HeadLeft = {16, 288, 8, 8, 0},
    TextureAtlas::Player1HeadRight = {8, 288, 8, 8, 0},
    TextureAtlas::Player1HeadTop = {32, 288, 8, 8, 0},
    TextureAtlas::Player1HeadBottom = {40, 288, 8, 8, 0},
    TextureAtlas::InventoryUI = {0, 105, 170, 151, 1},
    TextureAtlas::WorkBenchUI = {0, 105, 170, 151, 2},
    TextureAtlas::ChestUI = {0, 105, 170, 151, 3},
    TextureAtlas::CreativeUI = {0, 105, 170, 151, 4},
    TextureAtlas::DispenserDropperUI = {0, 105, 170, 151, 5},
    TextureAtlas::FurnaceUI = {0, 105, 170, 151, 6},
    TextureAtlas::HopperUI = {0, 105, 170, 151, 7},
    TextureAtlas::DamageBarRed = {256, 222, 1, 2, 0},
    TextureAtlas::DamageBarYellow = {257, 222, 1, 2, 0},
    TextureAtlas::DamageBarGreen = {258, 222, 1, 2, 0},
    TextureAtlas::DamageBarGray = {259, 222, 1, 2, 0},
    TextureAtlas::Sun = {320, 224, 32, 32, 0},
    TextureAtlas::Moon0 = {352, 240, 16, 16, 0},
    TextureAtlas::Moon1 = {368, 240, 16, 16, 0},
    TextureAtlas::Moon2 = {384, 240, 16, 16, 0},
    TextureAtlas::Moon3 = {400, 240, 16, 16, 0},
    TextureAtlas::Moon4 = {416, 240, 16, 16, 0},
    TextureAtlas::Moon5 = {432, 240, 16, 16, 0},
    TextureAtlas::Moon6 = {448, 240, 16, 16, 0},
    TextureAtlas::Moon7 = {464, 240, 16, 16, 0};

const TextureAtlas &TextureAtlas::Fire(int index)
{
    assert(index >= 0 && index < FireFrameCount());
    switch(index)
    {
    case 0:
        return Fire0;
    case 1:
        return Fire1;
    case 2:
        return Fire2;
    case 3:
        return Fire3;
    case 4:
        return Fire4;
    case 5:
        return Fire5;
    case 6:
        return Fire6;
    case 7:
        return Fire7;
    case 8:
        return Fire8;
    case 9:
        return Fire9;
    case 10:
        return Fire10;
    case 11:
        return Fire11;
    case 12:
        return Fire12;
    case 13:
        return Fire13;
    case 14:
        return Fire14;
    case 15:
        return Fire15;
    case 16:
        return Fire16;
    case 17:
        return Fire17;
    case 18:
        return Fire18;
    case 19:
        return Fire19;
    case 20:
        return Fire20;
    case 21:
        return Fire21;
    case 22:
        return Fire22;
    case 23:
        return Fire23;
    case 24:
        return Fire24;
    case 25:
        return Fire25;
    case 26:
        return Fire26;
    case 27:
        return Fire27;
    case 28:
        return Fire28;
    case 29:
        return Fire29;
    case 30:
        return Fire30;
    default:
        return Fire31;
    }
}

int TextureAtlas::FireFrameCount()
{
    return 32;
}

const TextureAtlas &TextureAtlas::Delete(int index)
{
    assert(index >= 0 && index < DeleteFrameCount());
    switch(index)
    {
    case 0:
        return Delete0;
    case 1:
        return Delete1;
    case 2:
        return Delete2;
    case 3:
        return Delete3;
    case 4:
        return Delete4;
    case 5:
        return Delete5;
    case 6:
        return Delete6;
    case 7:
        return Delete7;
    case 8:
        return Delete8;
    default:
        return Delete9;
    }
}

int TextureAtlas::DeleteFrameCount()
{
    return 10;
}

const TextureAtlas &TextureAtlas::ParticleSmoke(int index)
{
    assert(index >= 0 && index < ParticleSmokeFrameCount());
    switch(index)
    {
    case 0:
        return ParticleSmoke0;
    case 1:
        return ParticleSmoke1;
    case 2:
        return ParticleSmoke2;
    case 3:
        return ParticleSmoke3;
    case 4:
        return ParticleSmoke4;
    case 5:
        return ParticleSmoke5;
    case 6:
        return ParticleSmoke6;
    default:
        return ParticleSmoke7;
    }
}

int TextureAtlas::ParticleSmokeFrameCount()
{
    return 8;
}

}
}
