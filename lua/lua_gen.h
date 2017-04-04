#ifndef LUA_GEN_H
#define LUA_GEN_H
#include <iostream>
#include <sstream>
#include <string>

namespace luavm
{
    class ProjectSaver
    {
    protected:
        ProjectSaver() = default;
        virtual ~ProjectSaver(){}
    public:
        virtual void generateProjectCode(std::ostream& out) const = 0;
        std::string generateProjectString() const
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
        virtual void loadProjectCode(std::ostream& inp) const = 0;
    };
}

#endif // LUA_GEN_H
