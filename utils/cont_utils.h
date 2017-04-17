#ifndef CONT_UTILS_H
#define CONT_UTILS_H

#include <vector>
#include <set>
#include <algorithm>
#include <type_traits>
#include <stdint.h>
#include <iterator>

//containers utils

namespace utility
{
    template<typename T, typename L = std::vector<T>>
    size_t vector_unique_ordered(L& vec)
    {
        std::set<T> seen;

        auto newEnd = std::remove_if(vec.begin(), vec.end(), [&seen](const T& value)
        {
            if (seen.count(value))
                return true;

            seen.insert(value);
            return false;
        });

        vec.erase(newEnd, vec.end());

        return vec.size();
    }

    //swaps pointed values (so huge template, just to ensure it will swap values and not pointers itself and fail at compile time)
    template <typename PtrT,
              typename Pointed = typename std::enable_if<std::is_pointer<PtrT>::value,
                                                         typename std::remove_pointer<PtrT>::type>::type>
    void swapPointed(PtrT v1, PtrT v2)
    {
        std::swap<Pointed>(*v1, *v2);
    }

    //can erase from std::map
    template< typename ContainerT, typename PredicateT >
    void erase_if( ContainerT& items, const PredicateT& predicate )
    {
        for( auto it = items.begin(); it != items.end(); )
        {
              if( predicate(*it) ) it = items.erase(it);
              else ++it;
        }
    }
}

#endif // CONT_UTILS_H
