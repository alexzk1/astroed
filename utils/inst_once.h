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
    public:
        using T_ptr = T*;
    private:
        using self_ptr_t = ItCanBeOnlyOne<T>*;
        static self_ptr_t& staticPtr()
        {
            static self_ptr_t ptr = nullptr;
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
                staticPtr() = this;
        }
        virtual ~ItCanBeOnlyOne()
        {
            lock().clear();
        }
    public:
        static T_ptr instance()
        {
            return dynamic_cast<T_ptr>(staticPtr());
        }
    };
}
