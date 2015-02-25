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
#ifndef SCRIPT_H_INCLUDED
#define SCRIPT_H_INCLUDED

#include <cstdint>
#include <memory>
#include "stream/stream.h"
#include "render/mesh.h"
#include <unordered_map>
#include "util/variable_set.h"
#include "util/enum_traits.h"
#include <sstream>
#include <cmath>
#include <iostream>
#include "util/logging.h"

namespace programmerjake
{
namespace voxels
{
namespace Scripting
{
    struct Data : public std::enable_shared_from_this<Data>
    {
        enum class Type : std::uint8_t
        {
            Boolean,
            Integer,
            Float,
            Vector,
            Matrix,
            List,
            Object,
            String,
            DEFINE_ENUM_LIMITS(Boolean, String)
        };
        virtual Type type() const = 0;
        virtual ~Data()
        {
        }
        virtual std::shared_ptr<Data> dup() const = 0;
        virtual void write(stream::Writer &writer) const = 0;
        static std::shared_ptr<Data> read(stream::Reader &reader);
        virtual explicit operator std::wstring() const = 0;
        std::wstring typeString() const
        {
            switch(type())
            {
            case Type::Integer:
                return L"integer";
            case Type::Float:
                return L"float";
            case Type::Vector:
                return L"vector";
            case Type::Matrix:
                return L"matrix";
            case Type::List:
                return L"list";
            case Type::Object:
                return L"object";
            case Type::String:
                return L"string";
            case Type::Boolean:
                return L"boolean";
            }
            assert(false);
            return L"unknown";
        }
    };
    struct DataBoolean final : public Data
    {
        virtual Type type() const override
        {
            return Type::Boolean;
        }
        bool value;
        DataBoolean(bool value = false)
            : value(value)
        {
        }
        virtual std::shared_ptr<Data> dup() const override
        {
            return std::shared_ptr<Data>(new DataBoolean(value));
        }
        virtual void write(stream::Writer &writer) const override
        {
            stream::write<Type>(writer, type());
            stream::write<bool>(writer, value);
        }
        friend struct Data;
    private:
        static std::shared_ptr<DataBoolean> read(stream::Reader &reader)
        {
            return std::make_shared<DataBoolean>(stream::read<bool>(reader));
        }
    public:
        virtual explicit operator std::wstring() const override
        {
            if(value)
            {
                return L"true";
            }
            return L"false";
        }
    };
    struct DataInteger final : public Data
    {
        virtual Type type() const override
        {
            return Type::Integer;
        }
        std::int32_t value;
        DataInteger(std::int32_t value = 0)
            : value(value)
        {
        }
        virtual std::shared_ptr<Data> dup() const override
        {
            return std::shared_ptr<Data>(new DataInteger(value));
        }
        virtual void write(stream::Writer &writer) const override
        {
            stream::write<Type>(writer, type());
            stream::write<std::int32_t>(writer, value);
        }
        friend struct Data;
    private:
        static std::shared_ptr<DataInteger> read(stream::Reader &reader)
        {
            return std::make_shared<DataInteger>(stream::read<std::int32_t>(reader));
        }
    public:
        virtual explicit operator std::wstring() const override
        {
            std::wostringstream os;
            os << value;
            return os.str();
        }
    };
    struct DataFloat final : public Data
    {
        virtual Type type() const override
        {
            return Type::Float;
        }
        float value;
        DataFloat(float value = 0)
            : value(value)
        {
        }
        virtual std::shared_ptr<Data> dup() const override
        {
            return std::shared_ptr<Data>(new DataFloat(value));
        }
        virtual void write(stream::Writer &writer) const override
        {
            stream::write<Type>(writer, type());
            writer.writeF32(value);
        }
        friend struct Data;
    private:
        static std::shared_ptr<DataFloat> read(stream::Reader &reader)
        {
            return std::make_shared<DataFloat>(reader.readFiniteF32());
        }
    public:
        virtual explicit operator std::wstring() const override
        {
            std::wostringstream os;
            os << value << L"f";
            return os.str();
        }
    };
    struct DataVector final : public Data
    {
        virtual Type type() const override
        {
            return Type::Vector;
        }
        VectorF value;
        DataVector(VectorF value = VectorF(0))
            : value(value)
        {
        }
        virtual std::shared_ptr<Data> dup() const override
        {
            return std::shared_ptr<Data>(new DataVector(value));
        }
        virtual void write(stream::Writer &writer) const override
        {
            stream::write<Type>(writer, type());
            writer.writeF32(value.x);
            writer.writeF32(value.y);
            writer.writeF32(value.z);
        }
        friend struct Data;
    private:
        static std::shared_ptr<DataVector> read(stream::Reader &reader)
        {
            VectorF value;
            value.x = reader.readFiniteF32();
            value.y = reader.readFiniteF32();
            value.z = reader.readFiniteF32();
            return std::make_shared<DataVector>(value);
        }
    public:
        virtual explicit operator std::wstring() const override
        {
            std::wostringstream os;
            os << L"<" << value.x << L", " << value.y << L", " << value.z << L">";
            return os.str();
        }
    };
    struct DataMatrix final : public Data
    {
        virtual Type type() const override
        {
            return Type::Matrix;
        }
        Matrix value;
        DataMatrix(Matrix value = Matrix::identity())
            : value(value)
        {
        }
        virtual std::shared_ptr<Data> dup() const override
        {
            return std::shared_ptr<Data>(new DataMatrix(value));
        }
        virtual void write(stream::Writer &writer) const override
        {
            stream::write<Type>(writer, type());
            for(int x = 0; x < 4; x++)
                for(int y = 0; y < 3; y++)
                {
                    writer.writeF32(value.get(x, y));
                }
        }
        friend struct Data;
    private:
        static std::shared_ptr<DataMatrix> read(stream::Reader &reader)
        {
            Matrix value;
            for(int x = 0; x < 4; x++)
                for(int y = 0; y < 3; y++)
                {
                    value.set(x, y, reader.readFiniteF32());
                }
            return std::make_shared<DataMatrix>(value);
        }
    public:
        virtual explicit operator std::wstring() const override
        {
            std::wostringstream os;
            for(int y = 0; y < 4; y++)
            {
                const wchar_t *str = L"|";
                for(int x = 0; x < 4; x++)
                {
                    os << str << value.get(x, y);
                    str = L" ";
                }
                os << L"|\n";
            }
            return os.str();
        }
    };
    struct DataList final : public Data
    {
        virtual Type type() const override
        {
            return Type::List;
        }
        std::vector<std::shared_ptr<Data>> value;
        DataList(const std::vector<std::shared_ptr<Data>> &value = std::vector<std::shared_ptr<Data>>())
            : value(value)
        {
        }
        DataList(std::vector<std::shared_ptr<Data>>  &&value)
            : value(value)
        {
        }
        virtual std::shared_ptr<Data> dup() const override
        {
            auto retval = std::shared_ptr<DataList>(new DataList);
            retval->value.reserve(value.size());
            for(std::shared_ptr<Data> e : value)
            {
                retval->value.push_back(e->dup());
            }
            return std::static_pointer_cast<Data>(retval);
        }
        virtual void write(stream::Writer &writer) const override
        {
            stream::write<Type>(writer, type());
            assert((std::uint32_t)value.size() == value.size() && value.size() != (std::uint32_t) - 1);
            writer.writeU32((std::uint32_t)value.size());
            for(std::shared_ptr<Data> v : value)
            {
                assert(v);
                v->write(writer);
            }
        }
        friend struct Data;
    private:
        static std::shared_ptr<DataList> read(stream::Reader &reader)
        {
            std::shared_ptr<DataList> retval = std::make_shared<DataList>();
            std::size_t length = reader.readLimitedU32(0, (std::uint32_t)-2);
            retval->value.reserve(length);
            for(size_t i = 0; i < length; i++)
            {
                retval->value.push_back(Data::read(reader));
            }
            return retval;
        }
    public:
        virtual explicit operator std::wstring() const override
        {
            if(value.size() == 0)
            {
                return L"[]";
            }
            std::wostringstream os;
            const wchar_t * str = L"[";
            for(std::shared_ptr<Data> e : value)
            {
                os << str << (std::wstring)*e;
                str = L", ";
            }
            os << L"]";
            return os.str();
        }
    };
    struct DataObject final : public Data
    {
        virtual Type type() const override
        {
            return Type::Object;
        }
        std::unordered_map<std::wstring, std::shared_ptr<Data>> value;
        DataObject(const std::unordered_map<std::wstring, std::shared_ptr<Data>> &value = std::unordered_map<std::wstring, std::shared_ptr<Data>>())
            : value(value)
        {
        }
        DataObject(std::unordered_map<std::wstring, std::shared_ptr<Data>>  &&value)
            : value(value)
        {
        }
        virtual std::shared_ptr<Data> dup() const override
        {
            auto retval = std::shared_ptr<DataObject>(new DataObject);
            for(std::pair<std::wstring, std::shared_ptr<Data>> e : value)
            {
                retval->value.insert(make_pair(std::get<0>(e), std::get<1>(e)->dup()));
            }
            return std::static_pointer_cast<Data>(retval);
        }
        virtual void write(stream::Writer &writer) const override
        {
            stream::write<Type>(writer, type());
            assert((std::uint32_t)value.size() == value.size() && value.size() != (std::uint32_t) - 1);
            writer.writeU32((std::uint32_t)value.size());
            for(std::pair<std::wstring, std::shared_ptr<Data>> v : value)
            {
                assert(std::get<1>(v));
                writer.writeString(std::get<0>(v));
                (std::get<1>(v))->write(writer);
            }
        }
        friend struct Data;
    private:
        static std::shared_ptr<DataObject> read(stream::Reader &reader)
        {
            std::shared_ptr<DataObject> retval = std::make_shared<DataObject>();
            std::size_t length = reader.readLimitedU32(0, (std::uint32_t) - 2);
            for(std::size_t i = 0; i < length; i++)
            {
                std::wstring name = reader.readString();
                retval->value[name] = Data::read(reader);
            }
            return retval;
        }
    public:
        virtual explicit operator std::wstring() const override
        {
            if(value.size() == 0)
            {
                return L"{}";
            }
            std::wostringstream os;
            const wchar_t * str = L"{";
            for(std::pair<std::wstring, std::shared_ptr<Data>> e : value)
            {
                os << str << L"\"" << std::get<0>(e) << L"\" = " << (std::wstring)*std::get<1>(e);
                str = L", ";
            }
            os << L"}";
            return os.str();
        }
    };
    struct DataString final : public Data
    {
        virtual Type type() const override
        {
            return Type::String;
        }
        std::wstring value;
        DataString(std::wstring value = L"")
            : value(value)
        {
        }
        virtual std::shared_ptr<Data> dup() const override
        {
            return std::shared_ptr<Data>(new DataString(value));
        }
        virtual void write(stream::Writer &writer) const override
        {
            stream::write<Type>(writer, type());
            writer.writeString(value);
        }
        friend struct Data;
    private:
        static std::shared_ptr<DataString> read(stream::Reader &reader)
        {
            return std::make_shared<DataString>(reader.readString());
        }
    public:
        virtual explicit operator std::wstring() const override
        {
            return value;
        }
    };
    struct ScriptException final : public std::runtime_error
    {
        std::shared_ptr<Data> data;
        ScriptException(std::shared_ptr<Data> data)
            : std::runtime_error(string_cast<std::string>((std::wstring)*data)), data(data)
        {
        }
        ScriptException(std::wstring str)
            : ScriptException(std::static_pointer_cast<Data>(std::make_shared<DataString>(str)))
        {
        }
    };
    struct State;
    struct Node : public std::enable_shared_from_this<Node>
    {
        enum class Type : std::uint16_t
        {
            Const,
            CastToString,
            CastToInteger,
            CastToFloat,
            CastToVector,
            CastToMatrix,
            CastToList,
            CastToObject,
            CastToBoolean,
            LoadGlobals,
            ReadIndex,
            AssignIndex,
            Add,
            Sub,
            Mul,
            Div,
            Mod,
            Pow,
            And,
            Or,
            Xor,
            Concat,
            Dot,
            Cross,
            Equal,
            NotEqual,
            LessThan,
            GreaterThan,
            LessEqual,
            GreaterEqual,
            Not,
            Neg,
            Abs,
            Sin,
            Cos,
            Tan,
            ATan,
            ASin,
            ACos,
            Exp,
            Log,
            Sqrt,
            ATan2,
            Conditional,
            MakeRotate,
            MakeRotateX,
            MakeRotateY,
            MakeRotateZ,
            MakeScale,
            MakeTranslate,
            Block,
            ListLiteral,
            NewObject,
            DoWhile,
            RemoveTranslate,
            Invert,
            Ceil,
            Floor,
            For,
            DEFINE_ENUM_LIMITS(Const, For)
        };

        virtual ~Node()
        {
        }
        virtual Type type() const = 0;
        virtual std::shared_ptr<Data> evaluate(State &state, unsigned stackDepth = 0) const = 0;
        virtual void write(stream::Writer &writer) const = 0;
        static std::shared_ptr<Node> read(stream::Reader &reader, std::uint32_t nodeCount);
    protected:
        static void checkStackDepth(unsigned stackDepth)
        {
            if(stackDepth > 1000)
            {
                throw ScriptException(L"stack depth limit exceeded");
            }
        }
    };
    struct State final
    {
        std::shared_ptr<DataObject> variables;
        unsigned loopCount = 0;
        void onLoop()
        {
            if(++loopCount >= 100000)
                throw ScriptException(L"too many loops");
        }
        const std::vector<std::shared_ptr<Node>> &nodes;
        State(const std::vector<std::shared_ptr<Node>> &nodes)
            : variables(std::make_shared<DataObject>()), nodes(nodes)
        {
        }
    };
}

class Script final : public std::enable_shared_from_this<Script>
{
public:
    std::vector<std::shared_ptr<Scripting::Node>> nodes;
    std::shared_ptr<Scripting::Data> evaluate(std::shared_ptr<Scripting::DataObject> inputObject = std::make_shared<Scripting::DataObject>()) const
    {
        Scripting::State state(nodes);
        if(inputObject == nullptr)
            inputObject = std::make_shared<Scripting::DataObject>();
        state.variables->value[L"io"] = inputObject;
        return nodes.back()->evaluate(state);
    }
    Matrix evaluateAsMatrix(std::shared_ptr<Scripting::DataObject> inputObject = std::make_shared<Scripting::DataObject>()) const
    {
        std::shared_ptr<Scripting::Data> retval = evaluate(inputObject);
        if(retval->type() != Scripting::Data::Type::Matrix)
        {
            throw Scripting::ScriptException(retval);
        }
        return std::dynamic_pointer_cast<Scripting::DataMatrix>(retval)->value;
    }
    VectorF evaluateAsVector(std::shared_ptr<Scripting::DataObject> inputObject = std::make_shared<Scripting::DataObject>()) const
    {
        std::shared_ptr<Scripting::Data> retval = evaluate(inputObject);
        if(retval->type() != Scripting::Data::Type::Vector)
        {
            throw Scripting::ScriptException(retval);
        }
        return std::dynamic_pointer_cast<Scripting::DataVector>(retval)->value;
    }
    std::int32_t evaluateAsInteger(std::shared_ptr<Scripting::DataObject> inputObject = std::make_shared<Scripting::DataObject>()) const
    {
        std::shared_ptr<Scripting::Data> retval = evaluate(inputObject);
        if(retval->type() != Scripting::Data::Type::Integer)
        {
            throw Scripting::ScriptException(retval);
        }
        return std::dynamic_pointer_cast<Scripting::DataInteger>(retval)->value;
    }
    float evaluateAsFloat(std::shared_ptr<Scripting::DataObject> inputObject = std::make_shared<Scripting::DataObject>()) const
    {
        std::shared_ptr<Scripting::Data> retval = evaluate(inputObject);
        if(retval->type() != Scripting::Data::Type::Float)
        {
            throw Scripting::ScriptException(retval);
        }
        return std::dynamic_pointer_cast<Scripting::DataFloat>(retval)->value;
    }
    std::wstring evaluateAsString(std::shared_ptr<Scripting::DataObject> inputObject = std::make_shared<Scripting::DataObject>()) const
    {
        std::shared_ptr<Scripting::Data> retval = evaluate(inputObject);
        if(retval->type() != Scripting::Data::Type::String)
        {
            throw Scripting::ScriptException(retval);
        }
        return std::dynamic_pointer_cast<Scripting::DataString>(retval)->value;
    }
    bool evaluateAsBoolean(std::shared_ptr<Scripting::DataObject> inputObject = std::make_shared<Scripting::DataObject>()) const
    {
        std::shared_ptr<Scripting::Data> retval = evaluate(inputObject);
        if(retval->type() != Scripting::Data::Type::Boolean)
        {
            throw Scripting::ScriptException(retval);
        }
        return std::dynamic_pointer_cast<Scripting::DataBoolean>(retval)->value;
    }
    static std::shared_ptr<Script> parse(std::wstring code);
    static std::shared_ptr<Script> read(stream::Reader &reader, VariableSet &)
    {
        std::uint32_t nodeCount = reader.readU32();
        auto retval = std::make_shared<Script>();
        retval->nodes.reserve(nodeCount);
        for(uint32_t i = 0; i < nodeCount; i++)
        {
            retval->nodes.push_back(Scripting::Node::read(reader, nodeCount));
        }
        return retval;
    }
    void write(stream::Writer &writer, VariableSet &)
    {
        writer.writeU32(nodes.size());
        for(auto n : nodes)
        {
            n->write(writer);
        }
    }
};

inline std::shared_ptr<Scripting::Data> Scripting::Data::read(stream::Reader &reader)
{
    Type type = stream::read<Type>(reader);
    switch(type)
    {
    case Type::Boolean:
        return std::static_pointer_cast<Data>(DataBoolean::read(reader));
    case Type::Integer:
        return std::static_pointer_cast<Data>(DataInteger::read(reader));
    case Type::Float:
        return std::static_pointer_cast<Data>(DataFloat::read(reader));
    case Type::Vector:
        return std::static_pointer_cast<Data>(DataVector::read(reader));
    case Type::Matrix:
        return std::static_pointer_cast<Data>(DataMatrix::read(reader));
    case Type::List:
        return std::static_pointer_cast<Data>(DataList::read(reader));
    case Type::Object:
        return std::static_pointer_cast<Data>(DataObject::read(reader));
    case Type::String:
        return std::static_pointer_cast<Data>(DataString::read(reader));
    }
    assert(false);
    return nullptr;
}

inline void runEntityPartScript(Mesh &dest, const Mesh &partMesh, std::shared_ptr<Script> script, VectorF position, VectorF velocity, float age, std::shared_ptr<Scripting::DataObject> ioObject = std::make_shared<Scripting::DataObject>())
{
    try
    {
        ioObject->value[L"age"] = std::make_shared<Scripting::DataFloat>(age);
        ioObject->value[L"position"] = std::make_shared<Scripting::DataVector>(position);
        ioObject->value[L"velocity"] = std::make_shared<Scripting::DataVector>(velocity);
        ioObject->value[L"doDraw"] = std::make_shared<Scripting::DataBoolean>(true);
        ioObject->value[L"transform"] = std::make_shared<Scripting::DataMatrix>(Matrix::translate(position));
        ioObject->value[L"colorR"] = std::make_shared<Scripting::DataFloat>(1);
        ioObject->value[L"colorG"] = std::make_shared<Scripting::DataFloat>(1);
        ioObject->value[L"colorB"] = std::make_shared<Scripting::DataFloat>(1);
        ioObject->value[L"colorA"] = std::make_shared<Scripting::DataFloat>(1);
        script->evaluate(ioObject);
        std::shared_ptr<Scripting::Data> pDoDraw = ioObject->value[L"doDraw"];
        if(!pDoDraw || pDoDraw->type() != Scripting::Data::Type::Boolean)
            throw Scripting::ScriptException(L"io.doDraw is not a valid value");
        bool doDraw = dynamic_cast<Scripting::DataBoolean *>(pDoDraw.get())->value;

        std::shared_ptr<Scripting::Data> pTform = ioObject->value[L"transform"];
        if(!pTform || pTform->type() != Scripting::Data::Type::Matrix)
            throw Scripting::ScriptException(L"io.transform is not a valid value");
        Matrix tform = dynamic_cast<Scripting::DataMatrix *>(pTform.get())->value;

        std::shared_ptr<Scripting::Data> pColorR = ioObject->value[L"colorR"];
        if(!pColorR || pColorR->type() != Scripting::Data::Type::Float)
            throw Scripting::ScriptException(L"io.colorR is not a valid value");
        float colorR = dynamic_cast<Scripting::DataFloat *>(pColorR.get())->value;
        if(colorR < 0 || colorR > 1)
            throw Scripting::ScriptException(L"io.colorR is not a valid value");

        std::shared_ptr<Scripting::Data> pColorG = ioObject->value[L"colorG"];
        if(!pColorG || pColorG->type() != Scripting::Data::Type::Float)
            throw Scripting::ScriptException(L"io.colorG is not a valid value");
        float colorG = dynamic_cast<Scripting::DataFloat *>(pColorG.get())->value;
        if(colorG < 0 || colorG > 1)
            throw Scripting::ScriptException(L"io.colorG is not a valid value");

        std::shared_ptr<Scripting::Data> pColorB = ioObject->value[L"colorB"];
        if(!pColorB || pColorB->type() != Scripting::Data::Type::Float)
            throw Scripting::ScriptException(L"io.colorB is not a valid value");
        float colorB = dynamic_cast<Scripting::DataFloat *>(pColorB.get())->value;
        if(colorB < 0 || colorB > 1)
            throw Scripting::ScriptException(L"io.colorB is not a valid value");

        std::shared_ptr<Scripting::Data> pColorA = ioObject->value[L"colorA"];
        if(!pColorA || pColorA->type() != Scripting::Data::Type::Float)
            throw Scripting::ScriptException(L"io.colorA is not a valid value");
        float colorA = dynamic_cast<Scripting::DataFloat *>(pColorA.get())->value;
        if(colorA < 0 || colorA > 1)
            throw Scripting::ScriptException(L"io.colorA is not a valid value");
        if(doDraw)
            dest.append(colorize(RGBAF(colorR, colorG, colorB, colorA), transform(tform, partMesh)));
    }
    catch(Scripting::ScriptException & e)
    {
        getDebugLog() << L"scripting error : " << string_cast<std::wstring>(e.what()) << postnl;
    }
}
}
}

#endif // SCRIPT_H_INCLUDED

