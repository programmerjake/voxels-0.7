#ifndef WORLD_GENERATOR_H_INCLUDED
#define WORLD_GENERATOR_H_INCLUDED

#include "world/world.h"
#include <memory>

namespace programmerjake
{
namespace voxels
{
class WorldGenerator : public std::enable_shared_from_this<WorldGenerator>
{
protected:
    WorldGenerator()
    {
    }
public:
    virtual ~WorldGenerator()
    {
    }
    virtual void generateChunk(PositionI chunkBasePosition, World &world) const = 0;
};
}
}

#endif // WORLD_GENERATOR_H_INCLUDED
