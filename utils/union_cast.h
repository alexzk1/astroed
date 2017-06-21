#ifndef UNION_CAST_H
#define UNION_CAST_H
#include <memory.h>
#include <type_traits>
//tricking strict aliasing

template<class T, class Src>
typename std::enable_if<std::is_pointer<T>::value, T>::type union_cast(Src src)
{
//    union
//    {
//        Src s;
//        T d;
//    } tmp;
//    tmp.s = src;
//    return tmp.d;
    T tmp;
    memcpy(&tmp, &src, sizeof (tmp));
    return tmp;
}

#endif // UNION_CAST_H
