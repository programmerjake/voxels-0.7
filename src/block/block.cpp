/*
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

using namespace std;
namespace programmerjake
{
namespace voxels
{
const BlockDescriptor **BlockDescriptorIndex::blockDescriptorTable = nullptr;
std::size_t BlockDescriptorIndex::blockDescriptorTableSize = 0, BlockDescriptorIndex::blockDescriptorTableAllocated = 0;


BlockDescriptor::BlockDescriptor(std::wstring name, BlockShape blockShape, LightProperties lightProperties, bool isStaticMesh, bool isFaceBlockedNX, bool isFaceBlockedPX, bool isFaceBlockedNY, bool isFaceBlockedPY, bool isFaceBlockedNZ, bool isFaceBlockedPZ, Mesh meshCenter, Mesh meshFaceNX, Mesh meshFacePX, Mesh meshFaceNY, Mesh meshFacePY, Mesh meshFaceNZ, Mesh meshFacePZ, RenderLayer staticRenderLayer)
    : name(name), blockShape(blockShape), lightProperties(lightProperties), isStaticMesh(isStaticMesh), staticRenderLayer(staticRenderLayer)
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
}
}
