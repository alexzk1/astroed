#ifndef LUAAEVM_H
#define LUAAEVM_H

#include "../luasrc/customlua.h"

class LuaAeVm
{
private:
    luavm::LuaVM vm;
public:
    LuaAeVm();
};

#endif // LUAAEVM_H
