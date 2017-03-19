#ifndef CONT_UTILS_H
#define CONT_UTILS_H

#include <vector>
#include <set>
#include <algorithm>
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
}

#endif // CONT_UTILS_H
