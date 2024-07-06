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

constexpr bool log_enable = false;

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

struct JobQ
{

    static JobQ &get_instance()
    {
        static JobQ instance;
        return instance;
    }

    ~JobQ(){};

    template <typename T>
    void AddAwaits(coroutine_handle<T> h)
    {
        coros.push(h);
    }

private:
    queue<coroutine_handle<>> coros;

    static void runner(JobQ *ins)
    {
        static mutex m;
        while (true)
        {
            m.lock();
            if (ins->coros.size() > 0)
            {
                // MYLOG << "Found more coros to run." << endl;
                auto &c = ins->coros.front();
                ins->coros.pop();
                if (c.address() != nullptr)
                {
                    if (!c.done())
                    {
                        MYLOG << "Running." << endl;
                        c.resume();
                        MYLOG << "Done Running." << endl;
                    }
                    else
                    {
                        MYLOG << "Coro is done" << endl;
                    }
                }
                else
                {
                    MYLOG << "Coro is null" << endl;
                }
            }
            m.unlock();
            usleep(10);
        }
    }

    vector<thread> threads;
    static const size_t num_threads = 4;
    JobQ()
    {
        for (int i = 0; i < num_threads; ++i)
        {
            threads.push_back(thread(this->runner, this));
        }
    }

public:
    JobQ(JobQ const &) = delete;
    void operator=(JobQ const &) = delete;
};

template <class R>
struct Promise;

template <class R = void>
struct Task
{
    using promise_type = Promise<R>;
    using coro_handle = coroutine_handle<promise_type>;

    coro_handle m_handle;
    std::conditional_t<std::is_same_v<R, void>, std::variant<std::monostate, std::exception_ptr>, std::variant<std::monostate, std::exception_ptr, R>> val_;

    std::variant<std::monostate, std::coroutine_handle<>> awaiting;

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
        return m_handle.done();
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
        // JobQ::get_instance().AddAwaits(prom);
        /*if(!prom.done() && prom.address() != nullptr)*/
        // if(m_handle.done()) m_handle.destroy();
        // prom();
        // coro_handle::resume();
        //prom();
        awaiting = prom;
        prom();
    }

    /*promise_type promise()
    {
        return static_cast<promise_type>(coro_handle::promise());
    }*/

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
            // auto val = m_handle.promise().value();
            MYLOG << "got value" << endl;
            if (std::holds_alternative<R>(val_))
            {
                return std::get<R>(val_);
            }
            if (std::holds_alternative<std::exception_ptr>(val_))
            {
                throw std::get<std::exception_ptr>(val_);
            }
            throw std::runtime_error("Unknown return?");
        }
    }
};

template <typename R>
struct type_promise_type
{
    using promise_type = Promise<R>;
    using coro_handle = coroutine_handle<promise_type>;

    Task<R> *task = nullptr;

    bool valueset = false;

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
        val_ = r;
        /*         if (std::holds_alternative<R>(val_))
                {
                    MYLOG << "GOOD" << sizeof(val_) << endl;
                } */
        for (int i = 0; i < 8; ++i)
        {
            MYLOG << hex << *(reinterpret_cast<int *>(&val_) + i) << dec << " ";
        }
        MYLOG << endl;
        valueset = true;
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
    // coro_handle get_return_object() { return {coro_handle::from_promise(*this)}; }

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

    /* coro_handle initial_suspend() noexcept
    {
        MYLOG << "initial_suspend" << endl;
        //JobQ::get_instance().AddAwaits(get_return_object());

        return get_return_object();
    } */

    suspend_never initial_suspend() noexcept
    {
        MYLOG << "initial_suspend " << get_return_object().address() << endl;
        return {};
    }

    suspend_always final_suspend() noexcept
    {
        MYLOG << "final_suspend " << get_return_object().address() << endl;
        /*         MYLOG << "final_suspend" << endl;
                get_return_object().destroy();
                return get_return_object(); */

        return {};
    }

    // task<T>* task_ptr_{ nullptr };
};

/*template <>
struct Task<void> : std::coroutine_handle<Promise<void>>
{
    using promise_type = Promise<void>;


    bool await_ready() { return false; }

    template <class T>
    void await_suspend(coroutine_handle<Promise<T>> prom) { JobQ::get_instance().AddAwaits(prom); }
    void await_resume() {}
};*/
