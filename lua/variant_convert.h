#ifndef VARIANT_CONVERT_H
#define VARIANT_CONVERT_H

#ifdef QT_CORE_LIB
#include <QVariant>
#include <QString>
#include <QDate>

namespace luavm
{
    template<class T, typename T2 = typename std::enable_if<!std::is_enum<T>::value, void>::type>
    T variantTo(const QVariant& var)
    {
        return qvariant_cast<T>(var);
    }

    //to resolve enum is ambiqous with ints ... making different amount of template args
    template<typename T, typename T3 = int, typename T2 = typename std::enable_if<std::is_enum<T>::value, T>::type>
    inline
    T variantTo(const QVariant& var)
    {
        return static_cast<T>(var.toInt());
    }
}
#endif
#endif // VARIANT_CONVERT_H
