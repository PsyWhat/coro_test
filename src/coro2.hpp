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
#include <memory>

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


// First task should have void await_ready


ostream &MYLOG = *(new logger());

template <typename R>
struct Promise;

template <typename R>
struct TaskBase
{
public:
    coroutine_handle<> m_handle;
    Promise<R> *m_promise = nullptr;
    bool m_suspend = false;

    TaskBase() : m_promise(nullptr), m_suspend(false) {}
    TaskBase(Promise<R> *promise, coroutine_handle<> &&handle, bool suspend) : m_promise(promise), m_handle(std::move(handle)), m_suspend(suspend){};

    bool await_ready()
    {
        MYLOG << "TaskBase::await_ready() " << (m_suspend ? "true" : "false") << " m_handle:" << m_handle.address() << endl;
        this->m_promise->ptr = this;
        m_handle();
        return true;
    }
    bool await_suspend(coroutine_handle<> from)
    {
        MYLOG << "void TaskBase::await_suspend(coroutine_handle<> " << from.address() << ") m_handle:" << this->m_handle.address() << " suspend:"<< (m_suspend ? "true" : "false") << endl;
        //if(from.address() == m_handle.address() && !m_suspend)
        //    from();
        m_handle = from;
        return true;
    }
};

template <typename R>
struct TaskValue : TaskBase<R>
{
    std::variant<std::monostate, std::exception_ptr, R> value_;
    TaskValue() {}
    TaskValue(Promise<R> *promise, coroutine_handle<> &&handle, bool suspend) : TaskBase<R>(promise, std::move(handle), suspend) {}

    R await_resume();
};

struct TaskVoid : TaskBase<void>
{
    std::variant<std::monostate, std::exception_ptr> value_;
    TaskVoid() {}
    TaskVoid(Promise<void> *promise, coroutine_handle<> &&handle, bool suspend) : TaskBase<void>(promise, std::move(handle), suspend) {}
    void await_resume();
};


template <typename R = void>
struct TaskAwait : std::conditional_t<std::is_same_v<R, void>, TaskVoid, TaskValue<R>>
{
    using promise_type = Promise<R>;
    using coro_handle = coroutine_handle<promise_type>;
    using base_class = std::conditional_t<std::is_same_v<R, void>, TaskVoid, TaskValue<R>>;
    
    TaskAwait()
    {
        MYLOG << "TaskAwait()" << endl;
    }

    TaskAwait(TaskAwait &other): base_class() //: base_class(other.m_promise, std::move(other.m_handle), other.m_suspend)
    {
        MYLOG << "TaskAwait(Task &other" << &other << ")" << this << " m_promise:" << other.m_promise << " m_handle:" << other.m_handle.address() << endl;
        this->m_promise = other.m_promise;
        this->m_handle = other.m_handle;
        this->m_suspend = other.m_suspend;
    }

    TaskAwait(TaskAwait &&other) : base_class() //: base_class(other.m_promise, std::move(other.m_handle), other.m_suspend)
    {
        MYLOG << "TaskAwait(Task &&other" << &other << ") " << this << " m_promise:" << other.m_promise << " m_handle:" << other.m_handle.address() << endl;
        this->m_promise = other.m_promise;
        this->m_handle = other.m_handle;
        this->m_suspend = other.m_suspend;
    }

    TaskAwait(coro_handle handle): base_class()
    {
        MYLOG << "TaskAwait(coro_handle handle)" << this << endl;
        this->m_handle = handle;
        this->m_promise = &handle.promise();
    }

    TaskAwait(Promise<R> *promise, coroutine_handle<> handle, bool suspend) : base_class() //: base_class(promise, std::move(handle), suspend)
    {
        MYLOG << "TaskAwait( promise:" << promise << " Handle:" << handle.address() << " Suspend:" << (suspend ? "true" : "false") << ")" << this << endl;
        this->m_handle = handle;
        this->m_promise = promise;
    }

    ~TaskAwait()
    {
        MYLOG << "~TaskAwait: " << this->m_handle.address() << endl << "####" << this << endl;
    }

    TaskAwait operator co_await()
    {
        MYLOG << "    TaskAwait operator co_await() " << this->m_suspend << "   " << this->m_handle.address() << endl;
        // if(!TaskBase::m_handle.done())
        //     TaskBase::m_handle();
        if(this->m_suspend)
            return std::move(TaskAwait(this->m_promise, this->m_handle, !this->m_suspend));
        else
            return std::move(*this);
    }

};

template <typename R = void>
struct Task
{
    using promise_type = Promise<R>;
    using coro_handle = coroutine_handle<promise_type>;
    using base_class = std::conditional_t<std::is_same_v<R, void>, TaskVoid, TaskValue<R>>;
    
    coroutine_handle<> m_handle;
    Promise<R> *m_promise;
    bool m_suspend = false;

    Task()
    {
        MYLOG << "Task()" << endl;
    }

    Task(Task &other)
    {
        MYLOG << "Task(Task &other" << &other << ")" << this << endl;
        m_promise = other.m_promise;
        m_handle = other.m_handle;
        m_suspend = other.m_suspend;
    }

    Task(Task &&other)
    {
        MYLOG << "Task(Task &&other" << &other << ") " << this << endl;
        m_promise = (std::move(other.m_promise));
        m_handle = (std::move(other.m_handle));
        m_suspend = other.m_suspend;
    }

    Task(coro_handle handle)
    {
        MYLOG << "Task(coro_handle handle:" << handle.address() << ")" << this << endl;
        this->m_handle = handle;
        this->m_promise = &handle.promise();
    }

    Task(Promise<R> *promise, coroutine_handle<> handle, bool suspend)
    {
        MYLOG << "Task( promise:" << promise << " Handle:" << handle.address() << " Suspend:" << (suspend ? "true" : "false") << ")" << this << endl;
        m_promise = promise;
        m_handle = handle;
        m_suspend = suspend;
    }

    ~Task()
    {
        MYLOG << "~Task: " << this->m_handle.address() << endl << "####" << this << endl;
    }

    inline TaskAwait<R> operator co_await()
    {
        MYLOG << "    Task operator co_await() " << this->m_suspend << "   " << this->m_handle.address() << endl;
        // if(!TaskBase::m_handle.done())
        //     TaskBase::m_handle();
        //if(this->m_suspend)
            return TaskAwait<R>(this->m_promise, this->m_handle, this->m_suspend);
        //else
        //    return std::move(*this);
    }


    
    bool await_ready()
    {
        MYLOG << "Task::await_ready() " << (this->m_suspend ? "true" : "false") << endl;
        return true;//this->m_suspend;
    }
    bool await_suspend(coroutine_handle<> from)
    {
        MYLOG << "void Task::await_suspend(coroutine_handle<> " << from.address() << ") m_handle:" << this->m_handle.address() << " suspend:"<< (this->m_suspend ? "true" : "false") << endl;
        //if(from.address() == m_handle.address() && !m_suspend)
        //    from();
        /*if(from.address() == this->m_handle.address())
            return !this->m_suspend;
        from();
        this->m_handle();*/
        return false;
    }

    void await_resume()
    {
        MYLOG << "Task::await_resume() " << endl;
    }
};



















template <typename R>
struct promise_base
{
    void *ptr = nullptr;
    //promise_base(Task<R> &&t) : task(t){};
};

template <typename R>
struct type_promise_type : public promise_base<R>
{
    using promise_type = Promise<R>;
    using coro_handle = coroutine_handle<promise_type>;

    //type_promise_type(Task<R> &&task) : promise_base<R>(std::move(task)) {}

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


    void return_value(R &&r)
    {
        MYLOG << "return_value " << typeid(R).name() << " " << r << " " << &val_ << " " << endl;
        //val_ = std::move(r);

        if(this->ptr != nullptr)
            reinterpret_cast<TaskAwait<R>*>(this->ptr)->value_ = r;
        else
            throw std::runtime_error("type_promise_type error: Trying to access uninitialized pointer!");
    }

    void unhandled_exception() noexcept
    {
        cout << "!!!!!! unhandled exception !!!!!! :" << endl;

        if(this->ptr != nullptr)
            reinterpret_cast<TaskAwait<R>*>(this->ptr)->value_ = std::current_exception();
    }

    std::variant<std::monostate, std::exception_ptr, R> val_;
};

struct void_promise_type : public promise_base<void>
{
    using promise_type = Promise<void>;
    using coro_handle = coroutine_handle<promise_type>;

    //void_promise_type(Task<void> &&task) : promise_base<void>(std::move(task)){};

    void unhandled_exception() noexcept
    {
        cout << "!!!!!! unhandled exception(void) !!!!!!" << endl;
        
        if(this->ptr != nullptr)
            reinterpret_cast<TaskAwait<void>*>(this->ptr)->value_ = std::current_exception();
    }

    void return_void() { MYLOG << "return_void" << endl; }

    std::variant<std::monostate, std::exception_ptr> val_;
};

template <typename R>
struct Promise : std::conditional_t<std::is_same_v<R, void>, void_promise_type, type_promise_type<R>>
{
    using promise_type = Promise<R>;
    using coro_handle = coroutine_handle<promise_type>;
    using base_class = std::conditional_t<std::is_same_v<R, void>, void_promise_type, type_promise_type<R>>;

    coro_handle get_return_object()
    {
        return std::move(coro_handle::from_promise(*this));
    }

    Promise() //: base_class(std::move(Task<R>(this, get_return_object(), false)))
    {
        MYLOG << "Promise()" << endl;
    }
    Promise(Promise &other) : base_class(other)
    {
        MYLOG << "Promise &other" << endl;
    }
    Promise(Promise &&other) : base_class(other)
    {
        MYLOG << "Promise &&other" << endl;
    }


    ~Promise()
    {
        MYLOG << "~Promise()" << endl;
    }

    auto initial_suspend() noexcept
    {
        MYLOG << "initial_suspend()" << get_return_object().address() << endl;
        return suspend_always();
    }

    auto final_suspend() noexcept
    {
        MYLOG << "final_suspend()" << get_return_object().address() << endl;
        return suspend_never();
    }
};




template<typename R>
R TaskValue<R>::await_resume()
{
    MYLOG << "TaskValue::await_resume()" << this << endl;
    //m_handle();
    //auto val = this->m_promise->val_;

    //this->m_promise->get_return_object()();

    if(std::holds_alternative<R>(this->value_))
        return std::get<R>(this->value_);
    else if(std::holds_alternative<std::exception_ptr>(this->value_))
        throw std::get<std::exception_ptr>(this->value_);
    throw std::runtime_error("Promise did not returned value!");
}



void TaskVoid::await_resume()
{
    MYLOG << "TaskVoid::await_resume()" << this << endl;
    //auto val = this->m_promise->val_;

    //this->m_promise->get_return_object()();

    if(std::holds_alternative<std::exception_ptr>(this->value_))
        throw std::get<std::exception_ptr>(this->value_);
}

