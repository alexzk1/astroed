#ifndef LUA_GEN_H
#define LUA_GEN_H
#include <iostream>
#include <sstream>
#include <string>
#include "lua_params.h"
#include "luasrc/customlua.h"

namespace luavm
{
    class ProjectSaver
    {
    protected:
        ProjectSaver() = default;
        virtual ~ProjectSaver(){}
    public:
        virtual void generateProjectCode(std::ostream& out) const = 0;
        std::string generateProjectCodeString() const
        {
            std::stringstream os(std::ios_base::out);
            generateProjectCode(os);
            return os.str();
        }
    };

    class ProjectLoader
    {
    protected:
        ProjectLoader() = default;
        virtual ~ProjectLoader(){}
    public:
        virtual void loadProjectCode(const std::string& src) = 0;
    };
}

#endif // LUA_GEN_H
