#ifndef WORLD_GENERATOR_H_INCLUDED
#define WORLD_GENERATOR_H_INCLUDED

#include "world/world.h"
#include <memory>

using namespace std;

class WorldGenerator : public enable_shared_from_this<WorldGenerator>
{
protected:
    constexpr WorldGenerator()
    {
    }
public:
    virtual ~WorldGenerator()
    {
    }
    virtual void generateChunk(PositionI chunkBasePosition, World &world) const = 0;
};

#endif // WORLD_GENERATOR_H_INCLUDED
