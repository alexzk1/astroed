#ifndef ENUMS_CHECK
#define ENUMS_CHECK
#include <type_traits>
#include <utility>
#include <functional>

template<typename E>
constexpr typename std::underlying_type<E>::type enumToUType(E en) noexcept
{
    return static_cast<typename std::underlying_type<E>::type>(en);
}


namespace chkenum
{
    template <class T, T begin, T end>
    struct RangeCheck
    {
    public:
        using val_t  = typename std::underlying_type<T>::type;
        using pair_t = std::pair<T, T>;
        using CPairRef = const pair_t&;

        static
        typename std::enable_if<std::is_enum<T>::value, bool>::type inrange(const val_t& value)
        {
            return value >= enumToUType(begin) && value <= enumToUType(end);
        }

        static
        typename std::enable_if<std::is_enum<T>::value, CPairRef>::type getEnumFirstLast()
        {
            static const pair_t p{begin, end};
            return p;
        }

    };

    template<class T>
    struct EnumCheck;
}

#define DECLARE_ENUM_CHECK(T,B,E) namespace chkenum {template<> struct EnumCheck<T> : public RangeCheck<T, B, E> {};}

template<class T>
inline
typename std::enable_if<std::is_enum<T>::value, bool>::type testEnumRange(int val)
{
    return chkenum::EnumCheck<T>::inrange(val);
}

template<class T>
inline
typename std::enable_if<std::is_enum<T>::value, typename chkenum::EnumCheck<T>::CPairRef>::type getEnumFirstLast()
{
    return chkenum::EnumCheck<T>::getEnumFirstLast();
}


//enum must have DECLARE_ENUM_CHECK to use those forEach* functions, also elements assumed sequentaly numbered
template<class T>
inline
typename std::enable_if<std::is_enum<T>::value, void>::type forEachEnum(const typename std::function<void(T)>& callback)
{
    auto range = getEnumFirstLast<T>();
    for (int64_t index = static_cast<decltype (index)>(range.first); index <= static_cast<decltype (index)>(range.second); index ++)
        callback(static_cast<T>(index));
}

//breakable version, breaks loop if callback returns true
template<class T>
inline
typename std::enable_if<std::is_enum<T>::value, void>::type forEachEnumBreakable(const typename std::function<bool(T)>& callback)
{
    auto range = getEnumFirstLast<T>();
    for (int64_t index = static_cast<decltype (index)>(range.first); index <= static_cast<decltype (index)>(range.second); index ++)
        if (callback(static_cast<T>(index))) break;
}

#endif // ENUMS_CHECK

