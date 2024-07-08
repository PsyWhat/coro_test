#pragma once

#include <coroutine>
#include <iostream>
#include <thread>
#include <memory>
#include <vector>
#include <queue>
#include <optional>
#include <variant>
#include <sstream>

using namespace std;

constexpr bool log_enable = true;

class logger : private streambuf, public ostream
{
public:
    logger() : ostream(this) {}

private:
    virtual int overflow(int a)
    {
        if (log_enable)
            cout.put(a);
        return 0;
    }
};

ostream &MYLOG = *(new logger());


template <class R>
struct Promise;

template <class R = void>
struct Task
{
    using promise_type = Promise<R>;
    using coro_handle = coroutine_handle<promise_type>;

    coro_handle m_handle;
    std::conditional_t<std::is_same_v<R, void>, std::variant<std::monostate, std::exception_ptr>, std::variant<std::monostate, std::exception_ptr, R>> val_;
    bool isawaiting=false;

    //std::variant<std::monostate, std::coroutine_handle<>> awaiting;

    Task(coro_handle const &&handle) noexcept : m_handle(handle), val_(m_handle.promise().val_)
    {
        MYLOG << "Task(handle)" << handle.address() << " " << this << endl;
        m_handle.promise().task = this;
    }
    Task(Task<R> &other) noexcept : m_handle(other.m_handle), val_(other.val_)
    {
        MYLOG << "Task(other)" << &other << endl;

        for (int i = 0; i < 8; ++i)
        {
            MYLOG << hex << *(reinterpret_cast<int *>(&val_) + i) << dec << " ";
        }
        MYLOG << endl;
    }

    ~Task()
    {
        MYLOG << "~Task()" << endl;
    }

    bool await_ready() noexcept
    {
        MYLOG << "await_ready " << m_handle.done() << " " << m_handle.address() << /*typeid(this).name() <<*/ endl;
        return isawaiting;
    }

    Task<R> operator co_await() noexcept
    {
        MYLOG << "_____ operator co_await" << endl;
        if (m_handle.address())
        {
            MYLOG << "_____ operator co_await m_handle " << m_handle.address() << endl;
            //m_handle();
        }
        MYLOG << "_____ operator co_await after" << endl;
        return *this;
    }

    template <class T>
    void await_suspend(coroutine_handle<T> prom) noexcept
    {
        MYLOG << "await_suspend " << prom.address() << " " << m_handle.address() << /* typeid(this).name() << */ endl;
        prom();
    }


    auto await_resume() noexcept
    {
        MYLOG << "await_resume " << m_handle.address() << " " << endl;

        for (int i = 0; i < 8; ++i)
        {
            MYLOG << hex << *(reinterpret_cast<int *>(&val_) + i) << dec << " ";
        }
        MYLOG << endl;

        if constexpr (std::is_same_v<R, void>)
        {
            MYLOG << "void" << endl;
            return;
        }
        else
        {
            MYLOG << typeid(R).name() << endl;
            MYLOG << "got value" << endl;
            if (std::holds_alternative<R>(val_))
                return std::move(std::get<R>(val_));
            if (std::holds_alternative<std::exception_ptr>(val_))
                throw std::move(std::get<std::exception_ptr>(val_));
            throw std::runtime_error("Unknown return?");
        }
    }
};

template<class>
struct simple_prom;

template <class R = void>
struct InitTask
{
    using promise_type = simple_prom<R>;
    using coro_handle = coroutine_handle<promise_type>;
};

template<class R>
struct simple_prom
{
    auto get_return_object()
    {
        return std::move(coroutine_handle<simple_prom<R>>::from_promise(*this));
    }

    void return_void() {}

    void unhandled_exception() noexcept
    {
        cout << "!!!!!! unhandled exception !!!!!!" << endl;
    }

    auto initial_suspend() noexcept
    {
        return suspend_always{}; //Task<T>(get_return_object());
    }

    auto final_suspend() noexcept
    {
        return suspend_always{};
    }
};

template <typename R>
struct type_promise_type
{
    using promise_type = Promise<R>;
    using coro_handle = coroutine_handle<promise_type>;

    Task<R> *task = nullptr;

    type_promise_type()
    {
        MYLOG << "**type_promise_type() " << this << endl;
    }

    type_promise_type(const type_promise_type &&t)
    {
        MYLOG << "**type_promise_type(const type_promise_type& t) " << this << endl;
    }

    ~type_promise_type()
    {
        MYLOG << "**~type_promise_type() " << this << endl;
    }

    R &&value()
    {
        MYLOG << "value " << typeid(R).name() << " " << &val_ << " " << this << endl;
        MYLOG << endl;
        if (std::holds_alternative<R>(val_))
            return std::move(std::get<R>(val_));
        if (std::holds_alternative<std::exception_ptr>(val_))
            throw std::get<std::exception_ptr>(val_);

        throw std::runtime_error("Bad return");
    }

    void return_value(R &&r)
    {
        MYLOG << "return_value " << typeid(R).name() << " " << r << " " << &val_ << " " << endl;
        val_ = std::move(r);
        for (int i = 0; i < 8; ++i)
        {
            MYLOG << hex << *(reinterpret_cast<int *>(&val_) + i) << dec << " ";
        }
        MYLOG << endl;
    }

    void unhandled_exception() noexcept
    {
        cout << "!!!!!! unhandled exception !!!!!!" << endl;
        val_ = std::current_exception();
    }

    std::variant<std::monostate, std::exception_ptr, R> val_;
};

struct void_promise_type
{
    using promise_type = Promise<void>;
    using coro_handle = coroutine_handle<promise_type>;

    void unhandled_exception() noexcept
    {
        cout << "!!!!!! unhandled exception(void) !!!!!!" << endl;
        ;
        val_ = std::current_exception();
    }

    void return_void() { MYLOG << "return_void" << endl; }

    std::variant<std::monostate, std::exception_ptr> val_;
};


template <typename T>
struct Promise : std::conditional_t<std::is_same_v<T, void>, void_promise_type, type_promise_type<T>>
{

    using promise_type = Promise<T>;
    using coro_handle = coroutine_handle<promise_type>;

    Task<T> *task = nullptr;

    coro_handle get_return_object()
    {
        return std::move(coro_handle::from_promise(*this));
    }


    auto initial_suspend() noexcept
    {
        MYLOG << "initial_suspend " << get_return_object().address() << endl;
        return suspend_never{}; //Task<T>(get_return_object());
    }

    auto  final_suspend() noexcept
    {
        MYLOG << "final_suspend " << get_return_object().address() << endl;
        return suspend_always{};
    }

};




