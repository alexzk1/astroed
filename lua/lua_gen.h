#ifndef LUA_GEN_H
#define LUA_GEN_H
#include <iostream>
#include <sstream>
#include <string>

namespace luavm
{
    class LuaGenerator
    {
    protected:
        LuaGenerator() = default;
        virtual ~LuaGenerator(){}
    public:
        virtual void generateLuaCode(std::ostream& out) const = 0;
        std::string generateLuaString() const
        {
            std::stringstream os(std::ios_base::out);
            generateLuaCode(os);
            return os.str();
        }
    };
}

#endif // LUA_GEN_H
