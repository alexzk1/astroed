#ifndef LUA_ALLOCATOR_H
#define LUA_ALLOCATOR_H

#include <memory.h>
#include <memory>
#include <iostream>

namespace luaHelperNS
{
    template<int> inline size_t doAlign(size_t nsize);
    template<> inline size_t doAlign<4>(size_t nsize)
    {
        if (nsize % 4 == 0)
            return nsize;

        // do 32-bits operations
        return ((nsize >> 2) + 1)<<2;
    }
    template<> inline size_t doAlign<8>(size_t nsize)
    {
        if (nsize % 8 == 0)
            return nsize;
        // do 64-bits operations
        return ((nsize >> 3) + 1)<<3;
    }
}


class lua_allocator
{
private:

    void findAndDelete(char *ptr)
    {
        if (ptr != nullptr)
        {
            delete [] ptr;
        }
    }

    char* allocateNew(size_t size)
    {
        return new char [luaHelperNS::doAlign<sizeof(size_t)>(size)];
    }

public:
    lua_allocator()
    {}

    void* allocate(char *ptr, size_t osize, size_t nsize)
    {
        //std::lock_guard<std::mutex> grd(allocm); //fixme: warning, it may be deadlock here, however it is needed, because lualocker used can be unlocked anywhere
        //custom allocator, which uses new/delete which is thread-safe
        //pretty dumb and it is plenty to improve i bet
        /*
         *
         * The type of the memory-allocation function used by Lua states. The allocator function must provide a functionality similar to realloc, but not exactly the same.
         * Its arguments are ud, an opaque pointer passed to lua_newstate; ptr, a pointer to the block being allocated/reallocated/freed;
         * osize, the original size of the
         * block; nsize, the new size of the block. ptr is NULL if and only if osize is zero. When nsize is zero, the allocator must return NULL;
         * if osize is not zero, it should free the block pointed to by ptr. When nsize is not zero, the allocator returns NULL
         * if and only if it cannot fill the request. When nsize is not zero and osize is zero, the allocator should behave like malloc.
         * When nsize and osize are not zero, the allocator behaves like realloc. Lua assumes that the allocator never fails when osize >= nsize.
*/
        char* res = nullptr;
        if (nsize == 0)
        {
            if (osize)
                findAndDelete(ptr);
        }
        else
        {
            if (!osize)
            {
                //malloc
                res = allocateNew(nsize);
            }
            else
            {
                //realloc
                res = ptr;
                if (nsize > luaHelperNS::doAlign<sizeof(size_t)>(osize))
                {
                    res = allocateNew(nsize);
                    if (ptr != nullptr)
                    {
                        //std::cout << "Reallocating: " << (size_t)ptr << std::endl;
                        memcpy(res, ptr, std::min(osize, nsize));
                        findAndDelete(ptr);
                    }
                }
            }
        }

        return res;
    }
};

#endif // LUA_ALLOCATOR_H
