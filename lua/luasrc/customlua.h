#ifndef CUSTOMLUA_H
#define CUSTOMLUA_H

#include <memory>
#include <memory.h>
#include "lua.hpp"
#include "callbacks.h"
#include <deque>
#include "lua_allocator.h"

/*
 * Warning! I use modified lua sources where lua_CFunction is redefined as
 * typedef std::function<int(lua_state*)>* lua_CFunction;
 *
 * this will allow to use classes directly.
*/
namespace luavm
{
    class LuaDebug;
    constexpr auto LUA_UNIQUE_CB_BEGIN = 1024;

    //lambdas inside constructor init list make Qtcreator auto-align code future (ctrl+A, ctrl+I) crazy. So just moved out as macros.
#define LUA_DELETOR [&](lua_State* p){if (p != nullptr)lua_close(p);}
#define LUA_VOID_DELETOR [&](lua_State* p){(void)p;}

    class lua_vm_exception: public std::runtime_error
    {
    public:
        lua_vm_exception() = default;
        explicit lua_vm_exception(const std::string& s):std::runtime_error(s)
        {}
    };

    class LuaVM
    {
    public:
        typedef std::function<int(lua_State*)> FUNCTOR;
        typedef std::shared_ptr<FUNCTOR> functorptr;
    private:
        lua_allocator allocator;
        std::shared_ptr<lua_State> state;
        //so complex stuff with pointers is needed, because containers may reallocate-moving object, but that will change address which is on lua stack
        std::deque<functorptr> functions; //I put here deque instead vector ...and that might allow to avoid functorptr everywhere...


        static void* allocactorFunc(void* This, void *ptr, size_t osize, size_t nsize)
        {
            if (This)
            {
                auto pt = static_cast<LuaVM*>(This);
                if (pt->state && !lua_mutex(pt->state.get())->count())
                {
                    throw lua_vm_exception("Requested memory alloc/realloc/free in unlocked state.");
                }
                return pt->allocator.allocate(static_cast<char*>(ptr), osize, nsize);
            }
            return nullptr;
        }

    public:
        LuaVM(const LuaVM& ) = delete;
        LuaVM& operator = (const LuaVM&) = delete;

        LuaVM() :
            allocator(),
            state(nullptr)
          //state(luaL_newstate(),LUA_DELETOR) //do not use default allocator - it does not work with threads at all
        {
            state.reset(lua_newstate(allocactorFunc, this),LUA_DELETOR);
            //let it be for now so - all libs are loaded
            auto s = state.get();
            if (s == nullptr)
                throw lua_vm_exception("LuaVM allocation error.");
            luaL_openlibs(s);
        }

        //        //thread constructor, it should not delete pointer - it will be GC'ed by lua
        //        explicit LuaVM(lua_State* thread) :  state(thread,LUA_VOID_DELETOR)
        //        {
        //        }

        //dont want to do full lua as OOP ...so this overload allows simple usage of LuaVM in calls to globals
        operator lua_State*() const
        {
            return state.get();
        }

        std::string popError(lua_State* L = nullptr) const
        {
            if (!L)
                L = state.get();
            if (lua_isstring(L, -1))
            {
                std::string err(lua_tostring(L, -1));
                lua_pop(L,1);
                return err;
            }
            return "No lua-error description on lua-stack. Most likelly caused by lua_resume.";
        }

        template <class Src>
        int load(const Src& src, const std::string& chunkName = "defchunk", const std::string& mode = "bt") const
        {
            //FIXME: that may work for 1 plugin only - please re-think it because it is STATIC in fact
            bool done = false;
            auto reader = [&src, &done](lua_State *L,
                          void *data,
                          size_t *size) -> const char *
            {
                (void)L;
                (void)data;
                const char* r = (done)?nullptr:const_cast<Src*>(&src)->data();
                *size = src.size();
                done = true;
                return r;
            };

            return lua_load(state.get(), (lua_Reader)lambdacb::to_callback<LUA_UNIQUE_CB_BEGIN + 1>(reader), nullptr, chunkName.c_str(), mode.c_str());
        }

        //dont use it for short living things like lambda, use for class-methods ...because internal vector is never grows down
        //also functors must live until this object LuaVM exists
        void setGlobalFunctor(const std::string& name, FUNCTOR&& cb, int nupvals = 0) &
        {
            functions.push_back(functorptr(new FUNCTOR(std::move(cb))));
            lua_pushcclosure(state.get(), functions.back().get(), nupvals);
            lua_setglobal(state.get(), name.c_str());
        }

        void setGlobalInt(const std::string &name, const int val) const
        {
            lua_pushinteger(state.get(), val);
            lua_setglobal(state.get(), name.c_str());
        }

        void setGlobalString(const std::string &name, const std::string &val) const
        {
            lua_pushstring(state.get(), val.c_str());
            lua_setglobal(state.get(), name.c_str());
        }

        void disableGlobal(const std::string& name)
        {
            lua_pushnil(state.get());
            lua_setglobal(state.get(), name.c_str());
        }

        void setBaseSafety()
        {
            disableGlobal("io");
            disableGlobal("loadfile");
            disableGlobal("os");
        }

        template <class Src>
        void doString(const Src& src, const std::string& dc="main") const
        {
            throwIf(load(src, dc));
            throwIf(lua_pcall(state.get(), 0, 0, 0));
        }

        void throwIf(int code) const
        {
            if (code != LUA_OK && code != LUA_YIELD)
            {
                throw lua_vm_exception(popError() + "\nCode :" + std::to_string(code) + "\n");
            }
        }

        virtual ~LuaVM()
        {
            state.reset();
        }
    };

#undef LUA_DELETOR
#undef LUA_VOID_DELETOR

    inline void l_pushtablestring(lua_State* L , const std::string& key , const std::string& value)
    {
        lua_pushstring(L, key.c_str());
        lua_pushstring(L, value.c_str());
        lua_settable(L, -3);
    }

    class LuaDebug
    {
    private:
        lua_State* vm;
        std::shared_ptr<lua_Debug> ar;
        bool f1;
        bool f2;
    public:
        explicit LuaDebug(lua_State* vm) : vm(vm), ar(new lua_Debug), f1(false), f2(false)
        { }

        bool getStack(int level)
        {
            f1 = static_cast<bool>(lua_getstack(vm, level, ar.get()));
            return f1;
        }

        bool getInfo(const std::string& what, int level = 1)
        {
            //the parameter ar must be a valid activation record that was filled by a previous call to lua_getstack or given as argument to a hook (see lua_Hook).
            if (f1 || getStack(level))
            {
                f2 = static_cast<bool>(lua_getinfo(vm, what.c_str(), ar.get()));
            }
            return f2;
        }

        std::string getLocal(int n)
        {
            if (f1 || getStack(1))
            {
                return lua_getlocal(vm, ar.get(), n);
            }
            return nullptr;
        }

        std::string parseInfoName() const
        {
            std::string res;
            if (f2)
                res = ar->name;
            return res;
        }

        std::string parseInfoNameWhat() const
        {
            std::string res;
            if (f2)
                res = ar->namewhat;
            return res;
        }

        std::string parseInfoWhat() const
        {
            std::string res;
            if (f2)
                res = ar->what;
            return res;
        }

        std::string parseInfoSource() const
        {
            std::string res;
            if (f2)
                res = ar->source;
            return res;
        }

        std::string parseInfoShortSource() const
        {
            std::string res;
            if (f2)
                res = ar->short_src;
            return res;
        }

        int parseInfoCurrentLine() const
        {
            return ar->currentline;
        }

        int parseInfoNUPS() const
        {
            return ar->nups;
        }

        int parseInfoLineDefined() const
        {
            return ar->linedefined;
        }

        int parseInfoLastLineDefined() const
        {
            return ar->lastlinedefined;
        }

        void reset()
        {
            f1 = f2 = false;
            ar.reset(new lua_Debug);
        }

        std::string lineDump()
        {
            getInfo("nSl");
            return "Error at line " + std::to_string(parseInfoCurrentLine())+" ("+parseInfoShortSource()+").";
        }

        std::string longDump()
        {
            std::string res = "Stack trace:\n";
            for (int i = 1; i < 100; i++)
            {
                if (getStack(i) && getInfo("nSl", i))
                {
                    res += "Error at line " + std::to_string(parseInfoCurrentLine())+" ("+parseInfoShortSource()+").\n";
                }
                else
                    break;
            }
            return res;
        }
    };
}

#endif // CUSTOMLUA_H


