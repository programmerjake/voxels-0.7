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
#ifndef ARENA_ALLOCATOR_H_INCLUDED
#define ARENA_ALLOCATOR_H_INCLUDED

#include <vector>

namespace programmerjake
{
namespace voxels
{
template <typename T, size_t sizeLimit = 10000>
class ArenaAllocator final
{
private:
    typedef char Node[sizeof(T)];
    std::vector<Node *> nodes;
public:
    ArenaAllocator()
    {
    }
    ~ArenaAllocator()
    {
        for(Node * node : nodes)
        {
            delete node;
        }
    }
    void * alloc()
    {
        if(nodes.size() > 0)
        {
            void * retval = (void *)nodes.back();
            nodes.pop_back();
            return retval;
        }
        return (void *)new Node;
    }
    void free(void * mem)
    {
        if(mem == nullptr)
            return;
        if(nodes.size() >= sizeLimit)
            delete (Node *)mem;
        else
            nodes.push_back((Node *)mem);
    }
};
}
}

#endif // ARENA_ALLOCATOR_H_INCLUDED
