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
#ifndef SCRIPT_NODES_H_INCLUDED
#define SCRIPT_NODES_H_INCLUDED

#include "script/script.h"
#include "util/util.h"

namespace programmerjake
{
namespace voxels
{
namespace Scripting
{
template<std::uint32_t size, typename ChildClass>
struct NodeConstArgCount : public Node
{
    NodeConstArgCount()
        : args()
    {
    }
    friend struct Node;
    checked_array<std::uint32_t, size> args;
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        std::shared_ptr<ChildClass> retval = std::make_shared<ChildClass>();
        for(std::uint32_t &v : retval->args)
        {
            v = reader.readLimitedU32(0, nodeCount - 1);
        }
        return std::static_pointer_cast<Node>(retval);
    }
public:
    virtual void write(stream::Writer &writer) const override
    {
        stream::write<Type>(writer, type());
        for(std::uint32_t v : args)
        {
            writer.writeU32(v);
        }
    }
};
struct NodeConst final : public Node
{
    friend struct Node;
    std::shared_ptr<Data> data;
    NodeConst(std::shared_ptr<Data> data)
        : data(data)
    {
    }
    virtual Type type() const override
    {
        return Type::Const;
    }
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t)
    {
        return std::make_shared<NodeConst>(Data::read(reader));
    }
public:
    virtual void write(stream::Writer &writer) const override
    {
        stream::write<Type>(writer, type());
        data->write(writer);
    }
    virtual std::shared_ptr<Data> evaluate(State &, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return data;
    }
};
struct NodeLoadGlobals final : public Node
{
    friend struct Node;
    virtual Type type() const override
    {
        return Type::LoadGlobals;
    }
protected:
    static std::shared_ptr<Node> read(stream::Reader &, std::uint32_t)
    {
        return std::make_shared<NodeLoadGlobals>();
    }
public:
    virtual void write(stream::Writer &writer) const override
    {
        stream::write<Type>(writer, type());
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return state.variables;
    }
};
struct NodeCast : public Node
{
    checked_array<std::uint32_t, 1> args;
    friend struct Node;
    NodeCast()
        : args()
    {
    }
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount, std::shared_ptr<NodeCast> retval)
    {
        for(std::uint32_t &v : retval->args)
        {
            v = reader.readLimitedU32(0, nodeCount - 1);
        }
        return std::static_pointer_cast<Node>(retval);
    }
public:
    virtual void write(stream::Writer &writer) const override
    {
        stream::write<Type>(writer, type());
        for(std::uint32_t v : args)
        {
            writer.writeU32(v);
        }
    }
};
struct NodeCastToString final : public NodeCast
{
    virtual Type type() const override
    {
        return Type::CastToString;
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        std::shared_ptr<Data> retval = state.nodes[args[0]]->evaluate(state, stackDepth + 1);
        return std::make_shared<DataString>((std::wstring) * retval);
    }
    friend struct Node;
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        return NodeCast::read(reader, nodeCount, std::make_shared<NodeCastToString>());
    }
};
struct NodeCastToInteger final : public NodeCast
{
    virtual Type type() const override
    {
        return Type::CastToInteger;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Integer)
        {
            return retval;
        }
        if(retval->type() == Data::Type::Float)
        {
            return std::make_shared<DataInteger>((std::int32_t)std::dynamic_pointer_cast<DataFloat>(retval)->value);
        }
        throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to integer");
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
    friend struct Node;
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        return NodeCast::read(reader, nodeCount, std::make_shared<NodeCastToInteger>());
    }
};
struct NodeCastToFloat final : public NodeCast
{
    virtual Type type() const override
    {
        return Type::CastToFloat;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Float)
        {
            return retval;
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::make_shared<DataFloat>((float)std::dynamic_pointer_cast<DataInteger>(retval)->value);
        }
        throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to float");
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
    friend struct Node;
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        return NodeCast::read(reader, nodeCount, std::make_shared<NodeCastToFloat>());
    }
};
struct NodeCastToVector final : public NodeCast
{
    virtual Type type() const override
    {
        return Type::CastToVector;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Vector)
        {
            return retval;
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::make_shared<DataVector>(VectorF((float)std::dynamic_pointer_cast<DataInteger>(retval)->value));
        }
        if(retval->type() == Data::Type::Float)
        {
            return std::make_shared<DataVector>(VectorF(std::dynamic_pointer_cast<DataFloat>(retval)->value));
        }
        if(retval->type() == Data::Type::List)
        {
            DataList *list = dynamic_cast<DataList *>(retval.get());
            if(list->value.size() != 3)
            {
                throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to vector");
            }
            VectorF v;
            v.x = std::dynamic_pointer_cast<DataFloat>(NodeCastToFloat::evaluate(list->value[0]))->value;
            v.y = std::dynamic_pointer_cast<DataFloat>(NodeCastToFloat::evaluate(list->value[1]))->value;
            v.z = std::dynamic_pointer_cast<DataFloat>(NodeCastToFloat::evaluate(list->value[2]))->value;
            return std::shared_ptr<Data>(new DataVector(v));
        }
        throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to vector");
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
    friend struct Node;
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        return NodeCast::read(reader, nodeCount, std::make_shared<NodeCastToVector>());
    }
};
struct NodeCastToMatrix final : public NodeCast
{
    virtual Type type() const override
    {
        return Type::CastToMatrix;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Matrix)
        {
            return retval;
        }
        if(retval->type() == Data::Type::List)
        {
            DataList *list = dynamic_cast<DataList *>(retval.get());
            if(list->value.size() != 16)
            {
                throw ScriptException(L"type cast error : can't cast " + retval->typeString() + L" to matrix");
            }
            Matrix v;
            for(int i = 0, y = 0; y < 4; y++)
            {
                for(int x = 0; x < 4; x++, i++)
                {
                    v.set(x, y, std::dynamic_pointer_cast<DataFloat>(NodeCastToFloat::evaluate(list->value[i]))->value);
                }
            }
            return std::shared_ptr<Data>(new DataMatrix(v));
        }
        throw ScriptException(std::make_shared<DataString>(L"type cast error : can't cast " + retval->typeString() + L" to matrix"));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
    friend struct Node;
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        return NodeCast::read(reader, nodeCount, std::make_shared<NodeCastToMatrix>());
    }
};
struct NodeCastToList final : public NodeCast
{
    virtual Type type() const override
    {
        return Type::CastToList;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::List)
        {
            return retval;
        }
        if(retval->type() == Data::Type::Vector)
        {
            VectorF value = dynamic_cast<DataVector *>(retval.get())->value;
            std::shared_ptr<DataList> retval2 = std::make_shared<DataList>();
            retval2->value.push_back(std::make_shared<DataFloat>(value.x));
            retval2->value.push_back(std::make_shared<DataFloat>(value.y));
            retval2->value.push_back(std::make_shared<DataFloat>(value.z));
            return std::static_pointer_cast<Data>(retval2);
        }
        if(retval->type() == Data::Type::Matrix)
        {
            Matrix value = dynamic_cast<DataMatrix *>(retval.get())->value;
            std::shared_ptr<DataList> retval2 = std::make_shared<DataList>();
            for(int y = 0; y < 4; y++)
            {
                for(int x = 0; x < 4; x++)
                {
                    retval2->value.push_back(std::make_shared<DataFloat>(value.get(x, y)));
                }
            }
            return std::static_pointer_cast<Data>(retval2);
        }
        if(retval->type() == Data::Type::String)
        {
            std::wstring value = std::dynamic_pointer_cast<DataString>(retval)->value;
            std::shared_ptr<DataList> retval2 = std::make_shared<DataList>();
            retval2->value.reserve(value.size());
            for(std::size_t i = 0; i < value.size(); i++)
            {
                retval2->value.push_back(std::make_shared<DataString>(std::wstring(L"") + value[i]));
            }
            return retval2;
        }
        throw ScriptException(std::make_shared<DataString>(L"type cast error : can't cast " + retval->typeString() + L" to list"));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
    friend struct Node;
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        return NodeCast::read(reader, nodeCount, std::make_shared<NodeCastToList>());
    }
};
struct NodeCastToObject final : public NodeCast
{
    virtual Type type() const override
    {
        return Type::CastToObject;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Object)
        {
            return retval;
        }
        throw ScriptException(std::make_shared<DataString>(L"type cast error : can't cast " + retval->typeString() + L" to object"));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
    friend struct Node;
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        return NodeCast::read(reader, nodeCount, std::make_shared<NodeCastToObject>());
    }
};
struct NodeCastToBoolean final : public NodeCast
{
    virtual Type type() const override
    {
        return Type::CastToBoolean;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Boolean)
        {
            return retval;
        }
        throw ScriptException(std::make_shared<DataString>(L"type cast error : can't cast " + retval->typeString() + L" to boolean"));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
    friend struct Node;
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        return NodeCast::read(reader, nodeCount, std::make_shared<NodeCastToBoolean>());
    }
};
struct NodeReadIndex final : public NodeConstArgCount<2, NodeReadIndex>
{
    virtual Type type() const override
    {
        return Type::ReadIndex;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        if(arg2->type() == Data::Type::String)
        {
            std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
            if(value == L"dup")
            {
                return arg1->dup();
            }
        }
        if(arg1->type() == Data::Type::Vector)
        {
            if(arg2->type() == Data::Type::Integer)
            {
                switch(dynamic_cast<DataInteger *>(arg2.get())->value)
                {
                case 0:
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.x));
                case 1:
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.y));
                case 2:
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.z));
                default:
                    throw ScriptException(L"index out of range");
                }
            }
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                if(value == L"x")
                {
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.x));
                }
                if(value == L"y")
                {
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.y));
                }
                if(value == L"z")
                {
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.z));
                }
                if(value == L"length")
                {
                    return std::shared_ptr<Data>(new DataInteger(3));
                }
                throw ScriptException(L"variable doesn't exist : " + value);
            }
            throw ScriptException(L"illegal type for index");
        }
        if(arg1->type() == Data::Type::Matrix)
        {
            if(arg2->type() == Data::Type::Integer)
            {
                std::int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                if(value < 0 || value >= 16)
                {
                    throw ScriptException(L"index out of range");
                }
                return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataMatrix *>(arg1.get())->value.get(value % 4, value / 4)));
            }
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                if(value == L"length")
                {
                    return std::shared_ptr<Data>(new DataInteger(16));
                }
                throw ScriptException(L"variable doesn't exist : " + value);
            }
            throw ScriptException(L"illegal type for index");
        }
        if(arg1->type() == Data::Type::List)
        {
            const std::vector<std::shared_ptr<Data>> &list = dynamic_cast<DataList *>(arg1.get())->value;
            if(arg2->type() == Data::Type::Integer)
            {
                std::int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                if(value < 0 || (std::size_t)value >= list.size())
                {
                    throw ScriptException(L"index out of range");
                }
                return list[value];
            }
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                if(value == L"length")
                {
                    return std::shared_ptr<Data>(new DataInteger(list.size()));
                }
                throw ScriptException(L"variable doesn't exist : " + value);
            }
            throw ScriptException(L"illegal type for index");
        }
        if(arg1->type() == Data::Type::Object)
        {
            const std::unordered_map<std::wstring, std::shared_ptr<Data>> &map = dynamic_cast<DataObject *>(arg1.get())->value;
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                auto i = map.find(value);
                if(i == map.end())
                {
                    throw ScriptException(L"variable doesn't exist : " + value);
                }
                return std::get<1>(*i);
            }
            throw ScriptException(L"illegal type for index");
        }
        if(arg1->type() == Data::Type::String)
        {
            std::wstring str = dynamic_cast<DataString *>(arg1.get())->value;
            if(arg2->type() == Data::Type::Integer)
            {
                std::int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                if(value < 0 || (std::size_t)value >= str.size())
                {
                    throw ScriptException(L"index out of range");
                }
                return std::shared_ptr<Data>(new DataString(str.substr(value, 1)));
            }
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                if(value == L"length")
                {
                    return std::shared_ptr<Data>(new DataInteger(str.size()));
                }
                throw ScriptException(L"variable doesn't exist : " + value);
            }
            throw ScriptException(L"illegal type for index");
        }
        throw ScriptException(L"invalid type to index");
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1), state.nodes[args[1]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeAssignIndex final : public NodeConstArgCount<3, NodeAssignIndex>
{
    virtual Type type() const override
    {
        return Type::AssignIndex;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2, std::shared_ptr<Data> arg3)
    {
        if(arg1->type() == Data::Type::Vector)
        {
            if(arg2->type() == Data::Type::Integer)
            {
                std::int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                if(value < 0 || value >= 3)
                {
                    throw ScriptException(L"index out of range");
                }
                if(arg3->type() != Data::Type::Float)
                {
                    throw ScriptException(L"can't assign " + arg3->typeString() + L" to float");
                }
                float newValue = dynamic_cast<DataFloat *>(arg3.get())->value;
                switch(value)
                {
                case 0:
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.x = newValue));
                case 1:
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.y = newValue));
                case 2:
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.z = newValue));
                default:
                    UNREACHABLE();
                }
            }
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                if(value != L"x" && value != L"y" && value != L"z")
                {
                    throw ScriptException(L"can't write to " + value);
                }
                if(arg3->type() != Data::Type::Float)
                {
                    throw ScriptException(L"can't assign " + arg3->typeString() + L" to a float");
                }
                float newValue = dynamic_cast<DataFloat *>(arg3.get())->value;
                if(value == L"x")
                {
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.x = newValue));
                }
                if(value == L"y")
                {
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.y = newValue));
                }
                if(value == L"z")
                {
                    return std::shared_ptr<Data>(new DataFloat(dynamic_cast<DataVector *>(arg1.get())->value.z = newValue));
                }
                if(value == L"length")
                {
                    return std::shared_ptr<Data>(new DataInteger(3));
                }
                throw ScriptException(L"variable doesn't exist : " + value);
            }
            throw ScriptException(L"illegal type for index");
        }
        if(arg1->type() == Data::Type::Matrix)
        {
            if(arg2->type() == Data::Type::Integer)
            {
                std::int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                if(arg3->type() != Data::Type::Float)
                {
                    throw ScriptException(L"can't assign " + arg3->typeString() + L" to a float");
                }
                float newValue = dynamic_cast<DataFloat *>(arg3.get())->value;
                if(value < 0 || value >= 16)
                {
                    throw ScriptException(L"index out of range");
                }
                dynamic_cast<DataMatrix *>(arg1.get())->value.set(value % 4, value / 4, newValue);
                return std::shared_ptr<Data>(new DataFloat(newValue));
            }
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                throw ScriptException(L"can't write to " + value);
            }
            throw ScriptException(L"illegal type for index");
        }
        if(arg1->type() == Data::Type::List)
        {
            std::vector<std::shared_ptr<Data>> &list = dynamic_cast<DataList *>(arg1.get())->value;
            if(arg2->type() == Data::Type::Integer)
            {
                std::int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                if(value < 0 || (std::size_t)value >= list.size())
                {
                    throw ScriptException(L"index out of range");
                }
                return list[value] = arg3->dup();
            }
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                throw ScriptException(L"can't write to " + value);
            }
            throw ScriptException(L"illegal type for index");
        }
        if(arg1->type() == Data::Type::Object)
        {
            std::unordered_map<std::wstring, std::shared_ptr<Data>> &map = dynamic_cast<DataObject *>(arg1.get())->value;
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                return map[value] = arg3->dup();
            }
            throw ScriptException(L"illegal type for index");
        }
        if(arg1->type() == Data::Type::String)
        {
            std::wstring &str = dynamic_cast<DataString *>(arg1.get())->value;
            if(arg2->type() == Data::Type::Integer)
            {
                std::int32_t value = dynamic_cast<DataInteger *>(arg2.get())->value;
                if(value < 0 || (std::size_t)value >= str.size())
                {
                    throw ScriptException(L"index out of range");
                }
                str = str.substr(0, value) + (std::wstring) * arg3 + str.substr(value + 1);
            }
            if(arg2->type() == Data::Type::String)
            {
                std::wstring value = dynamic_cast<DataString *>(arg2.get())->value;
                throw ScriptException(L"can't write to " + value);
            }
            throw ScriptException(L"illegal type for index");
        }
        throw ScriptException(L"invalid type to index");
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1), state.nodes[args[1]]->evaluate(state, stackDepth + 1), state.nodes[args[2]]->evaluate(state, stackDepth + 1));
    }
};
template <typename ChildClass>
struct NodeBinaryArithmatic : public NodeConstArgCount<2, ChildClass>
{
    static std::shared_ptr<Data> toData(float v)
    {
        return std::shared_ptr<Data>(new DataFloat(v));
    }
    static std::shared_ptr<Data> toData(std::int32_t v)
    {
        return std::shared_ptr<Data>(new DataInteger(v));
    }
    static std::shared_ptr<Data> toData(VectorF v)
    {
        return std::shared_ptr<Data>(new DataVector(v));
    }
    static std::shared_ptr<Data> toData(Matrix v)
    {
        return std::shared_ptr<Data>(new DataMatrix(v));
    }
    static std::shared_ptr<Data> toData(bool v)
    {
        return std::shared_ptr<Data>(new DataBoolean(v));
    }
    static std::int32_t throwError()
    {
        throw ScriptException(L"invalid types for " + ChildClass::operatorString());
    }
    static std::shared_ptr<Data> evaluateBackup(std::shared_ptr<Data>, std::shared_ptr<Data>)
    {
        throwError();
        return nullptr;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        if(arg1->type() == Data::Type::Float)
        {
            auto v1 = dynamic_cast<DataFloat *>(arg1.get())->value;
            if(arg2->type() == Data::Type::Float)
            {
                auto v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Integer)
            {
                auto v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Vector)
            {
                auto v2 = dynamic_cast<DataVector *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Matrix)
            {
                auto v2 = dynamic_cast<DataMatrix *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
        }
        if(arg1->type() == Data::Type::Integer)
        {
            auto v1 = dynamic_cast<DataInteger *>(arg1.get())->value;
            if(arg2->type() == Data::Type::Float)
            {
                auto v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Integer)
            {
                auto v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Vector)
            {
                auto v2 = dynamic_cast<DataVector *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Matrix)
            {
                auto v2 = dynamic_cast<DataMatrix *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
        }
        if(arg1->type() == Data::Type::Vector)
        {
            auto v1 = dynamic_cast<DataVector *>(arg1.get())->value;
            if(arg2->type() == Data::Type::Float)
            {
                auto v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Integer)
            {
                auto v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Vector)
            {
                auto v2 = dynamic_cast<DataVector *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Matrix)
            {
                auto v2 = dynamic_cast<DataMatrix *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
        }
        if(arg1->type() == Data::Type::Matrix)
        {
            auto v1 = dynamic_cast<DataMatrix *>(arg1.get())->value;
            if(arg2->type() == Data::Type::Float)
            {
                auto v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Integer)
            {
                auto v2 = dynamic_cast<DataInteger *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Vector)
            {
                auto v2 = dynamic_cast<DataVector *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
            if(arg2->type() == Data::Type::Matrix)
            {
                auto v2 = dynamic_cast<DataMatrix *>(arg2.get())->value;
                return toData(ChildClass::evalFn(v1, v2));
            }
        }
        if(arg1->type() == Data::Type::Boolean && arg2->type() == Data::Type::Boolean)
        {
            auto v1 = dynamic_cast<DataBoolean *>(arg1.get())->value;
            auto v2 = dynamic_cast<DataBoolean *>(arg2.get())->value;
            return toData(ChildClass::evalFn(v1, v2));
        }
        return ChildClass::evaluateBackup(arg1, arg2);
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        Node::checkStackDepth(stackDepth);
        return evaluate(state.nodes[NodeConstArgCount<2, ChildClass>::args[0]]->evaluate(state, stackDepth + 1), state.nodes[NodeConstArgCount<2, ChildClass>::args[1]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeAdd : public NodeBinaryArithmatic<NodeAdd>
{
    static std::wstring operatorString()
    {
        return L"+";
    }
    virtual Type type() const override
    {
        return Type::Add;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static VectorF evalFn(VectorF a, VectorF b)
    {
        return a + b;
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static float evalFn(float a, float b)
    {
        return a + b;
    }
    static std::int32_t evalFn(std::int32_t a, std::int32_t b)
    {
        return a + b;
    }
    static float evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static float evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeSub : public NodeBinaryArithmatic<NodeSub>
{
    static std::wstring operatorString()
    {
        return L"-";
    }
    virtual Type type() const override
    {
        return Type::Sub;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static VectorF evalFn(VectorF a, VectorF b)
    {
        return a - b;
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static float evalFn(float a, float b)
    {
        return a - b;
    }
    static std::int32_t evalFn(std::int32_t a, std::int32_t b)
    {
        return a - b;
    }
    static float evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static float evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeMul : public NodeBinaryArithmatic<NodeMul>
{
    static std::wstring operatorString()
    {
        return L"*";
    }
    virtual Type type() const override
    {
        return Type::Mul;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static VectorF evalFn(VectorF a, VectorF b)
    {
        return a - b;
    }
    static VectorF evalFn(float a, VectorF b)
    {
        return a * b;
    }
    static VectorF evalFn(VectorF a, float b)
    {
        return a * b;
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static float evalFn(float a, float b)
    {
        return a * b;
    }
    static std::int32_t evalFn(std::int32_t a, std::int32_t b)
    {
        return a * b;
    }
    static float evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static float evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeDiv : public NodeBinaryArithmatic<NodeDiv>
{
    static std::wstring operatorString()
    {
        return L"/";
    }
    virtual Type type() const override
    {
        return Type::Div;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static VectorF evalFn(VectorF a, VectorF b)
    {
        if(b.x == 0 || b.y == 0 || b.z == 0)
        {
            throw ScriptException(L"divide by zero");
        }
        return a / b;
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static VectorF evalFn(VectorF a, float b)
    {
        if(b == 0)
        {
            throw ScriptException(L"divide by zero");
        }
        return a / b;
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static float evalFn(float a, float b)
    {
        if(b == 0)
        {
            throw ScriptException(L"divide by zero");
        }
        return a / b;
    }
    static std::int32_t evalFn(std::int32_t a, std::int32_t b)
    {
        if(b == 0)
        {
            throw ScriptException(L"divide by zero");
        }
        return a / b;
    }
    static float evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static float evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeMod : public NodeBinaryArithmatic<NodeMod>
{
    static std::wstring operatorString()
    {
        return L"%";
    }
    virtual Type type() const override
    {
        return Type::Mod;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static float evalFn(float a, float b)
    {
        if(b == 0)
        {
            throw ScriptException(L"divide by zero");
        }
        return fmod(a, b);
    }
    static std::int32_t evalFn(std::int32_t a, std::int32_t b)
    {
        if(b == 0)
        {
            throw ScriptException(L"divide by zero");
        }
        return a % b;
    }
    static float evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static float evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodePow : public NodeBinaryArithmatic<NodePow>
{
    static std::wstring operatorString()
    {
        return L"**";
    }
    virtual Type type() const override
    {
        return Type::Pow;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static float evalFn(float a, float b)
    {
        if(b != (int)b && a <= 0)
        {
            throw ScriptException(L"domain error");
        }
        if(b <= 0 && a <= 0)
        {
            throw ScriptException(L"domain error");
        }
        return pow(a, b);
    }
    static float evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static float evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
    static float evalFn(std::int32_t a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeAnd : public NodeBinaryArithmatic<NodeAnd>
{
    static std::wstring operatorString()
    {
        return L"and";
    }
    virtual Type type() const override
    {
        return Type::And;
    }
    static bool evalFn(bool a, bool b)
    {
        return a && b;
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(std::int32_t a, std::int32_t b)
    {
        return a & b;
    }
    static std::int32_t evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::int32_t evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeOr : public NodeBinaryArithmatic<NodeOr>
{
    static std::wstring operatorString()
    {
        return L"or";
    }
    virtual Type type() const override
    {
        return Type::Or;
    }
    static bool evalFn(bool a, bool b)
    {
        return a || b;
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(std::int32_t a, std::int32_t b)
    {
        return a | b;
    }
    static std::int32_t evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::int32_t evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeXor : public NodeBinaryArithmatic<NodeXor>
{
    static std::wstring operatorString()
    {
        return L"xor";
    }
    virtual Type type() const override
    {
        return Type::Xor;
    }
    static bool evalFn(bool a, bool b)
    {
        return a != b;
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(std::int32_t a, std::int32_t b)
    {
        return a ^ b;
    }
    static std::int32_t evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::int32_t evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeConcat : public NodeBinaryArithmatic<NodeConcat>
{
    static std::wstring operatorString()
    {
        return L"~";
    }
    virtual Type type() const override
    {
        return Type::Concat;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static Matrix evalFn(Matrix a, Matrix b)
    {
        return a.concat(b);
    }
    static VectorF evalFn(Matrix a, VectorF b)
    {
        return a.apply(b);
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(std::int32_t, std::int32_t) // error case
    {
        return throwError();
    }
    static std::shared_ptr<Data> evaluateBackup(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        if(arg1->type() == Data::Type::String || arg2->type() == Data::Type::String)
        {
            return std::shared_ptr<Data>(new DataString((std::wstring)*arg1 + (std::wstring)*arg2));
        }
        if(arg1->type() == Data::Type::List || arg2->type() == Data::Type::List)
        {
            std::shared_ptr<DataList> list = std::make_shared<DataList>();
            for(std::shared_ptr<Data> v : dynamic_cast<DataList *>(arg1.get())->value)
            {
                list->value.push_back(v->dup());
            }
            for(std::shared_ptr<Data> v : dynamic_cast<DataList *>(arg2.get())->value)
            {
                list->value.push_back(v->dup());
            }
            return std::static_pointer_cast<Data>(list);
        }
        throwError();
        return nullptr;
    }
    static std::int32_t evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::int32_t evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeDot : public NodeBinaryArithmatic<NodeDot>
{
    static std::wstring operatorString()
    {
        return L"dot";
    }
    virtual Type type() const override
    {
        return Type::Dot;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static float evalFn(VectorF a, VectorF b)
    {
        return dot(a, b);
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(std::int32_t, std::int32_t) // error case
    {
        return throwError();
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeCross : public NodeBinaryArithmatic<NodeCross>
{
    static std::wstring operatorString()
    {
        return L"cross";
    }
    virtual Type type() const override
    {
        return Type::Cross;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static VectorF evalFn(VectorF a, VectorF b)
    {
        return cross(a, b);
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(std::int32_t, std::int32_t) // error case
    {
        return throwError();
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
};
struct NodeEqual : public NodeBinaryArithmatic<NodeEqual>
{
    static std::wstring operatorString()
    {
        return L"==";
    }
    virtual Type type() const override
    {
        return Type::Equal;
    }
    static bool evalFn(bool a, bool b)
    {
        return a == b;
    }
    static bool evalFn(VectorF a, VectorF b)
    {
        return a == b;
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static bool evalFn(Matrix a, Matrix b)
    {
        return a == b;
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static bool evalFn(float a, float b)
    {
        return a == b;
    }
    static bool evalFn(std::int32_t a, std::int32_t b)
    {
        return a == b;
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::shared_ptr<Data> evaluateBackup(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
        {
            return std::shared_ptr<Data>(new DataBoolean((std::wstring)*arg1 == (std::wstring)*arg2));
        }
        if(arg1->type() == Data::Type::List && arg2->type() == Data::Type::List)
        {
            const std::vector<std::shared_ptr<Data>> &a = dynamic_cast<DataList *>(arg1.get())->value, &b = dynamic_cast<DataList *>(arg2.get())->value;
            if(a.size() != b.size())
            {
                return std::shared_ptr<Data>(new DataBoolean(false));
            }
            for(std::size_t i = 0; i < a.size(); i++)
            {
                if(a[i]->type() != b[i]->type())
                {
                    return std::shared_ptr<Data>(new DataBoolean(false));
                }
                if(!std::dynamic_pointer_cast<DataBoolean>(NodeEqual::evaluate(a[i], b[i]))->value)
                {
                    return std::shared_ptr<Data>(new DataBoolean(false));
                }
            }
            return std::shared_ptr<Data>(new DataBoolean(true));
        }
        if(arg1->type() == Data::Type::Object && arg2->type() == Data::Type::Object)
        {
            const std::unordered_map<std::wstring, std::shared_ptr<Data>> &a = dynamic_cast<DataObject *>(arg1.get())->value, &b = dynamic_cast<DataObject *>(arg2.get())->value;
            if(a.size() != b.size())
            {
                return std::shared_ptr<Data>(new DataBoolean(false));
            }
            for(std::pair<std::wstring, std::shared_ptr<Data>> v : a)
            {
                std::shared_ptr<Data> va = std::get<1>(v);
                auto iter = b.find(std::get<0>(v));
                if(iter == b.end())
                {
                    return std::shared_ptr<Data>(new DataBoolean(false));
                }
                std::shared_ptr<Data> vb = std::get<1>(*iter);
                if(va->type() != vb->type())
                {
                    return std::shared_ptr<Data>(new DataBoolean(false));
                }
                if(!std::dynamic_pointer_cast<DataBoolean>(NodeEqual::evaluate(va, vb))->value)
                {
                    return std::shared_ptr<Data>(new DataBoolean(false));
                }
            }
            return std::shared_ptr<Data>(new DataBoolean(true));
        }
        throwError();
        return nullptr;
    }
};
struct NodeNotEqual : public NodeBinaryArithmatic<NodeNotEqual>
{
    static std::wstring operatorString()
    {
        return L"!=";
    }
    virtual Type type() const override
    {
        return Type::NotEqual;
    }
    static bool evalFn(bool a, bool b)
    {
        return a != b;
    }
    static bool evalFn(VectorF a, VectorF b)
    {
        return a != b;
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static bool evalFn(Matrix a, Matrix b)
    {
        return a != b;
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static bool evalFn(float a, float b)
    {
        return a != b;
    }
    static bool evalFn(std::int32_t a, std::int32_t b)
    {
        return a != b;
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::shared_ptr<Data> evaluateBackup(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
        {
            return std::shared_ptr<Data>(new DataBoolean((std::wstring)*arg1 != (std::wstring)*arg2));
        }
        if(arg1->type() == Data::Type::List && arg2->type() == Data::Type::List)
        {
            const std::vector<std::shared_ptr<Data>> &a = dynamic_cast<DataList *>(arg1.get())->value, &b = dynamic_cast<DataList *>(arg2.get())->value;
            if(a.size() != b.size())
            {
                return std::shared_ptr<Data>(new DataBoolean(true));
            }
            for(std::size_t i = 0; i < a.size(); i++)
            {
                if(a[i]->type() != b[i]->type())
                {
                    return std::shared_ptr<Data>(new DataBoolean(true));
                }
                if(!std::dynamic_pointer_cast<DataBoolean>(NodeEqual::evaluate(a[i], b[i]))->value)
                {
                    return std::shared_ptr<Data>(new DataBoolean(true));
                }
            }
            return std::shared_ptr<Data>(new DataBoolean(false));
        }
        if(arg1->type() == Data::Type::Object && arg2->type() == Data::Type::Object)
        {
            const std::unordered_map<std::wstring, std::shared_ptr<Data>> &a = dynamic_cast<DataObject *>(arg1.get())->value, &b = dynamic_cast<DataObject *>(arg2.get())->value;
            if(a.size() != b.size())
            {
                return std::shared_ptr<Data>(new DataBoolean(true));
            }
            for(std::pair<std::wstring, std::shared_ptr<Data>> v : a)
            {
                std::shared_ptr<Data> va = std::get<1>(v);
                auto iter = b.find(std::get<0>(v));
                if(iter == b.end())
                {
                    return std::shared_ptr<Data>(new DataBoolean(true));
                }
                std::shared_ptr<Data> vb = std::get<1>(*iter);
                if(va->type() != vb->type())
                {
                    return std::shared_ptr<Data>(new DataBoolean(true));
                }
                if(!std::dynamic_pointer_cast<DataBoolean>(NodeEqual::evaluate(va, vb))->value)
                {
                    return std::shared_ptr<Data>(new DataBoolean(true));
                }
            }
            return std::shared_ptr<Data>(new DataBoolean(false));
        }
        throwError();
        return nullptr;
    }
};
struct NodeLessThan : public NodeBinaryArithmatic<NodeLessThan>
{
    static std::wstring operatorString()
    {
        return L"<";
    }
    virtual Type type() const override
    {
        return Type::LessThan;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static bool evalFn(float a, float b)
    {
        return a < b;
    }
    static bool evalFn(std::int32_t a, std::int32_t b)
    {
        return a < b;
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::shared_ptr<Data> evaluateBackup(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
        {
            return std::shared_ptr<Data>(new DataBoolean((std::wstring)*arg1 < (std::wstring)*arg2));
        }
        throwError();
        return nullptr;
    }
};
struct NodeGreaterThan : public NodeBinaryArithmatic<NodeGreaterThan>
{
    static std::wstring operatorString()
    {
        return L">";
    }
    virtual Type type() const override
    {
        return Type::GreaterThan;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static bool evalFn(float a, float b)
    {
        return a > b;
    }
    static bool evalFn(std::int32_t a, std::int32_t b)
    {
        return a > b;
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::shared_ptr<Data> evaluateBackup(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
        {
            return std::shared_ptr<Data>(new DataBoolean((std::wstring)*arg1 > (std::wstring)*arg2));
        }
        throwError();
        return nullptr;
    }
};
struct NodeLessEqual : public NodeBinaryArithmatic<NodeLessEqual>
{
    static std::wstring operatorString()
    {
        return L"<=";
    }
    virtual Type type() const override
    {
        return Type::LessEqual;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static bool evalFn(float a, float b)
    {
        return a <= b;
    }
    static bool evalFn(std::int32_t a, std::int32_t b)
    {
        return a <= b;
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::shared_ptr<Data> evaluateBackup(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
        {
            return std::shared_ptr<Data>(new DataBoolean((std::wstring)*arg1 <= (std::wstring)*arg2));
        }
        throwError();
        return nullptr;
    }
};
struct NodeGreaterEqual : public NodeBinaryArithmatic<NodeGreaterEqual>
{
    static std::wstring operatorString()
    {
        return L">=";
    }
    virtual Type type() const override
    {
        return Type::GreaterEqual;
    }
    static std::int32_t evalFn(bool, bool) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, VectorF) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(Matrix, float) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(float, Matrix) // error case
    {
        return throwError();
    }
    static std::int32_t evalFn(VectorF, Matrix) // error case
    {
        return throwError();
    }
    static bool evalFn(float a, float b)
    {
        return a >= b;
    }
    static bool evalFn(std::int32_t a, std::int32_t b)
    {
        return a >= b;
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(float a, std::int32_t b)
    {
        return evalFn((float)a, (float)b);
    }
    static decltype(evalFn((float)0, (float)0)) evalFn(std::int32_t a, float b)
    {
        return evalFn((float)a, (float)b);
    }
    static std::shared_ptr<Data> evaluateBackup(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        if(arg1->type() == Data::Type::String && arg2->type() == Data::Type::String)
        {
            return std::shared_ptr<Data>(new DataBoolean((std::wstring)*arg1 >= (std::wstring)*arg2));
        }
        throwError();
        return nullptr;
    }
};
struct NodeNeg final : public NodeConstArgCount<1, NodeNeg>
{
    virtual Type type() const override
    {
        return Type::Neg;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Vector)
        {
            return std::shared_ptr<Data>(new DataVector(-dynamic_cast<DataVector *>(retval.get())->value));
        }
        if(retval->type() == Data::Type::Float)
        {
            return std::shared_ptr<Data>(new DataFloat(-dynamic_cast<DataFloat *>(retval.get())->value));
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::shared_ptr<Data>(new DataInteger(-dynamic_cast<DataInteger *>(retval.get())->value));
        }
        throw ScriptException(std::make_shared<DataString>(L"invalid type for - : " + retval->typeString()));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeNot final : public NodeConstArgCount<1, NodeNot>
{
    virtual Type type() const override
    {
        return Type::Not;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Boolean)
        {
            return std::shared_ptr<Data>(new DataBoolean(!dynamic_cast<DataBoolean *>(retval.get())->value));
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::shared_ptr<Data>(new DataInteger(~dynamic_cast<DataInteger *>(retval.get())->value));
        }
        throw ScriptException(std::make_shared<DataString>(L"invalid type for not : " + retval->typeString()));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeAbs final : public NodeConstArgCount<1, NodeAbs>
{
    virtual Type type() const override
    {
        return Type::Abs;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Vector)
        {
            return std::shared_ptr<Data>(new DataFloat(abs(dynamic_cast<DataVector *>(retval.get())->value)));
        }
        if(retval->type() == Data::Type::Float)
        {
            return std::shared_ptr<Data>(new DataFloat(std::abs(dynamic_cast<DataFloat *>(retval.get())->value)));
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::shared_ptr<Data>(new DataInteger(std::abs(dynamic_cast<DataInteger *>(retval.get())->value)));
        }
        throw ScriptException(std::make_shared<DataString>(L"invalid type for std::abs : " + retval->typeString()));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeSin final : public NodeConstArgCount<1, NodeSin>
{
    virtual Type type() const override
    {
        return Type::Sin;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Float)
        {
            return std::shared_ptr<Data>(new DataFloat(sin(dynamic_cast<DataFloat *>(retval.get())->value)));
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::shared_ptr<Data>(new DataFloat((float)sin(dynamic_cast<DataInteger *>(retval.get())->value)));
        }
        throw ScriptException(std::make_shared<DataString>(L"invalid type for sin : " + retval->typeString()));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeCos final : public NodeConstArgCount<1, NodeCos>
{
    virtual Type type() const override
    {
        return Type::Cos;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Float)
        {
            return std::shared_ptr<Data>(new DataFloat(cos(dynamic_cast<DataFloat *>(retval.get())->value)));
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::shared_ptr<Data>(new DataFloat((float)cos(dynamic_cast<DataInteger *>(retval.get())->value)));
        }
        throw ScriptException(std::make_shared<DataString>(L"invalid type for cos : " + retval->typeString()));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeTan final : public NodeConstArgCount<1, NodeTan>
{
    virtual Type type() const override
    {
        return Type::Tan;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Float)
        {
            float value = dynamic_cast<DataFloat *>(retval.get())->value;
            if(std::abs(cos(value)) < 1e-6)
            {
                throw ScriptException(L"tan domain error");
            }
            return std::shared_ptr<Data>(new DataFloat(tan(value)));
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::shared_ptr<Data>(new DataFloat((float)tan(dynamic_cast<DataInteger *>(retval.get())->value)));
        }
        throw ScriptException(std::make_shared<DataString>(L"invalid type for tan : " + retval->typeString()));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeATan final : public NodeConstArgCount<1, NodeATan>
{
    virtual Type type() const override
    {
        return Type::ATan;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Float)
        {
            float value = dynamic_cast<DataFloat *>(retval.get())->value;
            return std::shared_ptr<Data>(new DataFloat(atan(value)));
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::shared_ptr<Data>(new DataFloat((float)atan(dynamic_cast<DataInteger *>(retval.get())->value)));
        }
        throw ScriptException(std::make_shared<DataString>(L"invalid type for atan : " + retval->typeString()));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeASin final : public NodeConstArgCount<1, NodeASin>
{
    virtual Type type() const override
    {
        return Type::ASin;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        float value;
        if(retval->type() == Data::Type::Float)
        {
            value = dynamic_cast<DataFloat *>(retval.get())->value;
        }
        else if(retval->type() == Data::Type::Integer)
        {
            value = (float)dynamic_cast<DataInteger *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for asin : " + retval->typeString()));
        }
        if(value < -1 || value > 1)
        {
            throw ScriptException(L"asin domain error");
        }
        return std::shared_ptr<Data>(new DataFloat(asin(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeACos final : public NodeConstArgCount<1, NodeACos>
{
    virtual Type type() const override
    {
        return Type::ACos;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        float value;
        if(retval->type() == Data::Type::Float)
        {
            value = dynamic_cast<DataFloat *>(retval.get())->value;
        }
        else if(retval->type() == Data::Type::Integer)
        {
            value = (float)dynamic_cast<DataInteger *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for acos : " + retval->typeString()));
        }
        if(value < -1 || value > 1)
        {
            throw ScriptException(L"acos domain error");
        }
        return std::shared_ptr<Data>(new DataFloat(acos(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeExp final : public NodeConstArgCount<1, NodeExp>
{
    virtual Type type() const override
    {
        return Type::Exp;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        float value;
        if(retval->type() == Data::Type::Float)
        {
            value = dynamic_cast<DataFloat *>(retval.get())->value;
        }
        else if(retval->type() == Data::Type::Integer)
        {
            value = (float)dynamic_cast<DataInteger *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for exp : " + retval->typeString()));
        }
        if(value > 88.7228)
        {
            throw ScriptException(L"exp overflow error");
        }
        return std::shared_ptr<Data>(new DataFloat(exp(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeLog final : public NodeConstArgCount<1, NodeLog>
{
    virtual Type type() const override
    {
        return Type::Log;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        float value;
        if(retval->type() == Data::Type::Float)
        {
            value = dynamic_cast<DataFloat *>(retval.get())->value;
        }
        else if(retval->type() == Data::Type::Integer)
        {
            value = (float)dynamic_cast<DataInteger *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for log : " + retval->typeString()));
        }
        if(value <= 0)
        {
            throw ScriptException(L"log domain error");
        }
        return std::shared_ptr<Data>(new DataFloat(log(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeSqrt final : public NodeConstArgCount<1, NodeSqrt>
{
    virtual Type type() const override
    {
        return Type::Sqrt;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        float value;
        if(retval->type() == Data::Type::Float)
        {
            value = dynamic_cast<DataFloat *>(retval.get())->value;
        }
        else if(retval->type() == Data::Type::Integer)
        {
            value = (float)dynamic_cast<DataInteger *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for sqrt : " + retval->typeString()));
        }
        if(value < 0)
        {
            throw ScriptException(L"sqrt domain error");
        }
        return std::shared_ptr<Data>(new DataFloat(sqrt(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeATan2 final : public NodeConstArgCount<2, NodeATan2>
{
    virtual Type type() const override
    {
        return Type::ATan2;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        float v1;
        if(arg1->type() == Data::Type::Float)
        {
            v1 = dynamic_cast<DataFloat *>(arg1.get())->value;
        }
        else if(arg1->type() == Data::Type::Integer)
        {
            v1 = (float)dynamic_cast<DataInteger *>(arg1.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for atan2 : " + arg1->typeString()));
        }
        float v2;
        if(arg2->type() == Data::Type::Float)
        {
            v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
        }
        else if(arg2->type() == Data::Type::Integer)
        {
            v2 = (float)dynamic_cast<DataInteger *>(arg2.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for atan2 : " + arg2->typeString()));
        }
        if(v1 == 0 && v2 == 0)
        {
            throw ScriptException(L"sqrt domain error");
        }
        return std::shared_ptr<Data>(new DataFloat(atan2(v1, v2)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1), state.nodes[args[1]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeConditional final : public NodeConstArgCount<3, NodeConditional>
{
    virtual Type type() const override
    {
        return Type::Conditional;
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        std::shared_ptr<Data> condition = state.nodes[args[0]]->evaluate(state, stackDepth + 1);
        if(condition->type() != Data::Type::Boolean)
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for conditional : " + condition->typeString()));
        }
        if(dynamic_cast<DataBoolean *>(condition.get())->value)
        {
            return state.nodes[args[1]]->evaluate(state, stackDepth + 1);
        }
        return state.nodes[args[2]]->evaluate(state, stackDepth + 1);
    }
};
struct NodeMakeRotate final : public NodeConstArgCount<2, NodeMakeRotate>
{
    virtual Type type() const override
    {
        return Type::MakeRotate;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> arg1, std::shared_ptr<Data> arg2)
    {
        VectorF v1;
        if(arg1->type() == Data::Type::Vector)
        {
            v1 = dynamic_cast<DataVector *>(arg1.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for make_rotate : " + arg1->typeString()));
        }
        float v2;
        if(arg2->type() == Data::Type::Float)
        {
            v2 = dynamic_cast<DataFloat *>(arg2.get())->value;
        }
        else if(arg2->type() == Data::Type::Integer)
        {
            v2 = (float)dynamic_cast<DataInteger *>(arg2.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for make_rotate : " + arg2->typeString()));
        }
        if(v1 == VectorF(0))
        {
            throw ScriptException(L"make_rotate called with <0, 0, 0>");
        }
        return std::shared_ptr<Data>(new DataMatrix(Matrix::rotate(v1, v2)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1), state.nodes[args[1]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeMakeRotateX final : public NodeConstArgCount<1, NodeMakeRotateX>
{
    virtual Type type() const override
    {
        return Type::MakeRotateX;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        float value;
        if(retval->type() == Data::Type::Float)
        {
            value = dynamic_cast<DataFloat *>(retval.get())->value;
        }
        else if(retval->type() == Data::Type::Integer)
        {
            value = (float)dynamic_cast<DataInteger *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for make_rotatex : " + retval->typeString()));
        }
        return std::shared_ptr<Data>(new DataMatrix(Matrix::rotateX(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeMakeRotateY final : public NodeConstArgCount<1, NodeMakeRotateY>
{
    virtual Type type() const override
    {
        return Type::MakeRotateY;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        float value;
        if(retval->type() == Data::Type::Float)
        {
            value = dynamic_cast<DataFloat *>(retval.get())->value;
        }
        else if(retval->type() == Data::Type::Integer)
        {
            value = (float)dynamic_cast<DataInteger *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for make_rotatey : " + retval->typeString()));
        }
        return std::shared_ptr<Data>(new DataMatrix(Matrix::rotateY(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeMakeRotateZ final : public NodeConstArgCount<1, NodeMakeRotateZ>
{
    virtual Type type() const override
    {
        return Type::MakeRotateZ;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        float value;
        if(retval->type() == Data::Type::Float)
        {
            value = dynamic_cast<DataFloat *>(retval.get())->value;
        }
        else if(retval->type() == Data::Type::Integer)
        {
            value = (float)dynamic_cast<DataInteger *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for make_rotatez : " + retval->typeString()));
        }
        return std::shared_ptr<Data>(new DataMatrix(Matrix::rotateZ(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeMakeScale final : public NodeConstArgCount<1, NodeMakeScale>
{
    virtual Type type() const override
    {
        return Type::MakeScale;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        VectorF value;
        if(retval->type() == Data::Type::Float)
        {
            value = VectorF(dynamic_cast<DataFloat *>(retval.get())->value);
        }
        else if(retval->type() == Data::Type::Integer)
        {
            value = VectorF((float)dynamic_cast<DataInteger *>(retval.get())->value);
        }
        else if(retval->type() == Data::Type::Vector)
        {
            value = dynamic_cast<DataVector *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for make_scale : " + retval->typeString()));
        }
        return std::shared_ptr<Data>(new DataMatrix(Matrix::scale(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeMakeTranslate final : public NodeConstArgCount<1, NodeMakeTranslate>
{
    virtual Type type() const override
    {
        return Type::MakeTranslate;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        VectorF value;
        if(retval->type() == Data::Type::Vector)
        {
            value = dynamic_cast<DataVector *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for make_translate : " + retval->typeString()));
        }
        return std::shared_ptr<Data>(new DataMatrix(Matrix::translate(value)));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeBlock final : public Node
{
    NodeBlock()
        : nodes()
    {
    }
    friend struct Node;
    std::vector<std::uint32_t> nodes;
    virtual Type type() const override
    {
        return Type::Block;
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        std::shared_ptr<Data> retval = std::shared_ptr<Data>(new DataInteger(0));
        for(std::uint32_t n : nodes)
        {
            retval = state.nodes[n]->evaluate(state, stackDepth + 1);
        }
        return retval;
    }
    virtual void write(stream::Writer &writer) const override
    {
        stream::write<Type>(writer, type());
        assert((std::uint32_t)nodes.size() == nodes.size() && nodes.size() != (std::uint32_t) - 1);
        writer.writeU32((std::uint32_t)nodes.size());
        for(std::uint32_t v : nodes)
        {
            writer.writeU32(v);
        }
    }
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        std::shared_ptr<NodeBlock> retval = std::make_shared<NodeBlock>();
        std::size_t length = reader.readLimitedU32(0, (std::uint32_t) - 2);
        retval->nodes.reserve(length);
        for(std::size_t i = 0; i < length; i++)
        {
            retval->nodes.push_back(reader.readLimitedU32(0, nodeCount - 1));
        }
        return std::static_pointer_cast<Node>(retval);
    }
};
struct NodeListLiteral final : public Node
{
    NodeListLiteral()
        : nodes()
    {
    }
    friend struct Node;
    std::vector<std::uint32_t> nodes;
    virtual Type type() const override
    {
        return Type::ListLiteral;
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        std::shared_ptr<DataList> retval = std::make_shared<DataList>();
        for(std::uint32_t n : nodes)
        {
            retval->value.push_back(state.nodes[n]->evaluate(state, stackDepth + 1));
        }
        return std::static_pointer_cast<Data>(retval);
    }
    virtual void write(stream::Writer &writer) const override
    {
        stream::write<Type>(writer, type());
        assert((std::uint32_t)nodes.size() == nodes.size() && nodes.size() != (std::uint32_t) - 1);
        writer.writeU32((std::uint32_t)nodes.size());
        for(std::uint32_t v : nodes)
        {
            writer.writeU32(v);
        }
    }
protected:
    static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount)
    {
        std::shared_ptr<NodeBlock> retval = std::make_shared<NodeBlock>();
        std::size_t length = reader.readLimitedU32(0, (std::uint32_t) - 2);
        retval->nodes.reserve(length);
        for(std::size_t i = 0; i < length; i++)
        {
            retval->nodes.push_back(reader.readLimitedU32(0, nodeCount - 1));
        }
        return std::static_pointer_cast<Node>(retval);
    }
};
struct NodeNewObject final : public Node
{
    friend struct Node;
    virtual Type type() const override
    {
        return Type::NewObject;
    }
protected:
    static std::shared_ptr<Node> read(stream::Reader &, std::uint32_t)
    {
        return std::make_shared<NodeNewObject>();
    }
public:
    virtual void write(stream::Writer &writer) const override
    {
        stream::write<Type>(writer, type());
    }
    virtual std::shared_ptr<Data> evaluate(State &, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return std::shared_ptr<Data>(new DataObject);
    }
};
struct NodeDoWhile final : public NodeConstArgCount<2, NodeDoWhile>
{
    virtual Type type() const override
    {
        return Type::DoWhile;
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        std::shared_ptr<Data> retval;
        while(true)
        {
            state.onLoop();
            retval = state.nodes[args[0]]->evaluate(state, stackDepth + 1);
            std::shared_ptr<Data> condition = state.nodes[args[1]]->evaluate(state, stackDepth + 1);
            if(condition->type() != Data::Type::Boolean)
            {
                throw ScriptException(std::make_shared<DataString>(L"invalid type for conditional : " + condition->typeString()));
            }
            if(!dynamic_cast<DataBoolean *>(condition.get())->value)
            {
                return retval;
            }
        }
    }
};
struct NodeRemoveTranslate final : public NodeConstArgCount<1, NodeRemoveTranslate>
{
    virtual Type type() const override
    {
        return Type::RemoveTranslate;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        Matrix value;
        if(retval->type() == Data::Type::Matrix)
        {
            value = dynamic_cast<DataMatrix *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for remove_translate : " + retval->typeString()));
        }
        value.set(3, 0, 0);
        value.set(3, 1, 0);
        value.set(3, 2, 0);
        return std::shared_ptr<Data>(new DataMatrix(value));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeInvert final : public NodeConstArgCount<1, NodeInvert>
{
    virtual Type type() const override
    {
        return Type::Invert;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        Matrix value;
        if(retval->type() == Data::Type::Matrix)
        {
            value = dynamic_cast<DataMatrix *>(retval.get())->value;
        }
        else
        {
            throw ScriptException(std::make_shared<DataString>(L"invalid type for invert : " + retval->typeString()));
        }
        try
        {
            value = inverse(value);
        }
        catch(std::domain_error &)
        {
            throw ScriptException(L"can't invert singular matrix");
        }
        return std::shared_ptr<Data>(new DataMatrix(value));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeCeil final : public NodeConstArgCount<1, NodeCeil>
{
    virtual Type type() const override
    {
        return Type::Ceil;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Float)
        {
            return std::shared_ptr<Data>(new DataInteger((std::int32_t)ceil(dynamic_cast<DataFloat *>(retval.get())->value)));
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::shared_ptr<Data>(new DataInteger(dynamic_cast<DataInteger *>(retval.get())->value));
        }
        throw ScriptException(std::make_shared<DataString>(L"invalid type for ceil : " + retval->typeString()));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeFloor final : public NodeConstArgCount<1, NodeCeil>
{
    virtual Type type() const override
    {
        return Type::Floor;
    }
    static std::shared_ptr<Data> evaluate(std::shared_ptr<Data> retval)
    {
        if(retval->type() == Data::Type::Float)
        {
            return std::shared_ptr<Data>(new DataInteger((std::int32_t)ceil(dynamic_cast<DataFloat *>(retval.get())->value)));
        }
        if(retval->type() == Data::Type::Integer)
        {
            return std::shared_ptr<Data>(new DataInteger(dynamic_cast<DataInteger *>(retval.get())->value));
        }
        throw ScriptException(std::make_shared<DataString>(L"invalid type for ceil : " + retval->typeString()));
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        return evaluate(state.nodes[args[0]]->evaluate(state, stackDepth + 1));
    }
};
struct NodeFor final : public NodeConstArgCount<4, NodeFor>
{
    virtual Type type() const override
    {
        return Type::For;
    }
    virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth) const override
    {
        checkStackDepth(stackDepth);
        std::shared_ptr<Data> retval;
        state.nodes[args[0]]->evaluate(state, stackDepth + 1);
        while(true)
        {
            state.onLoop();
            std::shared_ptr<Data> condition = state.nodes[args[1]]->evaluate(state, stackDepth + 1);
            if(condition->type() != Data::Type::Boolean)
            {
                throw ScriptException(std::make_shared<DataString>(L"invalid type for conditional : " + condition->typeString()));
            }
            if(!dynamic_cast<DataBoolean *>(condition.get())->value)
            {
                return retval;
            }
            retval = state.nodes[args[3]]->evaluate(state, stackDepth + 1);
            state.nodes[args[2]]->evaluate(state, stackDepth + 1);
        }
    }
};
}
}
}

#endif // SCRIPT_NODES_H_INCLUDED
