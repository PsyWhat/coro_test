#pragma once

#include <coroutine>
#include <iostream>
#include <thread>
#include <memory>
#include <vector>
#include <queue>
#include <optional>

using namespace std;

constexpr bool log_enable = false;

class logger : private streambuf, public ostream
{
public:
    logger() : ostream(this){}

private:
    virtual int overflow(int a)
    {
        if(log_enable)
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

template <class R>
struct Task : coroutine_handle<Promise<R>>
{
    using promise_type = Promise<R>;
    using coro_handle = coroutine_handle<promise_type>;

    Task(coro_handle const &handle) noexcept : coro_handle(handle)
    {
        MYLOG << "Task(handle)" << &handle << endl;
    }
    Task(Task<R> &other) noexcept : coro_handle(other)
    {
        MYLOG << "Task(other)" << &other << endl;
    }

    bool await_ready() noexcept
    {
        MYLOG << "await_ready" << /*typeid(this).name() <<*/ endl;
        return coro_handle::done();
    }

    template <class T>
    void await_suspend(coroutine_handle<T> prom) noexcept
    {
        MYLOG << "await_suspend" << /* typeid(this).name() << */ endl;
        //JobQ::get_instance().AddAwaits(prom);
        /*if(!prom.done() && prom.address() != nullptr)*/
        prom();
        //coro_handle::resume();
    }

    /*promise_type promise()
    {
        return static_cast<promise_type>(coro_handle::promise());
    }*/

    R await_resume() noexcept
    {
        MYLOG << "await_resume" << /* typeid(this).name() << */ endl;
        return coro_handle::promise().ret;
    }
};

template <typename R>
struct Promise
{
    using promise_type = Promise<R>;
    using coro_handle = Task<R>;

    coro_handle get_return_object()
    {
        return {coro_handle::from_promise(*this)};
    }
    // std::suspend_always initial_suspend() noexcept { MYLOG<<"initial_suspend"<<endl; JobQ::get_instance().AddAwaits(get_return_object()); return {}; }
    coro_handle initial_suspend() noexcept
    {
        MYLOG << "initial_suspend" << endl;
        return get_return_object();
    }
    coro_handle final_suspend() noexcept
    {
        MYLOG << "final_suspend" << endl;
        return get_return_object();
    }

    R ret;
    /*auto value()
    {
        MYLOG << "value" << endl;
        return ret.value_or(0);
    }*/

    void return_value(R &&r)
    {
        MYLOG << "return_value" << endl;
        ret = r;
    }

    void unhandled_exception() { throw exception(); }
};

template <>
struct Promise<void>
{
    using promise_type = Promise<void>;
    using coro_handle = Task<void>;
    coro_handle get_return_object() { return {coro_handle::from_promise(*this)}; }
    coro_handle initial_suspend() noexcept
    {
        MYLOG << "initial_suspend" << endl;
        //JobQ::get_instance().AddAwaits(get_return_object());

        return get_return_object();
    }
    coro_handle final_suspend() noexcept
    {
        MYLOG << "final_suspend" << endl;
        return get_return_object();
    }
    void unhandled_exception() { throw exception(); }

    void return_void() { MYLOG << "return_void" << endl; }
};

template <>
void Task<void>::await_resume() noexcept { MYLOG << "await_resume(void)" << endl; }

/*template <>
struct Task<void> : std::coroutine_handle<Promise<void>>
{
    using promise_type = Promise<void>;


    bool await_ready() { return false; }

    template <class T>
    void await_suspend(coroutine_handle<Promise<T>> prom) { JobQ::get_instance().AddAwaits(prom); }
    void await_resume() {}
};*/
