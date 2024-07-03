#pragma once

#include <coroutine>
#include <iostream>
#include <thread>
#include <memory>
#include <vector>
#include <queue>
#include <optional>

using namespace std;


class logger : ostream
{
    public:
    template<typename T>
    ostream& operator << (T const& val)
    {
        if(true)
        {
            cout << val;
        }
    }
};

logger MYLOG;


struct JobQ
{

    static JobQ& get_instance()
    {
        static JobQ instance;
        return instance;
    }

    ~JobQ(){};

    template<typename T>
    void AddAwaits(coroutine_handle<T> h)
    {
        coros.push(h);
    }

private:
    
    queue<coroutine_handle<>> coros;

    static void runner(JobQ *ins)
    {
        static mutex m;
        while(true)
        {
            m.lock();
            if(ins->coros.size() > 0)
            {
                MYLOG << "Found more coros to run." << endl;
                auto &c = ins->coros.front();
                ins->coros.pop();
                MYLOG << "Running." << endl;
                c.resume();
            }
            m.unlock();
            usleep(10);
        }
    }

    vector<thread> threads;
    static const size_t num_threads = 4;
    JobQ()
    {
        for(int i=0; i< num_threads; ++i)
        {
            threads.push_back(thread(this->runner, this));
        }
    }

public:
    JobQ(JobQ const &) = delete;
    void operator=(JobQ const &) = delete;
};


template <class R>
struct Task
{
    struct Promise;
    using promise_type = Promise;
    using coro_handle = coroutine_handle<promise_type>;

    coro_handle m_handle;

    Task(coro_handle handle) : m_handle(handle) {MYLOG << "Task(handle)" << &handle << endl;}
    Task(Task &other) : m_handle(other.m_handle) { MYLOG << "Task(other)" << &other.m_handle << endl; other = nullptr;}

    bool await_ready() {MYLOG << "await_ready" << endl; return false; }


    template <class T>
    void await_suspend(coroutine_handle<T> prom) { MYLOG << "await_suspend" << endl; JobQ::get_instance().AddAwaits(prom); }

    
    R await_resume() { MYLOG << "await_resume" << endl; return m_handle.promise().ret;}
    

    
        struct Promise
        {
            coro_handle get_return_object() { return {coro_handle::from_promise(*this)}; }
            // std::suspend_always initial_suspend() noexcept { MYLOG<<"initial_suspend"<<endl; JobQ::get_instance().AddAwaits(get_return_object()); return {}; }
            std::suspend_always initial_suspend() noexcept { MYLOG<<"initial_suspend"<<endl; JobQ::get_instance().AddAwaits(get_return_object()); return {}; }
            std::suspend_always final_suspend() noexcept { MYLOG<<"final_suspend"<<endl; return {}; }

            R ret;
            auto value(){MYLOG<<"value"<<endl; return ret.value_or(0);}

            
            void return_value(R &&r) {MYLOG<<"return_value"<<endl; ret = r;}

            void unhandled_exception() {throw exception();}
        };

    
};


template <>
struct Task<void>::Promise
{
    using promise_type = Promise;
    using coro_handle = coroutine_handle<promise_type>;
    coro_handle get_return_object() { return {coro_handle::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { MYLOG<<"initial_suspend"<<endl; JobQ::get_instance().AddAwaits(get_return_object()); return {}; }
    std::suspend_always final_suspend() noexcept { MYLOG<<"final_suspend"<<endl; return {}; }
    void unhandled_exception() {throw exception();}

    void return_void(){MYLOG<<"return_void"<<endl;}
};

template<>
void Task<void>::await_resume() { MYLOG << "await_resume(void)" << endl;}


/*template <>
struct Task<void> : std::coroutine_handle<Promise<void>>
{
    using promise_type = Promise<void>;


    bool await_ready() { return false; }

    template <class T>
    void await_suspend(coroutine_handle<Promise<T>> prom) { JobQ::get_instance().AddAwaits(prom); }
    void await_resume() {}
};*/



