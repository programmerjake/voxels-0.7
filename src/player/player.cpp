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
#include "player/player.h"
#include "render/generate.h"
#include "texture/texture_atlas.h"
#include "block/builtin/stone.h"
#include "block/builtin/air.h"
#include "item/item.h"
#include "ui/gameui.h"
#include "item/builtin/stone.h"
#include "entity/builtin/particles/smoke.h"
#include "item/builtin/tools/mineral_toolsets.h"
#include "item/builtin/minerals.h"

namespace programmerjake
{
namespace voxels
{
namespace Entities
{
namespace builtin
{
void PlayerEntity::generateMeshes()
{
    head = (Mesh)transform(Matrix::translate(-0.5, -0.5, -0.5).concat(Matrix::scale(0.5)), Generate::unitBox(TextureAtlas::Player1HeadLeft.td(), TextureAtlas::Player1HeadRight.td(), TextureAtlas::Player1HeadBottom.td(), TextureAtlas::Player1HeadTop.td(), TextureAtlas::Player1HeadBack.td(), TextureAtlas::Player1HeadFront.td()));
    #warning add rest of player
}
void PlayerEntity::render(Entity &entity, Mesh &dest, RenderLayer rl, Matrix cameraToWorldMatrix) const
{
    #warning implement
}
void PlayerEntity::moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const
{
    std::shared_ptr<Player> player = std::static_pointer_cast<Player>(entity.data);
    if(player == nullptr)
    {
        entity.destroy();
        return;
    }
    player->lastPosition = entity.physicsObject->getPosition();
    VectorF moveDirection = transform(player->getWorldOrientationXZTransform(), player->gameInput->moveDirectionPlayerRelative.get());
    VectorF newVelocity = entity.physicsObject->getVelocity();
    newVelocity.x = moveDirection.x;
    newVelocity.z = moveDirection.z;
    if(player->gameInputMonitoring->retrieveGotJump() || player->gameInput->jump.get())
    {
        if(entity.physicsObject->isSupported())
            newVelocity.y = std::max<float>(5, newVelocity.y);
    }
    entity.physicsObject->setCurrentState(player->lastPosition, newVelocity);
    if(player->gameInput->isCreativeMode.get())
    {

    }
    for(int i = player->gameInputMonitoring->retrieveActionCount(); i > 0; i--)
    {
        RayCasting::Collision c = player->castRay(world, lock_manager, RayCasting::BlockCollisionMaskDefault);
        if(c.valid())
        {
            switch(c.type)
            {
            case RayCasting::Collision::Type::None:
                break;
            case RayCasting::Collision::Type::Entity:
            {
                if(c.entity == nullptr || !c.entity->good())
                    break;
                if(c.entity->descriptor->onUse(*c.entity, world, lock_manager, player))
                    continue;
            }
            case RayCasting::Collision::Type::Block:
            {
                PositionI pos = c.blockPosition;
                bool good = true;
                BlockIterator bi = world.getBlockIterator(pos);
                Block b = bi.get(lock_manager);
                good = good && b.good();
                if(good)
                {
                    good = b.descriptor->onUse(world, b, bi, lock_manager, player);
                }
                if(good)
                    continue;
                break;
            }
            }
        }
        Item item = player->removeSelectedItem();
        if(item.good())
        {
            item = item.descriptor->onUse(item, world, lock_manager, *player);
            player->addItem(item);
        }
    }
    bool gotAttackDown = player->gameInputMonitoring->retrieveGotAttack();
    if(gotAttackDown || player->gameInput->attack.get())
    {
        player->destructingTime += deltaTime;
        if(player->cooldownTimeLeft > 0)
        {
            player->cooldownTimeLeft -= deltaTime;
            if(player->cooldownTimeLeft <= 0)
                player->cooldownTimeLeft = 0;
            player->destructingTime = 0;
        }
        RayCasting::Collision c = player->castRay(world, lock_manager, RayCasting::BlockCollisionMaskDefault);
        if(c.valid() && player->cooldownTimeLeft <= 0)
        {
            Item tool = player->removeSelectedItem();
            Item originalTool = tool;
            switch(c.type)
            {
            case RayCasting::Collision::Type::None:
                break;
            case RayCasting::Collision::Type::Entity:
                break;
            case RayCasting::Collision::Type::Block:
            {
                PositionI pos = c.blockPosition;
                if(pos != player->destructingPosition)
                {
                    player->destructingPosition = pos;
                    player->destructingTime = 0;
                }
                bool good = true;
                BlockIterator bi = world.getBlockIterator(pos);
                Block b = bi.get(lock_manager);
                good = good && b.good();
                if(good && gotAttackDown)
                {
                    good = b.descriptor->onStartAttack(world, b, bi, lock_manager, player);
                }
                if(good)
                {
                    float totalDestructTime = b.descriptor->getBreakDuration(tool);
                    if(totalDestructTime < 0)
                    {
                        good = false;
                        player->destructingTime = 0;
                        player->getBlockDestructProgress().store(-1.0f, std::memory_order_relaxed);
                    }
                    else if(player->destructingTime < totalDestructTime)
                    {
                        good = false;
                        player->getBlockDestructProgress().store(player->destructingTime / totalDestructTime, std::memory_order_relaxed);
                    }
                }
                else
                {
                    player->getBlockDestructProgress().store(-1.0f, std::memory_order_relaxed);
                    player->destructingTime = 0;
                }
                if(good)
                {
                    world.setBlock(bi, lock_manager, Block(Blocks::builtin::Air::descriptor()));
                    b.descriptor->onBreak(world, b, bi, lock_manager, tool);
                    for(int i = 0; i < 5; i++)
                    {
                        VectorF p = VectorF(std::uniform_real_distribution<float>(0, 1)(world.getRandomGenerator()),
                                            std::uniform_real_distribution<float>(0, 1)(world.getRandomGenerator()),
                                            std::uniform_real_distribution<float>(0, 1)(world.getRandomGenerator()));
                        Entities::builtin::particles::Smoke::addToWorld(world, lock_manager, bi.position() + p);
                    }
                    player->destructingTime = 0;
                    player->cooldownTimeLeft = 0.25f;
                    player->getBlockDestructProgress().store(-1.0f, std::memory_order_relaxed);
                }
                break;
            }
            }
            if(tool.good())
            {
                player->addItem(tool);
            }
        }
        else
        {
            player->getBlockDestructProgress().store(-1.0f, std::memory_order_relaxed);
            player->destructingTime = 0;
        }
    }
    else
    {
        player->destructingTime = 0;
        player->cooldownTimeLeft = 0;
        player->getBlockDestructProgress().store(-1.0f, std::memory_order_relaxed);
    }
    for(int i = player->gameInputMonitoring->retrieveDropCount(); i > 0; i--)
    {
        Item item = player->removeSelectedItem();
        if(item.good())
        {
            item.descriptor->dropAsEntity(item, world, lock_manager, *player);
        }
    }
    #warning implement
}
void PlayerEntity::makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const
{
    std::shared_ptr<Player> player = std::static_pointer_cast<Player>(entity.data);
    if(player == nullptr)
    {
        entity.destroy();
        return;
    }
    player->lastPosition = entity.physicsObject->getPosition();
    player->playerEntity = &entity;
}
}
}

void Player::addToPlayersList()
{
#ifdef DEBUG_VERSION
    for(int i = 0; i < 2; i++)
    {
        addItem(Item(Items::builtin::tools::DiamondToolset::pointer()->getAxe()));
        //addItem(Item(Items::builtin::tools::DiamondToolset::pointer()->getHoe()));
        addItem(Item(Items::builtin::tools::DiamondToolset::pointer()->getPickaxe()));
        addItem(Item(Items::builtin::tools::DiamondToolset::pointer()->getShovel()));
    }
    for(int i = 0; i < 25; i++)
    {
        addItem(Item(Items::builtin::Coal::descriptor()));
        addItem(Item(Items::builtin::RedstoneDust::descriptor()));
    }
#endif // DEBUG_VERSION
    Players.addPlayer(this);
}

void Player::removeFromPlayersList()
{
    Players.removePlayer(this);
}

bool Player::setDialog(std::shared_ptr<ui::Ui> ui)
{
    return gameUi->setDialog(ui);
}

bool Player::placeBlock(RayCasting::Collision collision, World &world, WorldLockManager &lock_manager, Block b)
{
    if(!b.good())
        return false;
    BlockIterator bi = world.getBlockIterator(collision.blockPosition);
    Block oldBlock = bi.get(lock_manager);
    if(!oldBlock.good())
        return false;
    if(oldBlock.descriptor->isReplaceable())
    {
        if(playerEntity->physicsObject->collidesWithBlock(b.descriptor->blockShape, collision.blockPosition))
            return false;
        oldBlock.descriptor->onReplace(world, oldBlock, bi, lock_manager);
        world.setBlock(bi, lock_manager, b);
        return true;
    }
    return false;
}

std::atomic<float> &Player::getBlockDestructProgress()
{
    return gameUi->blockDestructProgress;
}

bool Player::removeBlock(RayCasting::Collision collision, World &world, WorldLockManager &lock_manager, bool runBreakAction)
{
    PositionI pos = collision.blockPosition;
    bool good = true;
    BlockIterator bi = world.getBlockIterator(pos);
    Block b = bi.get(lock_manager);
    good = good && b.good();
    if(good)
    {
        world.setBlock(bi, lock_manager, Block(Blocks::builtin::Air::descriptor()));
        if(runBreakAction)
        {
            Item tool;
            b.descriptor->onBreak(world, b, bi, lock_manager, tool);
        }
        return true;
    }
    return false;
}

Players_t::ListType *Players_t::pPlayers = nullptr;
std::recursive_mutex Players_t::playersLock;
void Players_t::addPlayer(Player *player)
{
    std::unique_lock<std::recursive_mutex> theLock(playersLock);
    if(pPlayers == nullptr)
    {
        pPlayers = new ListType([](Player *)
        {
        });
    }
    pPlayers->push_front(player);
}
void Players_t::removePlayer(Player *player)
{
    std::unique_lock<std::recursive_mutex> theLock(playersLock);
    if(pPlayers == nullptr)
    {
        pPlayers = new ListType([](Player *)
        {
        });
    }
    pPlayers->erase(pPlayers->to_iterator(player));
}
Players_t::ListType::iterator Players_t::begin()
{
    std::unique_lock<std::recursive_mutex> theLock(playersLock);
    if(pPlayers == nullptr)
    {
        pPlayers = new ListType([](Player *)
        {
        });
    }
    return pPlayers->begin();
}
Players_t::ListType::iterator Players_t::end()
{
    std::unique_lock<std::recursive_mutex> theLock(playersLock);
    if(pPlayers == nullptr)
    {
        pPlayers = new ListType([](Player *)
        {
        });
    }
    return pPlayers->end();
}
LockedPlayers Players_t::lock()
{
    return LockedPlayers();
}
Players_t Players;
}
}
