#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <stdexcept>

namespace utility
{

    //makes global static instance of T, T must have default constructor
    template <class T>
    inline T& globalInstance()
    {
        static T inst;
        return inst;
    }

    template <class T>
    const inline T& globalInstanceConst()
    {
        static const T inst;
        return inst;
    }

    //if this class is sublassed it allows to create only 1 instance of the child at runtime
    template <class T>
    class ItCanBeOnlyOne
    {
    private:
        using T_ptr = T*;

        static T_ptr& staticPtr()
        {
            static T_ptr ptr = nullptr;
            return ptr;
        }

        std::atomic_flag& lock()
        {
            static std::atomic_flag locked = ATOMIC_FLAG_INIT;
            return locked;
        }
    protected:
        ItCanBeOnlyOne()
        {
            if (lock().test_and_set())
            {
                std::string s = "Second instance of " + std::string(typeid(T).name())+" is now allowed.";
                throw std::runtime_error(s);
            }
            else
                staticPtr() = static_cast<T*>(this);
        }
        virtual ~ItCanBeOnlyOne()
        {
            lock().clear();
        }
    public:
        static T* instance()
        {
            return staticPtr();
        }
    };
}
