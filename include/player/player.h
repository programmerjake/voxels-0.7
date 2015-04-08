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
#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "stream/stream.h"
#include "world/world.h"
#include "entity/entity.h"
#include "physics/physics.h"
#include "util/global_instance_maker.h"
#include "input/input.h"
#include <string>
#include <cassert>
#include <memory>
#include <atomic>
#include <mutex>
#include <iterator>
#include "util/intrusive_list.h"
#include "item/item_struct.h"
#include "entity/builtin/item.h"
#include "block/builtin/air.h"

namespace programmerjake
{
namespace voxels
{
class Player;
namespace ui
{
class GameUi;
class Ui;
}

namespace Entities
{
namespace builtin
{
class PlayerEntity final : public EntityDescriptor
{
    friend class global_instance_maker<PlayerEntity>;
private:
    Mesh head;
    Mesh body;
    Mesh leftArm;
    Mesh rightArm;
    Mesh leftLeg;
    Mesh rightLeg;
    void generateMeshes();
    PlayerEntity()
        : EntityDescriptor(L"builtin.player")
    {
        generateMeshes();
    }
    Player *getPlayer(Entity &entity) const
    {
        return static_cast<Player *>(entity.data.get());
    }
    virtual std::shared_ptr<PhysicsObject> makePhysicsObject(Entity &entity, World &world, PositionF position, VectorF velocity, std::shared_ptr<PhysicsWorld> physicsWorld) const override
    {
        return PhysicsObject::makeCylinder(position, velocity,
                                           true, false,
                                           0.3, 0.9,
                                           PhysicsProperties(PhysicsProperties::playerCollisionMask | PhysicsProperties::blockCollisionMask, PhysicsProperties::playerCollisionMask, 0, 1), physicsWorld);
    }
public:
    static const PlayerEntity *descriptor()
    {
        return global_instance_maker<PlayerEntity>::getInstance();
    }
    virtual void render(Entity &entity, Mesh &dest, RenderLayer rl, Matrix cameraToWorldMatrix) const override;
    virtual void moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const override;
    virtual Matrix getSelectionBoxTransform(const Entity &entity) const override
    {
        return Matrix::translate(-0.5f, -0.5f, -0.5f).concat(Matrix::scale(0.25, 1.8, 0.25)).concat(Matrix::translate(entity.physicsObject->getPosition()));
    }
    virtual void makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const override;
};
}
}

class Player final
{
    friend class Entities::builtin::PlayerEntity;
    friend class Players_t;
    Player(const Player &) = delete;
    Player &operator =(const Player &) = delete;
private:
    intrusive_list_members<Player> playersListMembers;
    void addToPlayersList();
    void removeFromPlayersList();
    PositionF lastPosition;
    std::shared_ptr<GameInput> gameInput;
    Entity *playerEntity;
    struct GameInputMonitoring
    {
        std::atomic_bool gotJump;
        std::atomic_int actionCount;
        std::atomic_int dropCount;
        std::atomic_bool gotAttack;
        GameInputMonitoring()
            : gotJump(false), actionCount(0), dropCount(0), gotAttack(false)
        {
        }
        bool retrieveGotJump()
        {
            return gotJump.exchange(false);
        }
        bool retrieveGotAttack()
        {
            return gotAttack.exchange(false);
        }
        int retrieveActionCount()
        {
            return actionCount.exchange(0);
        }
        int retrieveDropCount()
        {
            return dropCount.exchange(0);
        }
    };
    std::shared_ptr<GameInputMonitoring> gameInputMonitoring;
    ui::GameUi *const gameUi;
    std::atomic<float> &getBlockDestructProgress(); /// the amount that a block is destroyed (ex. 0 means just started, and 0.9 means almost done). negative numbers mean no block is being destroyed
    float destructingTime = 0, cooldownTimeLeft = 0;
    PositionI destructingPosition;
public:
    Entity *getPlayerEntity() const
    {
        return playerEntity;
    }
    const std::wstring name;
    std::size_t currentItemIndex = 0;
    ItemStackArray<9, 4> items;
    std::recursive_mutex itemsLock;
    bool setDialog(std::shared_ptr<ui::Ui> ui);
    Player(std::wstring name, std::shared_ptr<GameInput> gameInput, ui::GameUi *gameUi)
        : gameInput(gameInput), gameUi(gameUi), name(name)
    {
        assert(gameInput != nullptr);
        assert(gameUi != nullptr);
        gameInputMonitoring = std::make_shared<GameInputMonitoring>();
        gameInput->jump.onChange.bind([this](EventArguments&)
        {
            if(this->gameInput->jump.get())
                gameInputMonitoring->gotJump = true;
            return Event::ReturnType::Propagate;
        });
        gameInput->attack.onChange.bind([this](EventArguments&)
        {
            if(this->gameInput->attack.get())
                gameInputMonitoring->gotAttack = true;
            return Event::ReturnType::Propagate;
        });
        gameInput->action.bind([this](EventArguments&)
        {
            gameInputMonitoring->actionCount++;
            return Event::ReturnType::Propagate;
        });
        gameInput->drop.bind([this](EventArguments&)
        {
            gameInputMonitoring->dropCount++;
            return Event::ReturnType::Propagate;
        });
        gameInput->hotBarMoveLeft.bind([this](EventArguments&)
        {
            std::unique_lock<std::recursive_mutex> theLock(itemsLock);
            currentItemIndex = (currentItemIndex + items.itemStacks.size() - 1) % items.itemStacks.size();
            return Event::ReturnType::Propagate;
        });
        gameInput->hotBarMoveRight.bind([this](EventArguments&)
        {
            std::unique_lock<std::recursive_mutex> theLock(itemsLock);
            currentItemIndex = (currentItemIndex + 1) % items.itemStacks.size();
            return Event::ReturnType::Propagate;
        });
        gameInput->hotBarSelect.bind([this](EventArguments &argsIn)
        {
            std::unique_lock<std::recursive_mutex> theLock(itemsLock);
            HotBarSelectEventArguments &args = dynamic_cast<HotBarSelectEventArguments &>(argsIn);
            currentItemIndex = (args.newSelection % items.itemStacks.size() + items.itemStacks.size()) % items.itemStacks.size();
            return Event::ReturnType::Propagate;
        });
        addToPlayersList();
    }
    ~Player()
    {
        removeFromPlayersList();
    }
    Matrix getWorldOrientationXZTransform() const
    {
        return Matrix::rotateY(gameInput->viewTheta.get());
    }
    Matrix getWorldOrientationTransform() const
    {
        return Matrix::rotateX(gameInput->viewPhi.get()).concat(Matrix::rotateY(gameInput->viewTheta.get()));
    }
    Matrix getViewTransform() const
    {
        VectorF pos = lastPosition + VectorF(0, 0.6, 0);
        return Matrix::translate(-pos).concat(Matrix::rotateY(-gameInput->viewTheta.get())).concat(Matrix::rotateX(-gameInput->viewPhi.get()));
    }
    PositionF getPosition() const
    {
        return lastPosition;
    }
    VectorF getViewDirectionXZ() const
    {
        return transform(getWorldOrientationXZTransform(), VectorF(0, 0, -1));
    }
    VectorF getViewDirection() const
    {
        return transform(getWorldOrientationTransform(), VectorF(0, 0, -1));
    }
    BlockFace getViewDirectionXZBlockFaceOut() const
    {
        return getBlockFaceFromOutVector(getViewDirectionXZ());
    }
    BlockFace getViewDirectionBlockFaceOut() const
    {
        return getBlockFaceFromOutVector(getViewDirection());
    }
    BlockFace getViewDirectionXZBlockFaceIn() const
    {
        return getBlockFaceFromInVector(getViewDirectionXZ());
    }
    BlockFace getViewDirectionBlockFaceIn() const
    {
        return getBlockFaceFromInVector(getViewDirection());
    }
    RayCasting::Ray getViewRay() const
    {
        return transform(inverse(getViewTransform()), RayCasting::Ray(PositionF(0, 0, 0, getPosition().d), VectorF(0, 0, -1)));
    }
    RayCasting::Collision castRay(World &world, WorldLockManager &lock_manager, RayCasting::BlockCollisionMask rayBlockCollisionMask)
    {
        return world.castRay(getViewRay(), lock_manager, 10, rayBlockCollisionMask, playerEntity);
    }
    Entity *createDroppedItemEntity(ItemStack itemStack, World &world, WorldLockManager &lock_manager)
    {
        assert(itemStack.good());
        RayCasting::Ray ray = getViewRay();
        return ItemDescriptor::addToWorld(world, lock_manager, itemStack, ray.startPosition, normalize(ray.direction) * 6, this);
    }
    RayCasting::Collision getPlacedBlockPosition(World &world, WorldLockManager &lock_manager, RayCasting::BlockCollisionMask rayBlockCollisionMask = RayCasting::BlockCollisionMaskDefault)
    {
        RayCasting::Collision c = castRay(world, lock_manager, rayBlockCollisionMask);
        if(c.valid())
        {
            switch(c.type)
            {
            case RayCasting::Collision::Type::None:
                return RayCasting::Collision(world);
            case RayCasting::Collision::Type::Entity:
                return RayCasting::Collision(world);
            case RayCasting::Collision::Type::Block:
            {
                PositionI &pos = c.blockPosition;
                switch(c.blockFace)
                {
                case BlockFaceOrNone::NX:
                    pos.x--;
                    break;
                case BlockFaceOrNone::PX:
                    pos.x++;
                    break;
                case BlockFaceOrNone::NY:
                    pos.y--;
                    break;
                case BlockFaceOrNone::PY:
                    pos.y++;
                    break;
                case BlockFaceOrNone::NZ:
                    pos.z--;
                    break;
                case BlockFaceOrNone::PZ:
                    pos.z++;
                    break;
                default:
                    return RayCasting::Collision(world);
                }
                return c;
            }
            }
        }
        return RayCasting::Collision(world);
    }
    bool placeBlock(RayCasting::Collision collision, World &world, WorldLockManager &lock_manager, Block b);
    bool removeBlock(RayCasting::Collision collision, World &world, WorldLockManager &lock_manager, bool runBreakAction = false);
    unsigned addItem(Item item)
    {
        if(!item.good())
            return 1;
        std::unique_lock<std::recursive_mutex> theLock(itemsLock);
        unsigned addedCount = items.itemStacks[currentItemIndex][0].insert(item);
        if(addedCount > 0)
            return addedCount;
        return items.insert(item);
    }
    ItemStack &getSelectedItemStackReference()
    {
        return items.itemStacks[currentItemIndex][0];
    }
    const ItemStack getSelectedItemStack()
    {
        std::unique_lock<std::recursive_mutex> theLock(itemsLock);
        return items.itemStacks[currentItemIndex][0];
    }
    Item getSelectedItem()
    {
        return getSelectedItemStack().item;
    }
    Item removeSelectedItem()
    {
        std::unique_lock<std::recursive_mutex> theLock(itemsLock);
        Item retval = getSelectedItem();
        int removedCount = getSelectedItemStackReference().remove(retval);
        if(removedCount > 0)
            return retval;
        return Item();
    }
};

class LockedPlayers;

class Players_t final
{
    friend class Player;
    friend class LockedPlayers;
private:
    typedef intrusive_list<Player, &Player::playersListMembers> ListType;
    static ListType *pPlayers;
    static std::recursive_mutex playersLock;
    static void addPlayer(Player *player);
    static void removePlayer(Player *player);
    static ListType::iterator begin();
    static ListType::iterator end();
public:
    LockedPlayers lock();
};

class LockedPlayers final
{
    friend class Players_t;
private:
    std::unique_lock<std::recursive_mutex> theLock;
    LockedPlayers()
        : theLock(Players_t::playersLock)
    {
    }
public:
    typedef Players_t::ListType::iterator iterator;
    iterator begin() const
    {
        return Players_t::begin();
    }
    iterator end() const
    {
        return Players_t::end();
    }
    void unlock()
    {
        theLock.unlock();
    }
    void lock()
    {
        theLock.lock();
    }
    bool try_lock()
    {
        return theLock.try_lock();
    }
};

extern Players_t Players;
}
}

#endif // PLAYER_H_INCLUDED
