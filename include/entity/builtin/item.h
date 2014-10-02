#ifndef ITEM_H_INCLUDED
#define ITEM_H_INCLUDED

#include "entity/entity.h"
#include <unordered_map>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>

using namespace std;

namespace Entities
{
namespace builtin
{
class Item : public EntityDescriptor
{
protected:
    enum_array<Mesh, RenderLayer> meshes;
    static constexpr float baseSize = 0.5f, extraHeight = 0.1f;
    Item(wstring name, enum_array<Mesh, RenderLayer> meshes)
        : EntityDescriptor(name, PhysicsObjectConstructor::cylinderMaker(baseSize / 2, baseSize / 2 + extraHeight / 2, true, false, PhysicsProperties(PhysicsProperties::blockCollisionMask, PhysicsProperties::itemCollisionMask))), meshes(meshes)
    {
    }
    struct ItemData final
    {
        float angle = 0, bobPhase = 0;
        double timeLeft = 5 * 60;
        int8_t count = 1;
        ItemData()
        {
            minstd_rand generator((int)this);
            generator.discard(1000);
            uniform_real_distribution<float> distribution(0, 2 * M_PI);
            angle = distribution(generator);
            bobPhase = distribution(generator);
        }
    };
    static shared_ptr<ItemData> getItemData(const Entity &entity)
    {
        shared_ptr<ItemData> retval = static_pointer_cast<ItemData>(entity.data);
        if(retval == nullptr)
        {
            retval = shared_ptr<ItemData>(new ItemData);
        }
        return retval;
    }
    static shared_ptr<ItemData> getOrMakeItemData(Entity &entity)
    {
        shared_ptr<ItemData> retval = static_pointer_cast<ItemData>(entity.data);
        if(retval == nullptr)
        {
            retval = shared_ptr<ItemData>(new ItemData);
            entity.data = static_pointer_cast<void>(retval);
        }
        return retval;
    }
public:
    virtual void render(const Entity &entity, Mesh &dest, RenderLayer rl) const override
    {
        shared_ptr<ItemData> data = getItemData(entity);
        Matrix tform = Matrix::rotateY(data->angle);
        tform = tform.concat(Matrix::translate(0, extraHeight / 2 * std::sin(data->bobPhase), 0));
        dest.append(transform(tform.concat(Matrix::translate(entity.physicsObject->getPosition())), meshes[rl]));
    }
    virtual void moveStep(Entity &entity, World &world, double deltaTime) const override
    {
        shared_ptr<ItemData> data = getOrMakeItemData(entity);
        constexpr float angleSpeed = 2 * M_PI / 7.5f;
        constexpr float bobSpeed = 2.1f * angleSpeed;
        data->angle = std::fmod(data->angle + angleSpeed * (float)deltaTime, M_PI * 2);
        data->bobPhase = std::fmod(data->bobPhase + bobSpeed * (float)deltaTime, M_PI * 2);
        data->timeLeft -= deltaTime;
        if(data->timeLeft <= 0)
            entity.destroy();
        else
        {
            //cout << "Entity " << (void *)&entity << ": pos:" << (VectorF)entity.physicsObject->getPosition() << endl;
        }
    }
};
}
}

#endif // ITEM_H_INCLUDED
