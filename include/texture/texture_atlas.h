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
#ifndef TEXTURE_ATLAS_H_INCLUDED
#define TEXTURE_ATLAS_H_INCLUDED

#include "texture/texture_descriptor.h"
#include "util/checked_array.h"
#include <tuple>

namespace programmerjake
{
namespace voxels
{
class TextureAtlas final
{
    struct ImageDescriptor final
    {
        Image image;
        const wchar_t *const fileName;
        const int width, height;
        constexpr ImageDescriptor(const wchar_t *fileName, int width, int height)
            : image(), fileName(fileName), width(width), height(height)
        {
        }
        ImageDescriptor(const ImageDescriptor &) = default;
        ImageDescriptor &operator =(const ImageDescriptor &) = delete;
    };
	static checked_array<ImageDescriptor, 8> &textures();
    class TextureLoader final
    {
    public:
        TextureLoader();
    };
    static TextureLoader textureLoader;
public:
    static Image texture(std::size_t textureIndex);
	const int left, top, width, height;
	std::size_t textureIndex;
    float minU() const
    {
        return (left + pixelOffset) / textureXRes(textureIndex);
    }
    float maxU() const
    {
        return (left + width - pixelOffset) / textureXRes(textureIndex);
    }
    float minV() const
    {
        return 1 - (top + height - pixelOffset) / textureYRes(textureIndex);
    }
    float maxV() const
    {
        return 1 - (top + pixelOffset) / textureYRes(textureIndex);
    }
	static int textureXRes(std::size_t textureIndex)
	{
	    return textures()[textureIndex].width;
	}
	static int textureYRes(std::size_t textureIndex)
	{
	    return textures()[textureIndex].height;
	}
    static constexpr float pixelOffset = 0.05f;
	constexpr TextureAtlas(int left, int top, int width, int height, std::size_t textureIndex = 0)
        : left(left), top(top), width(width), height(height), textureIndex(textureIndex)
	{
	}
	const TextureAtlas & operator =(const TextureAtlas &) = delete;
	TextureDescriptor td() const
	{
		return TextureDescriptor(texture(textureIndex), minU(), maxU(), minV(), maxV());
	}
	TextureDescriptor tdNoOffset() const
	{
	    return TextureDescriptor(texture(textureIndex), (float)left / textureXRes(textureIndex), (float)(left + width) / textureXRes(textureIndex), 1 - (float)(top + height) / textureYRes(textureIndex), 1 - (float)top / textureYRes(textureIndex));
	}
	static const TextureAtlas &Fire(int index);
	static int FireFrameCount();
	static const TextureAtlas &Delete(int index);
	static int DeleteFrameCount();
	static const TextureAtlas &ParticleSmoke(int index);
	static int ParticleSmokeFrameCount();
	static const TextureAtlas
	ActivatorRailOff,
    ActivatorRailOn,
    BedHead,
    BedFoot,
    BedTopSide,
    BedTopLeftSide,
    BedTopRightSide,
    BedBottom,
    BedBottomLeftSide,
    BedBottomRightSide,
    BedBottomSide,
    BedItem,
    Font8x8,
    Shockwave,
    Bedrock,
    BirchLeaves,
    BirchLeavesBlocked,
    BirchPlank,
    BirchSapling,
    BirchWood,
    WoodEnd,
    BlazePowder,
    BlazeRod,
    Bone,
    BoneMeal,
    Bow,
    BrownMushroom,
    Bucket,
    CactusSide,
    CactusBottom,
    CactusTop,
    CactusGreen,
    ChestSide,
    ChestTop,
    ChestFront,
    Cobblestone,
    Coal,
    CoalOre,
    CocoaSmallSide,
    CocoaSmallTop,
    CocoaSmallStem,
    CocoaMediumSide,
    CocoaMediumTop,
    CocoaMediumStem,
    CocoaLargeSide,
    CocoaLargeTop,
    CocoaLargeStem,
    CocoaBeans,
    DeadBush,
    DandelionYellow,
    Dandelion,
    CyanDye,
    Delete0,
    Delete1,
    Delete2,
    Delete3,
    Delete4,
    Delete5,
    Delete6,
    Delete7,
    Delete8,
    Delete9,
    DetectorRailOff,
    DetectorRailOn,
    Diamond,
    DiamondAxe,
    DiamondHoe,
    DiamondOre,
    DiamondPickaxe,
    DiamondShovel,
    Dirt,
    DirtMask,
    DispenserSide,
    DispenserTop,
    DropperSide,
    DropperTop,
    DispenserDropperPistonFurnaceFrame,
    Emerald,
    EmeraldOre,
    PistonBaseSide,
    PistonBaseTop,
    PistonHeadSide,
    PistonHeadFace,
    PistonHeadBase,
    StickyPistonHeadFace,
    FarmlandSide,
    FarmlandTop,
    Fire0,
    Fire1,
    Fire2,
    Fire3,
    Fire4,
    Fire5,
    Fire6,
    Fire7,
    Fire8,
    Fire9,
    Fire10,
    Fire11,
    Fire12,
    Fire13,
    Fire14,
    Fire15,
    Fire16,
    Fire17,
    Fire18,
    Fire19,
    Fire20,
    Fire21,
    Fire22,
    Fire23,
    Fire24,
    Fire25,
    Fire26,
    Fire27,
    Fire28,
    Fire29,
    Fire30,
    Fire31,
    Flint,
    FlintAndSteel,
    FurnaceFrontOff,
    FurnaceSide,
    FurnaceFrontOn,
    WorkBenchTop,
    WorkBenchSide0,
    WorkBenchSide1,
    Glass,
    GoldAxe,
    GoldHoe,
    GoldIngot,
    GoldOre,
    GoldPickaxe,
    GoldShovel,
    GrassMask,
    GrassTop,
    Gravel,
    GrayDye,
    Gunpowder,
    HopperRim,
    HopperInside,
    HopperSide,
    HopperBigBottom,
    HopperMediumBottom,
    HopperSmallBottom,
    HopperItem,
    HotBarBox,
    InkSac,
    IronAxe,
    IronHoe,
    IronIngot,
    IronOre,
    IronPickaxe,
    IronShovel,
    JungleLeaves,
    JungleLeavesBlocked,
    JunglePlank,
    JungleSapling,
    JungleWood,
    Ladder,
    LapisLazuli,
    LapisLazuliOre,
    WheatItem,
    LavaBucket,
    OakLeaves,
    OakLeavesBlocked,
    LeverBaseBigSide,
    LeverBaseSmallSide,
    LeverBaseTop,
    LeverHandleSide,
    LeverHandleTop,
    LeverHandleBottom,
    LightBlueDye,
    LightGrayDye,
    LimeDye,
    MagentaDye,
    MinecartInsideSide,
    MinecartInsideBottom,
    MinecartItem,
    MinecartOutsideLeftRight,
    MinecartOutsideFrontBack,
    MinecartOutsideBottom,
    MinecartOutsideTop,
    MinecartWithChest,
    MinecartWithHopper,
    MinecartWithTNT,
    MobSpawner,
    OrangeDye,
    WaterSide0,
    WaterSide1,
    WaterSide2,
    WaterSide3,
    WaterSide4,
    WaterSide5,
    Obsidian,
    ParticleSmoke0,
    ParticleSmoke1,
    ParticleSmoke2,
    ParticleSmoke3,
    ParticleSmoke4,
    ParticleSmoke5,
    ParticleSmoke6,
    ParticleSmoke7,
    ParticleFire0,
    ParticleFire1,
    PinkDye,
    PinkStone,
    PistonShaft,
    OakPlank,
    PoweredRailOff,
    PoweredRailOn,
    PurpleDye,
    PurplePortal,
    Quartz,
    Rail,
    RailCurve,
    RedMushroom,
    BlockOfRedstone,
    RedstoneComparatorOff,
    RedstoneComparatorOn,
    RedstoneComparatorRepeatorSide,
    RedstoneShortTorchSideOn,
    RedstoneShortTorchSideOff,
    RedstoneDust0,
    RedstoneDust1,
    RedstoneDust2Corner,
    RedstoneDust2Across,
    RedstoneDust3,
    RedstoneDust4,
    RedstoneDustItem,
    RedstoneRepeatorBarSide,
    RedstoneRepeatorBarTopBottom,
    RedstoneRepeatorBarEnd,
    RedstoneOre,
    ActiveRedstoneOre,
    RedstoneRepeatorOff,
    RedstoneRepeatorOn,
    RedstoneTorchSideOn,
    RedstoneTorchSideOff,
    RedstoneTorchTopOn,
    RedstoneTorchTopOff,
    RedstoneTorchBottomOn,
    RedstoneTorchBottomOff,
    Rose,
    RoseRed,
    Sand,
    OakSapling,
    WheatSeeds,
    Shears,
    Slime,
    Snow,
    SnowBall,
    SnowGrass,
    SpruceLeaves,
    SpruceLeavesBlocked,
    SprucePlank,
    SpruceSapling,
    SpruceWood,
    Stick,
    Stone,
    StoneAxe,
    StoneButtonFace,
    StoneButtonLongEdge,
    StoneButtonShortEdge,
    WoodButtonFace,
    WoodButtonLongEdge,
    WoodButtonShortEdge,
    StoneHoe,
    StonePickaxe,
    StonePressurePlateFace,
    StonePressurePlateSide,
    StoneShovel,
    StringItem,
    TallGrass,
    LavaSide0,
    LavaSide1,
    LavaSide2,
    LavaSide3,
    LavaSide4,
    LavaSide5,
    TNTSide,
    TNTTop,
    TNTBottom,
    Blank,
    TorchSide,
    TorchTop,
    TorchBottom,
    Vines,
    WaterBucket,
    SpiderWeb,
    WetFarmland,
    Wheat0,
    Wheat1,
    Wheat2,
    Wheat3,
    Wheat4,
    Wheat5,
    Wheat6,
    Wheat7,
    OakWood,
    WoodAxe,
    WoodHoe,
    WoodPickaxe,
    WoodPressurePlateFace,
    WoodPressurePlateSide,
    WoodShovel,
    Wool,
    Selection,
    Crosshairs,
    Player1HeadFront,
    Player1HeadBack,
    Player1HeadLeft,
    Player1HeadRight,
    Player1HeadTop,
    Player1HeadBottom,
    InventoryUI,
    WorkBenchUI,
    ChestUI,
    CreativeUI,
    DispenserDropperUI,
    FurnaceUI,
    HopperUI,
    DamageBarRed,
    DamageBarYellow,
    DamageBarGreen,
    DamageBarGray,
    Sun,
    Moon0,
    Moon1,
    Moon2,
    Moon3,
    Moon4,
    Moon5,
    Moon6,
    Moon7;
};
}
}

#endif // TEXTURE_ATLAS_H_INCLUDED
