#ifndef LUA_PARAMS
#define LUA_PARAMS

#ifdef QT_CORE_LIB
#include <QString>
#include <QMap>
#include <QList>
#include <QVariant>
#include <QSet>
#include <QByteArray>
#include "variant_convert.h"
#endif

#include <string>
#include <stdint.h>
#include <type_traits>
#include <map>
#include <vector>
#include <set>
#include <memory>
#include <algorithm>

#include "utils/enums_check.h"
#include "utils/fatal_err.h"
#include "luasrc/lua.hpp"

namespace luavm
{
    namespace lua_templ
    {
        template <class K, class V>
        using lua_map_t = std::map<K, V>;

        template <class T>
        using lua_arrayhash_t = lua_map_t<int, T>;

        template <class T>
        using lua_array_t = std::vector<T>;

        template <class T>
        using lua_set_t = std::set<T>;

        template <class T>
        using lua_hash_t = lua_map_t<std::string, T>;
        typedef lua_array_t<std::string> string_arr_t;

#ifdef QT_CORE_LIB
        template <class T>
        using lua_qthash_t = lua_map_t<QString, T>;
        typedef lua_array_t<QString>     qtstring_arr_t;
#endif




        //some templates to make proper conditional compiling with explicit instantiation
        template <typename T>       struct isset:std::false_type{};
        template <typename... Args> struct isset<std::set<Args...>>:std::true_type{};

        template <typename T>       struct ismap:std::false_type{};
        template <typename... Args> struct ismap<std::map<Args...>>:std::true_type{};


        template <typename T>       struct ispair:std::false_type{};
        template <typename... Args> struct ispair<std::pair<Args...>>:std::true_type{};


        template <typename T>       struct isvector:std::false_type{};
        template <typename... Args> struct isvector<std::vector<Args...>>:std::true_type{};

        template <typename T>       struct issharedptr:std::false_type{};
        template <typename... Args> struct issharedptr<std::shared_ptr<Args...>>:std::true_type{};

#ifdef QT_CORE_LIB
        template <typename... Args> struct isset<QSet<Args...>>:std::true_type{};
        template <typename... Args> struct ismap<QMap<Args...>>:std::true_type{}; //not tested conversion with lua* funcs
        template <typename... Args> struct isvector<QList<Args...>>:std::true_type{};  //not tested conversion with lua* funcs
        template <>                 struct isvector<QStringList>:std::true_type{}; //there is inheritance and line above dont catch it
#endif

        template <typename T>
        struct checker
        {
            static constexpr bool isEnum    = std::is_enum<T>::value;
            static constexpr bool isSet     = isset<T>::value;
            static constexpr bool isMap     = ismap<T>::value;
            static constexpr bool isPair    = ispair<T>::value;
            static constexpr bool isVector  = isvector<T>::value;
            static constexpr bool isShared  = issharedptr<T>::value;

            static constexpr bool isTable   = isSet || isMap || isVector || isPair;
            static constexpr bool isGeneric = !isEnum && !isShared && !isTable;
        };

        //WARNING! index parameter is expected to be positive (use lua_gettop(L) instead -1),
        //using negatives leads to UB

        //--------------------------------------------------------------------------
        //explicit convertion to C++ type
        //usage: auto s = luaParam<int>(L, 1)
        template <class T>
        typename std::enable_if<checker<T>::isGeneric, T>::type
        luaParam(lua_State* L, int index);
        //--------------------------------------------------------------------------
        //check if LUA value can be explicit converted to C++ type
        //usage: luaTestType<int>(L, 2)
        template <class T>
        inline
        typename std::enable_if<checker<T>::isGeneric, bool>::type
        luaTestType(lua_State* L, int index);

        //--------------------------------------------------------------------------
        template <class T>
        inline T testGetParam(lua_State* L, int index);
        //--------------------------------------------------------------------------

        //special case for enums
        template <class T>
        inline typename std::enable_if<checker<T>::isEnum, T>::type
        luaParam(lua_State* L, int index)
        {
            return static_cast<T>(lua_tointeger(L, index));
        }
        //--------------------------------------------------------------------------

        template <class T>
        inline typename std::enable_if<checker<T>::isEnum, bool>::type
        luaTestType(lua_State* L, int index)
        {
            bool r = !lua_isnil(L, index) && lua_isinteger(L, index);
            if (r)
            {
                r = testEnumRange<T>(lua_tointeger(L, index));
            }
            return r;
        }
        //--------------------------------------------------------------------------

        template<>
        inline std::string luaParam<std::string>(lua_State* L, int index)
        {
            size_t sz = 0;
            const char* s = lua_tolstring(L, index, &sz);
            return std::string(s, sz);
        }
        //--------------------------------------------------------------------------
#ifdef QT_CORE_LIB
        template<>
        inline QString luaParam<QString>(lua_State* L, int index)
        {
            return QString::fromUtf8(lua_tostring(L, index));
            //return QString(lua_tostring(L, index));
        }
#endif
        //--------------------------------------------------------------------------
        template<>
        inline int64_t luaParam<int64_t>(lua_State* L, int index)
        {
            return static_cast<int64_t>(lua_tointeger(L, index));
        }
        //--------------------------------------------------------------------------
        template<>
        inline int luaParam<int>(lua_State* L, int index)
        {
            return static_cast<int>(lua_tointeger(L, index));
        }
        //--------------------------------------------------------------------------
        template<>
        inline bool luaParam<bool>(lua_State* L, int index)
        {
            if (lua_isnil(L, index)) //just to be safe, possibly toboolean does it too
                return false;
            return static_cast<bool>(lua_toboolean(L, index));
        }
        //--------------------------------------------------------------------------
        template<>
        inline double luaParam<double>(lua_State* L, int index)
        {
            return static_cast<double>(lua_tonumber(L, index));
        }
        //--------------------------------------------------------------------------
        template<>
        inline lua_State* luaParam<lua_State*>(lua_State* L, int index)
        {
            return static_cast<lua_State*>(lua_tothread(L, index));
        }
        //--------------------------------------------------------------------------

        template <class T>
        inline typename std::enable_if<checker<T>::isShared, T>::type
        luaParam(lua_State *L, int index)
        {
            return static_cast<typename T::element_type*>(lua_touserdata(L, index))->shared_from_this(); //a bit unsafe ...
        }
        //--------------------------------------------------------------------------
        //support for hashes, if key's or value's type differes from C++ declared - that will be skipped
        template <class T>
        inline typename std::enable_if<checker<T>::isMap, T>::type
        luaParam(lua_State *L, int index)
        {
            T result;
            typename T::key_type key;

            lua_pushnil(L);  /* first key */
            while (lua_next(L, index) != 0)
            {
                if (luaTestType<typename T::key_type>(L, -2))
                {
                    lua_pushvalue(L, -2);
                    key = luaParam<typename T::key_type>(L, -1);
                    lua_pop(L, 1);

                    if (luaTestType<typename T::mapped_type>(L, -1))
                        result[key] = luaParam<typename T::mapped_type>(L, -1);
                }
                lua_pop(L, 1);
            }

            return result;
        }
        //--------------------------------------------------------------------------
        //making vector out of number-keyed map
        template<class T>
        inline typename std::enable_if<checker<T>::isVector, T>::type
        luaParam(lua_State *L, int index)
        {
            auto m = luaParam<lua_arrayhash_t<typename T::value_type>>(L, index);
            T result;
            result.reserve(m.size());
            //map is already sorted by key
            std::for_each(m.cbegin(), m.cend(), [&result]
                          (const typename lua_arrayhash_t<typename T::value_type>::value_type &v)
            {
                result.push_back(v.second);
            });
            return result;
        }
        //--------------------------------------------------------------------------

        //making set out of lua's values ignoring keys
        template<class T>
        inline
        typename std::enable_if<checker<T>::isSet, T>::type
        luaParam(lua_State *L, int index)
        {
            T result;

            lua_pushnil(L);  /* first key */
            while (lua_next(L, index) != 0)
            {
                if (luaTestType<typename T::key_type>(L, -1))
                    result.insert(luaParam<typename T::key_type>(L, -1));
                lua_pop(L, 1);
            }

            return result;
        }
        //--------------------------------------------------------------------------

        template<>
        inline bool luaTestType<lua_State*>(lua_State* L, int index)
        {
            return !lua_isnil(L, index) && lua_isthread(L, index);
        }
        //--------------------------------------------------------------------------
        template<>
        inline bool luaTestType<std::string>(lua_State* L, int index)
        {
            return !lua_isnil(L, index) && lua_isstring(L, index);
        }
        //--------------------------------------------------------------------------
#ifdef QT_CORE_LIB
        template<>
        inline bool luaTestType<QString>(lua_State* L, int index)
        {
            return !lua_isnil(L, index) && lua_isstring(L, index);
        }
#endif
        //--------------------------------------------------------------------------
        template<>
        inline bool luaTestType<int>(lua_State* L, int index)
        {
            return !lua_isnil(L, index) && lua_isinteger(L, index);
        }
        //--------------------------------------------------------------------------
        template<>
        inline bool luaTestType<int64_t>(lua_State* L, int index)
        {
            return !lua_isnil(L, index) && lua_isinteger(L, index);
        }
        //--------------------------------------------------------------------------
        template<>
        inline bool luaTestType<bool>(lua_State* L, int index)
        {
            return !lua_isnil(L, index) && lua_isboolean(L, index);
        }
        //--------------------------------------------------------------------------
        template<>
        inline bool luaTestType<double>(lua_State* L, int index)
        {
            return !lua_isnil(L, index) && lua_isnumber(L, index);
        }
        //--------------------------------------------------------------------------

        template<class T>
        inline typename std::enable_if<checker<T>::isTable, bool>::type
        luaTestType(lua_State* L, int index)
        {
            return !lua_isnil(L, index) && lua_istable(L, index);
        }
        //--------------------------------------------------------------------------
        template<class T>
        inline typename std::enable_if<checker<T>::isShared, bool>::type
        luaTestType(lua_State* L, int index)
        {
            return !lua_isnil(L, index) && lua_islightuserdata(L, index);
        }
        //--------------------------------------------------------------------------
        template <class T>
        inline T testGetParam(lua_State* L, int index)
        {
            if (!luaTestType<T>(L, index))
                FATAL_RISE("Invalid parameter type comes from LUA to C++.");

            return luaParam<T>(L, index);
        }
        //--------------------------------------------------------------------------
        template <class T>
        inline T testGetGlobal(lua_State* L, const std::string& name)
        {
            lua_getglobal(L, name.c_str());
            int index = lua_gettop(L);
            bool test = luaTestType<T>(L, index);
            if (!test)
            {
                lua_pop(L, 1);
                FATAL_RISE("Invalid parameter type comes from LUA to C++.");
            }
            T res = luaParam<T>(L, index);
            lua_pop(L, 1);
            return res;
        }

        //other classes may implement this to do luaPush
        class LuaPushable
        {
        public:
            LuaPushable() = default;
            virtual ~LuaPushable(){}
            virtual void luaPush(lua_State *L) const = 0;
        };
#ifdef QT_CORE_LIB
        inline void luaPushVariant(lua_State* L, const QVariant& var);
#endif
        struct LuaPushImpl
        {
#ifdef QT_CORE_LIB
            //overloads, shorter to write then specializations
            static void luaPush(lua_State *L, const QByteArray& s)
            {
                luaPush(L, s.constData(), static_cast<size_t>(s.size()));
            }

            static void luaPush(lua_State *L, const QString& s)
            {
                luaPush(L, s.toUtf8());
            }
#endif
            static void luaPush(lua_State *L, const std::string& s)
            {
                luaPush(L, s.c_str(), s.size());
            }

            static void luaPush(lua_State *L, int s)
            {
                lua_pushinteger(L, s);
            }

            static void luaPush(lua_State *L, int64_t s)
            {
                lua_pushinteger(L, s);
            }

            static void luaPush(lua_State *L, bool s)
            {
                lua_pushboolean(L, static_cast<int>(s));
            }

            static void luaPush(lua_State *L, double s)
            {
                lua_pushnumber(L, s);
            }

            static void luaPush(lua_State *L, const char *ptr, size_t size)
            {
                lua_pushlstring(L, ptr, size);
            }

            static void luaPush(lua_State *L, const LuaPushable& c)
            {
                c.luaPush(L);
            }

            static void luaPush(lua_State *L, const LuaPushable* c)
            {
                c->luaPush(L);
            }
#ifdef  QT_CORE_LIB
            static void luaPush(lua_State *L, const QVariant& var)
            {
                luaPushVariant(L, var);
            }
#endif
        };

        template<class T>inline
        typename std::enable_if<checker<T>::isGeneric, void>::type
        luaPush(lua_State *L, T what)
        {
            LuaPushImpl::luaPush(L, what);
        }


        template<class T> inline
        typename std::enable_if<checker<T>::isShared, void>::type
        luaPush(lua_State *L, const T& sp)
        {
            auto p = sp.get();
            if (p)
            {
                lua_pushlightuserdata(L, sp.get());
            }
            else
                lua_pushnil(L);
        }

        template<class T> inline
        typename std::enable_if<checker<T>::isEnum, void>::type
        luaPush(lua_State *L, T sp)
        {
            lua_pushinteger(L, static_cast<int>(sp));
        }

        template<class T> inline
        typename std::enable_if<checker<T>::isVector, void>::type
        luaPush(lua_State *L, const T& sp)
        {
            lua_newtable(L);
            for (int i = 0, index = 1, sz = sp.size(); i < sz; i++)
            {
                lua_pushinteger(L, index++);
                luaPush(L, sp.at(i));
                lua_settable(L, -3);
            }
        }

        template<class T> inline
        typename std::enable_if<checker<T>::isMap, void>::type
        luaPush(lua_State *L, const T& sp)
        {
            lua_newtable(L);
            for (const auto& v : sp)
            {
                luaPush(L, v.first);
                luaPush(L, v.second);
                lua_settable(L, -3);
            }
        }

#ifdef QT_CORE_LIB
        inline void luaPushVariant(lua_State* L, const QVariant& var)
        {
            if (!var.isValid() || var.isNull())
                lua_pushnil(L);
            else
                switch (var.type())
                {
                    case QVariant::ByteArray:
                    case QVariant::Char:
                    case QVariant::String:
                    {
                        QString s = var.toString();
                        luaPush(L, s);
                    }
                        break;
                    case QVariant::Bool:
                        lua_pushboolean(L, var.toBool());
                        break;
                    case QVariant::Double:
                        lua_pushnumber(L, variantTo<lua_Number>(var));
                        break;
                    case QVariant::Int:
                    case QVariant::UInt:
                    case QVariant::LongLong:
                    case QVariant::ULongLong:
                        lua_pushinteger(L, variantTo<lua_Integer>(var));
                        break;
                    case QVariant::Date:
                        lua_pushinteger(L, var.toDate().toJulianDay());
                        break;
                    case QVariant::StringList:
                        luaPush(L, var.toStringList());
                        break;
                    default:
                        FATAL_RISE("Invalid variant to lua runtime conversion.");
                        //break;
                }
        }
#endif
        inline void luaPushMany(lua_State* L){(void)L;} //end of recursion
        template <class T, class ...Args>
        inline void luaPushMany(lua_State* L, const T val, Args... args)
        {
            luaPush(L, val);
            luaPushMany(L, args...);
        }

        template <typename... Args>
        inline void luaPushFunc(lua_State* L, const std::string& funcName, Args... args)
        {
            lua_getglobal(L, funcName.c_str());
            luaPushMany(L, args...);
        }

        template <class T>//pushes single field of the current table on lua stack
        inline void luaPushField(lua_State* L, const std::string& field_name, const T& value)
        {
            luaPush(L, field_name);
            luaPush(L, value);
            lua_settable(L, -3);
        }

        template <class T> //pushes single field of the current table on lua stack
        inline void luaPushField(lua_State* L, const char* field_name, const T& value)
        {
            luaPushField(L, std::string(field_name), value);
        }

#ifdef QT_CORE_LIB
        template <class T> //pushes single field of the current table on lua stack
        inline void luaPushField(lua_State* L, const QString& field_name, const T& value)
        {
            auto f = field_name.toStdString();
            luaPushField(L, f, value);
        }
#endif


//NILP should be used to do lua_pushnil(L)
#define NILP std::shared_ptr<char>(nullptr)
    }
}
#endif // LUA_PARAMS

