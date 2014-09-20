#ifndef ARENA_ALLOCATOR_H_INCLUDED
#define ARENA_ALLOCATOR_H_INCLUDED

#include <vector>

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

#endif // ARENA_ALLOCATOR_H_INCLUDED
