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
#ifndef VARIABLE_SET_H_INCLUDED
#define VARIABLE_SET_H_INCLUDED

#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>
#include <tuple>
#include "stream/stream.h"

namespace programmerjake
{
namespace voxels
{
class VariableSet final
{
private:
    class VariableDescriptorBase
    {
    protected:
        static std::uint64_t makeIndex()
        {
            static std::atomic_uint_fast64_t nextIndex(0);
            return ++nextIndex;
        }
    };
    struct VariableBase
    {
        virtual ~VariableBase()
        {
        }
    };
    template <typename T>
    struct Variable : public VariableBase
    {
        std::shared_ptr<T> ptr;
        Variable(std::shared_ptr<T> ptr)
            : ptr(ptr)
        {
        }
    };
    std::unordered_map<std::size_t, std::shared_ptr<VariableBase>> variables;
    std::unordered_map<const void *, std::pair<size_t, std::weak_ptr<void>>> reverseMap;
public:
    template <typename T>
    class Descriptor final : public VariableDescriptorBase
    {
        friend class VariableSet;
    private:
        std::uint64_t descriptorIndex;
        std::shared_ptr<T> & unwrap(std::shared_ptr<VariableBase> ptr) const
        {
            return std::dynamic_pointer_cast<Variable<T>>(ptr)->ptr;
        }
        Descriptor(std::uint64_t descriptorIndex)
            : descriptorIndex(descriptorIndex)
        {
        }
    public:
        Descriptor()
            : descriptorIndex(makeIndex())
        {
        }
        bool operator !() const
        {
            return descriptorIndex == 0;
        }
        operator bool() const
        {
            return descriptorIndex != 0;
        }
        static Descriptor read(stream::Reader & reader)
        {
            return Descriptor(reader.readU64());
        }
        void write(stream::Writer & writer) const
        {
            writer.writeU64(descriptorIndex);
        }
        static Descriptor null()
        {
            return Descriptor(0);
        }
    };
    std::recursive_mutex theLock;
    template <typename T>
    std::shared_ptr<T> get(const Descriptor<T> & descriptor)
    {
        std::lock_guard<std::recursive_mutex> lockIt(theLock);
        auto iter = variables.find(descriptor.descriptorIndex);
        if(iter == variables.end())
            return nullptr;
        if(std::get<1>(*iter) == nullptr)
            return nullptr;
        return descriptor.unwrap(std::get<1>(*iter));
    }
    template <typename T>
    bool set(const Descriptor<T> & descriptor, std::shared_ptr<T> value)
    {
        std::lock_guard<std::recursive_mutex> lockIt(theLock);
        reverseMap.erase(static_cast<const void *>(get<T>(descriptor).get()));
        if(value == nullptr)
        {
            return variables.erase(descriptor.descriptorIndex) != 0;
        }
        else
        {
            std::shared_ptr<VariableBase> & variable = variables[descriptor.descriptorIndex];
            bool retval = true;
            if(variable == nullptr)
            {
                variable = std::shared_ptr<VariableBase>(new Variable<T>(value));
                retval = false;
            }
            descriptor.unwrap(variable) = value;
            reverseMap[static_cast<const void *>(value.get())] = std::make_pair(descriptor.descriptorIndex, std::weak_ptr<void>(std::static_pointer_cast<void>(value)));
            return retval;
        }
    }
    template <typename T>
    Descriptor<T> find(std::shared_ptr<T> value)
    {
        std::lock_guard<std::recursive_mutex> lockIt(theLock);
        auto iter = reverseMap.find(static_cast<const void *>(value.get()));
        if(iter == reverseMap.end())
            return Descriptor<T>::null();
        if(std::get<1>(std::get<1>(*iter)).expired())
        {
            reverseMap.erase(iter);
            return Descriptor<T>::null();
        }
        return Descriptor<T>(std::get<0>(std::get<1>(*iter)));
    }
    template <typename T>
    std::pair<Descriptor<T>, bool> findOrMake(std::shared_ptr<T> value)
    {
        std::lock_guard<std::recursive_mutex> lockIt(theLock);
        auto iter = reverseMap.find(static_cast<const void *>(value.get()));
        if(iter == reverseMap.end())
        {
            Descriptor<T> descriptor;
            return std::make_pair(descriptor, set<T>(descriptor, value));
        }
        std::get<1>(std::get<1>(*iter)) = value;
        return std::make_pair(Descriptor<T>(std::get<0>(std::get<1>(*iter))), true);
    }
    template <typename T>
    std::shared_ptr<T> read_helper(stream::Reader &reader)
    {
        Descriptor<T> descriptor = stream::read<Descriptor<T>>(reader);
        if(!descriptor)
            return nullptr;
        std::shared_ptr<T> retval = get(descriptor);
        bool changed = stream::read<bool>(reader);
        if(retval != nullptr && !changed)
            return retval;
        retval = T::read(reader, *this);
        set(descriptor, retval);
        return retval;
    }
    template <typename T>
    void write_helper(stream::Writer &writer, std::shared_ptr<T> value, bool changed)
    {
        if(value == nullptr)
        {
            stream::write<Descriptor<T>>(writer, Descriptor<T>::null());
            return;
        }
        std::pair<Descriptor<T>, bool> findOrMakeReturnValue = findOrMake<T>(value);
        stream::write<Descriptor<T>>(writer, std::get<0>(findOrMakeReturnValue));
        stream::write<bool>(writer, changed || !std::get<1>(findOrMakeReturnValue));
        if(std::get<1>(findOrMakeReturnValue) && !changed)
        {
            return;
        }
        value->write(writer, *this);
    }
};

class ChangeTracker
{
    typedef std::uint_fast32_t ChangeCountType;
    std::atomic<ChangeCountType> currentChangeCount;
    std::atomic_bool changedFlag;
    struct ChangeCountWrapper
    {
        ChangeCountType changeCount = 0;
    };
    VariableSet::Descriptor<ChangeCountWrapper> changeCountDescriptor;
public:
    ChangeTracker()
        : currentChangeCount(0), changedFlag(false), changeCountDescriptor()
    {
    }
    void onChange()
    {
        changedFlag = true;
    }
    bool getChanged(VariableSet &variableSet)
    {
        if(changedFlag.exchange(false))
        {
            currentChangeCount++;
            return true;
        }
        std::shared_ptr<ChangeCountWrapper> pChangeCount = variableSet.get(changeCountDescriptor);
        if(pChangeCount == nullptr)
            return true;
        if(pChangeCount->changeCount < currentChangeCount)
            return true;
        return false;
    }
    void onWrite(VariableSet &variableSet)
    {
        std::shared_ptr<ChangeCountWrapper> pChangeCount = std::shared_ptr<ChangeCountWrapper>(new ChangeCountWrapper);
        pChangeCount->changeCount = currentChangeCount;
        variableSet.set(changeCountDescriptor, pChangeCount);
    }
};

namespace stream
{
template <typename T>
struct rw_cached_helper<T, typename std::enable_if<rw_class_traits_helper_has_read_with_VariableSet<T>::value>::type>
{
    typedef typename rw_class_traits_helper_has_read_with_VariableSet<T>::value_type value_type;
    static value_type read(Reader &reader, VariableSet &variableSet)
    {
        return variableSet.read_helper<T>(reader);
    }
    static void write(Writer &writer, VariableSet &variableSet, value_type value)
    {
        return variableSet.write_helper<T>(writer, value, is_value_changed<T>()(value, variableSet));
    }
};
}
}
}

#endif // VARIABLE_SET_H_INCLUDED
