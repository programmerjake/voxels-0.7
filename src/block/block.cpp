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
#include "block/block.h"
#include "item/builtin/tools/tool.h"

using namespace std;
namespace programmerjake
{
namespace voxels
{
const BlockDescriptor **BlockDescriptorIndex::blockDescriptorTable = nullptr;
std::size_t BlockDescriptorIndex::blockDescriptorTableSize = 0, BlockDescriptorIndex::blockDescriptorTableAllocated = 0;


BlockDescriptor::BlockDescriptor(std::wstring name, BlockShape blockShape, LightProperties lightProperties, RayCasting::BlockCollisionMask blockRayCollisionMask, bool isStaticMesh, bool isFaceBlockedNX, bool isFaceBlockedPX, bool isFaceBlockedNY, bool isFaceBlockedPY, bool isFaceBlockedNZ, bool isFaceBlockedPZ, Mesh meshCenter, Mesh meshFaceNX, Mesh meshFacePX, Mesh meshFaceNY, Mesh meshFacePY, Mesh meshFaceNZ, Mesh meshFacePZ, RenderLayer staticRenderLayer)
    : name(name), blockShape(blockShape), lightProperties(lightProperties), blockRayCollisionMask(blockRayCollisionMask), isStaticMesh(isStaticMesh), staticRenderLayer(staticRenderLayer)
{
    isFaceBlocked[BlockFace::NX] = isFaceBlockedNX;
    isFaceBlocked[BlockFace::PX] = isFaceBlockedPX;
    isFaceBlocked[BlockFace::NY] = isFaceBlockedNY;
    isFaceBlocked[BlockFace::PY] = isFaceBlockedPY;
    isFaceBlocked[BlockFace::NZ] = isFaceBlockedNZ;
    isFaceBlocked[BlockFace::PZ] = isFaceBlockedPZ;
    this->meshCenter = meshCenter;
    meshFace[BlockFace::NX] = meshFaceNX;
    meshFace[BlockFace::PX] = meshFacePX;
    meshFace[BlockFace::NY] = meshFaceNY;
    meshFace[BlockFace::PY] = meshFacePY;
    meshFace[BlockFace::NZ] = meshFaceNZ;
    meshFace[BlockFace::PZ] = meshFacePZ;
    bdIndex = BlockDescriptorIndex::addNewDescriptor(this);
    BlockDescriptors.add(this);
}

BlockDescriptor::~BlockDescriptor()
{
    BlockDescriptors.remove(this);
}

linked_map<wstring, BlockDescriptorPointer> *BlockDescriptors_t::blocksMap = nullptr;

void BlockDescriptors_t::add(BlockDescriptorPointer bd) const
{
    assert(bd != nullptr);
    if(blocksMap == nullptr)
        blocksMap = new linked_map<wstring, BlockDescriptorPointer>();
    bool wasInserted = std::get<1>(blocksMap->insert(make_pair(bd->name, bd)));
    assert(wasInserted);
}

void BlockDescriptors_t::remove(BlockDescriptorPointer bd) const
{
    assert(bd != nullptr);
    assert(blocksMap != nullptr);
    size_t removedCount = blocksMap->erase(bd->name);
    assert(removedCount == 1);
    if(blocksMap->empty())
    {
        delete blocksMap;
        blocksMap = nullptr;
    }
}

#ifdef PACK_BLOCK
PackedBlock::PackedBlock(const Block &b)
    : descriptor(b.descriptor ? b.descriptor->getBlockDescriptorIndex() : BlockDescriptorIndex(nullptr)), lighting(b.lighting), data(b.data)
{
}
#endif

void Block::createNewLighting(Lighting oldLighting)
{
    if(!good())
    {
        lighting = Lighting();
    }
    else
    {
        lighting = descriptor->lightProperties.createNewLighting(oldLighting);
    }
}

bool Block::operator ==(const Block &r) const
{
    if(descriptor != r.descriptor)
    {
        return false;
    }

    if(descriptor == nullptr)
    {
        return true;
    }

    if(lighting != r.lighting)
    {
        return false;
    }

    if(data == r.data)
    {
        return true;
    }

    return descriptor->isDataEqual(data, r.data);
}

BlockLighting Block::calcBlockLighting(BlockIterator bi, WorldLockManager &lock_manager, WorldLightingProperties wlp)
{
    checked_array<checked_array<checked_array<std::pair<LightProperties, Lighting>, 3>, 3>, 3> blocks;
    for(int x = 0; (size_t)x < blocks.size(); x++)
    {
        for(int y = 0; (size_t)y < blocks[x].size(); y++)
        {
            for(int z = 0; (size_t)z < blocks[x][y].size(); z++)
            {
                BlockIterator curBi = bi;
                curBi.moveBy(VectorI(x - 1, y - 1, z - 1));
                auto l = std::pair<LightProperties, Lighting>(LightProperties(Lighting(), Lighting::makeMaxLight()), Lighting());
                const Block &b = curBi.get(lock_manager);

                if(b)
                {
                    std::get<0>(l) = b.descriptor->lightProperties;
                    std::get<1>(l) = b.lighting;
                }

                blocks[x][y][z] = l;
            }
        }
    }

    return BlockLighting(blocks, wlp);
}

float BlockDescriptor::getBreakDuration(Item tool) const
{
    float hardness = getHardness();
    if(hardness < 0)
        return hardness;
    if(hardness == 0)
        return hardness;
    float baseSpeed = 1.5f * hardness;
    const Items::builtin::tools::Tool *toolDescriptor = dynamic_cast<const Items::builtin::tools::Tool *>(tool.descriptor);
    if(!isMatchingTool(tool))
        return baseSpeed * 10.0f / 3.0f;
    if(!isHelpingToolKind(tool))
        return baseSpeed;
    if(toolDescriptor == nullptr)
        return baseSpeed;
    return baseSpeed * toolDescriptor->getMineDurationFactor(tool);
}

bool BlockDescriptor::isMatchingTool(Item tool) const
{
    if(!isHelpingToolKind(tool))
        return getToolLevel() <= ToolLevel_None;
    ToolLevel toolLevel = ToolLevel_None;
    const Items::builtin::tools::Tool *toolDescriptor = dynamic_cast<const Items::builtin::tools::Tool *>(tool.descriptor);
    if(toolDescriptor != nullptr)
        toolLevel = toolDescriptor->getToolLevel();
    return getToolLevel() <= toolLevel;
}

void BlockDescriptor::handleToolDamage(Item &tool) const
{
    tool = Items::builtin::tools::Tool::incrementDamage(tool, this);
}
}
}

namespace std
{
size_t hash<programmerjake::voxels::Block>::operator()(const programmerjake::voxels::Block &b) const
{
    if(b.descriptor == nullptr)
    {
        return 0;
    }

    return std::hash<programmerjake::voxels::BlockDescriptorPointer>()(b.descriptor) + b.descriptor->hashData(b.data);
}
}
