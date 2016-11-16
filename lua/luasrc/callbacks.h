#ifndef CALLBACKS_H
#define CALLBACKS_H


#include <iostream>
#include <functional>
#include <stdexcept>
#include <typeinfo>
#include <string>
#include <map>
#include <memory>

//all black magic below allows to use lambdas as C-functions pointers ... in trade of - it must set unique ID at compile time amoung whole project
namespace lambdacb {

    template <typename Function>
    struct function_traits
            : public function_traits<decltype(&Function::operator())>
    {};

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const>
    {
        typedef ReturnType (*pointer)(Args...);
        typedef std::function<ReturnType(Args...)> function;
    };

    template <typename Function>
    typename function_traits<Function>::pointer
    to_function_pointer (const Function& lambda)
    {
        return static_cast<typename function_traits<Function>::pointer>(lambda);
    }

    template <typename Function>
    typename function_traits<Function>::function
    to_function (const Function& lambda)
    {
        return static_cast<typename function_traits<Function>::function>(lambda);
    }
    typedef void(*funcptr_type)();


    //some more black magic

    template <const size_t _UniqueId, typename _Res, typename... _ArgTypes>
    struct fun_ptr_helper
    {
    public:
        typedef std::function<_Res(_ArgTypes...)> function_type;

        static void bind(function_type&& f)
        { instance().fn_.swap(f); }

        static void bind(const function_type& f)
        { instance().fn_=f; }

        static _Res invoke(_ArgTypes... args)
        { return instance().fn_(args...); }

        typedef decltype(&fun_ptr_helper::invoke) pointer_type;
        static pointer_type ptr()
        { return &invoke; }

    private:
        static fun_ptr_helper& instance()
        {
            static fun_ptr_helper inst_;
            return inst_;
        }

        fun_ptr_helper() {}

        function_type fn_;
    };


    //_UniqueId must be manually set at compile time to unique value, that generates unique static stub

    template <const size_t _UniqueId, typename _Res, typename... _ArgTypes>
    typename fun_ptr_helper<_UniqueId, _Res, _ArgTypes...>::pointer_type
    get_fn_ptr(const std::function<_Res(_ArgTypes...)>& f)
    {
        fun_ptr_helper<_UniqueId, _Res, _ArgTypes...>::bind(f);
        return fun_ptr_helper<_UniqueId, _Res, _ArgTypes...>::ptr();
    }


    template <const size_t _UniqueId, typename Function>
    funcptr_type
    to_callback (const Function& lambda)
    {
        return (funcptr_type)get_fn_ptr<_UniqueId>(to_function(lambda));
    }
}
#endif // CALLBACKS_H
