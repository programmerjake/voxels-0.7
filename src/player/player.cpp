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
#include "item/builtin/sand.h"

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
    head = transform(Matrix::translate(-0.5, -0.5, -0.5).concat(Matrix::scale(0.5)), Generate::unitBox(TextureAtlas::Player1HeadLeft.td(), TextureAtlas::Player1HeadRight.td(), TextureAtlas::Player1HeadBottom.td(), TextureAtlas::Player1HeadTop.td(), TextureAtlas::Player1HeadBack.td(), TextureAtlas::Player1HeadFront.td()));
    #warning add rest of player
}
void PlayerEntity::render(Entity &entity, Mesh &dest, RenderLayer rl, Matrix cameraToWorldMatrix) const
{
    #warning implement
}
void PlayerEntity::moveStep(Entity &entity, World &world, WorldLockManager &lock_manager, double deltaTime) const
{
    std::shared_ptr<Player> player = getPlayer(entity);
    if(player == nullptr)
    {
        entity.destroy();
        return;
    }
    PositionF lastPosition = entity.physicsObject->getPosition();
    BlockEffects blockEffects = entity.physicsObject->getBlockEffects();
    {
        std::unique_lock<std::recursive_mutex> lockIt(player->lastPositionLock);
        player->lastPosition = lastPosition;
    }
    VectorF moveDirection = transform(player->getWorldOrientationXZTransform(), player->gameInput->moveDirectionPlayerRelative.get());
    VectorF newVelocity = entity.physicsObject->getVelocity();
    float moveSpeed = 4.317f;
    VectorF gravity = VectorF(0, -32, 0);
    if(player->gameInput->sneak.get())
        moveSpeed = 1.295f;
    bool jumpDown = player->gameInput->jump.get();
    bool sneakDown = player->gameInput->sneak.get();
    if(blockEffects.canSwim)
    {
        gravity = VectorF(0, -1, 0);
        moveSpeed = 2;
        if(jumpDown && !sneakDown)
        {
            gravity.y = 6;
        }
        else if(sneakDown)
        {
            gravity.y = -6;
        }
    }
    if(blockEffects.canClimb)
    {
        float minYVelocity = -2;
        moveSpeed = 2;
        if(sneakDown)
            minYVelocity = 0;
        if(newVelocity.y <= minYVelocity)
        {
            newVelocity.y = minYVelocity;
            gravity = 0;
        }
        if(moveDirection != VectorF(0))
        {
            newVelocity.y = moveSpeed;
        }
    }
    if(player->gameInputMonitoring->retrieveGotJump() || jumpDown)
    {
        if(entity.physicsObject->isSupported())
            newVelocity.y = std::max<float>(9, newVelocity.y);
    }
    if(player->gameInput->isCreativeMode.get())
    {
        if(player->gameInput->fly.get())
        {
            moveSpeed = 10.89f;
            newVelocity.y = 0;
            gravity = VectorF(0);
            if(jumpDown && !sneakDown)
            {
                newVelocity.y = 7.5f;
            }
            else if(sneakDown)
            {
                newVelocity.y = -7.5f;
            }
        }
    }
    newVelocity.x = moveDirection.x * moveSpeed;
    newVelocity.z = moveDirection.z * moveSpeed;
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
                        player->setBlockDestructProgress(-1.0f);
                    }
                    else if(player->destructingTime < totalDestructTime)
                    {
                        good = false;
                        player->setBlockDestructProgress(player->destructingTime / totalDestructTime);
                    }
                }
                else
                {
                    player->setBlockDestructProgress(-1.0f);
                    player->destructingTime = 0;
                }
                if(good)
                {
                    world.setBlock(bi, lock_manager, Block(Blocks::builtin::Air::descriptor(), b.lighting));
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
                    player->setBlockDestructProgress(-1.0f);
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
            player->setBlockDestructProgress(-1.0f);
            player->destructingTime = 0;
        }
    }
    else
    {
        player->destructingTime = 0;
        player->cooldownTimeLeft = 0;
        player->setBlockDestructProgress(-1.0f);
    }
    for(int i = player->gameInputMonitoring->retrieveDropCount(); i > 0; i--)
    {
        Item item = player->removeSelectedItem();
        if(item.good())
        {
            item.descriptor->dropAsEntity(item, world, lock_manager, *player);
        }
    }
    entity.physicsObject->setCurrentState(lastPosition, newVelocity, gravity);
    #warning implement
}
void PlayerEntity::makeData(Entity &entity, World &world, WorldLockManager &lock_manager) const
{
    std::shared_ptr<Player> player = getPlayer(entity);
    if(player == nullptr)
    {
        entity.destroy();
        return;
    }
    player->playerEntity = &entity;
    std::unique_lock<std::recursive_mutex> lockIt(player->lastPositionLock);
    player->lastPosition = entity.physicsObject->getPosition();
}
Entity *PlayerEntity::addToWorld(World &world, WorldLockManager &lock_manager, PositionF position, std::shared_ptr<Player> player, VectorF velocity)
{
    return world.addEntity(descriptor(), position, velocity, lock_manager, std::static_pointer_cast<void>(std::make_shared<std::weak_ptr<Player>>(player)));
}
void PlayerEntity::write(const Entity &entity, stream::Writer &writer) const
{
    PositionF position = entity.physicsObject->getPosition();
    VectorF velocity = entity.physicsObject->getVelocity();
    std::shared_ptr<Player> player = getPlayer(entity);
    stream::write<PositionF>(writer, position);
    stream::write<VectorF>(writer, velocity);
    Player::writeReference(writer, player);
}
Entity *PlayerEntity::read(stream::Reader &reader) const
{
    PositionF position = stream::read<PositionF>(reader);
    VectorF velocity = stream::read<VectorF>(reader);
    std::shared_ptr<Player> player = Player::readReference(reader);
    if(player == nullptr)
        throw stream::InvalidDataValueException("read empty player");
    World::StreamWorld streamWorld = World::getStreamWorld(reader);
    assert(streamWorld);
    if(player->playerEntity)
        throw stream::InvalidDataValueException("player already has entity");
    return streamWorld.world().addEntity(this, position, velocity, streamWorld.lock_manager(), std::static_pointer_cast<void>(std::make_shared<std::weak_ptr<Player>>(player)));
}
}
}

void Player::addToPlayersList()
{
    world.players().addPlayer(shared_from_this());
}

std::shared_ptr<Player> Player::make(std::wstring name, ui::GameUi *gameUi, std::shared_ptr<World> pworld)
{
    auto retval = std::shared_ptr<Player>(new Player(name, gameUi, pworld));
#ifdef DEBUG_VERSION
    for(int i = 0; i < 1; i++)
    {
        retval->addItem(Item(Items::builtin::tools::DiamondToolset::pointer()->getAxe()));
        //retval->addItem(Item(Items::builtin::tools::DiamondToolset::pointer()->getHoe()));
        retval->addItem(Item(Items::builtin::tools::DiamondToolset::pointer()->getPickaxe()));
        retval->addItem(Item(Items::builtin::tools::DiamondToolset::pointer()->getShovel()));
    }
    for(int i = 0; i < 25; i++)
    {
        retval->addItem(Item(Items::builtin::Coal::descriptor()));
        retval->addItem(Item(Items::builtin::Sand::descriptor()));
        retval->addItem(Item(Items::builtin::Gravel::descriptor()));
        retval->addItem(Item(Items::builtin::RedstoneDust::descriptor()));
    }
#endif // DEBUG_VERSION
    retval->addToPlayersList();
    return retval;
}


void Player::removeFromPlayersList()
{
    if(worldW.lock())
        world.players().removePlayer(name);
}

bool Player::setDialog(std::shared_ptr<ui::Ui> ui)
{
    if(!gameUi)
        return false;
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
        b.lighting = oldBlock.lighting;
        world.setBlock(bi, lock_manager, b);
        return true;
    }
    return false;
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
        world.setBlock(bi, lock_manager, Block(Blocks::builtin::Air::descriptor(), b.lighting));
        if(runBreakAction)
        {
            Item tool;
            b.descriptor->onBreak(world, b, bi, lock_manager, tool);
        }
        return true;
    }
    return false;
}

void PlayerList::addPlayer(std::shared_ptr<Player> player)
{
    std::unique_lock<std::recursive_mutex> theLock(playersLock);
    assert(players.count(player->name) == 0);
    players.emplace(player->name, player);
}

void PlayerList::removePlayer(std::wstring name)
{
    std::unique_lock<std::recursive_mutex> theLock(playersLock);
    players.erase(name);
}

void Player::writeReference(stream::Writer &writer, std::shared_ptr<Player> player)
{
    std::wstring name = L"";
    if(player)
        name = player->name;
    stream::write<std::wstring>(writer, name);
}

std::shared_ptr<Player> Player::readReference(stream::Reader &reader)
{
    std::wstring name = stream::read<std::wstring>(reader);
    if(name.empty())
        return nullptr;
    World::StreamWorld streamWorld = World::getStreamWorld(reader);
    assert(streamWorld);
    LockedPlayers lockedPlayers = streamWorld.world().players().lock();
    auto iter = streamWorld.world().players().players.find(name);
    if(iter == streamWorld.world().players().players.end())
        throw stream::InvalidDataValueException("unknown player name");
    return std::get<1>(*iter);
}

void Player::write(stream::Writer &writer)
{
    PositionF lastPosition;
    {
        std::unique_lock<std::recursive_mutex> lockIt(lastPositionLock);
        lastPosition = this->lastPosition;
    }
    stream::write<PositionF>(writer, lastPosition);
    stream::write<std::wstring>(writer, name);
    std::unique_lock<std::recursive_mutex> lockIt(itemsLock);
    ItemStackArray<9, 4> itemsCopy = items;
    std::size_t currentItemIndexCopy = currentItemIndex;
    lockIt.unlock();
    stream::write<std::uint8_t>(writer, static_cast<std::uint8_t>(currentItemIndexCopy));
    stream::write<ItemStackArray<9, 4>>(writer, itemsCopy);
}

std::shared_ptr<Player> Player::read(stream::Reader &reader)
{
    PositionF lastPosition = stream::read<PositionF>(reader);
    std::wstring name = stream::read<std::wstring>(reader);
    if(name.empty())
        throw stream::InvalidDataValueException("empty player name");
    ItemStackArray<9, 4> items;
    std::size_t currentItemIndex = static_cast<std::uint8_t>(stream::read_limited<std::uint8_t>(reader, 0, items.itemStacks.size() - 1));
    items = stream::read<ItemStackArray<9, 4>>(reader);
    World::StreamWorld streamWorld = World::getStreamWorld(reader);
    assert(streamWorld);
    {
        LockedPlayers lockedPlayers = streamWorld.world().players().lock();
        if(streamWorld.world().players().players.count(name) > 0)
            throw stream::InvalidDataValueException("duplicated player name");
    }
    std::shared_ptr<Player> retval = std::shared_ptr<Player>(new Player(name, nullptr, streamWorld.shared_world()));
    retval->currentItemIndex = currentItemIndex;
    retval->items = items;
    retval->lastPosition = lastPosition;
    retval->addToPlayersList();
    return retval;
}

Player::Player(std::wstring name, ui::GameUi *gameUi, std::shared_ptr<World> pworld)
    : lastPosition(),
    lastPositionLock(),
    playerEntity(nullptr),
    gameInputMonitoring(),
    gameUi(gameUi),
    destructingPosition(),
    world(*pworld),
    worldW(pworld),
    gameInput(std::make_shared<GameInput>()),
    name(name),
    items(),
    itemsLock()
{
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
}

void Player::setBlockDestructProgress(float v)
{
    if(gameUi)
        gameUi->blockDestructProgress.store(v, std::memory_order_relaxed);
}

}
}
