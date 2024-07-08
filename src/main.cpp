#include <coroutine>
#include <iostream>
#include <thread>
#include <memory>
#include <vector>
#include <queue>
#include <optional>

#include "coro2.hpp"

using namespace std;

Task<int> my_cor(int a)
{
    std::cout << "   my_cor" << endl;
    co_return 123;
}

Task<void> void_cor(int a)
{
    std::cout << "   void_cor" << a << endl;
    co_return;
}

Task<void> big_cor(int a)
{
    int b[1000000000];
    b[99999] = 123;
    b[57273] = 322;
    std::cout << "   void_cor" << a << b[99999] << endl;
    co_return;
}

Task<int> my_par_cor(int a, int b)
{
    std::cout << "   my_par_cor" << endl;
    // int k = a / 0;
    co_return a + b;
}

Task<int> my_nonwait_cor(int a, int b)
{
    std::cout << "   my_nonwait_cor" << endl;
    co_return a + b; // myStruct(a.value + b.value);
}

Task<void> final_coro(int a, int b)
{
    std::cout << "   final_coro" << a << " " << b << endl;
    co_return;
}

Task<void> call_two()
{
    co_await void_cor(1);
    co_await void_cor(2);
}

Task<void> call_five()
{
    co_await call_two();
    co_await call_two();
    co_await final_coro(500000, 90000000);
}

struct InitialTask
{
    struct promise_type
    {
        InitialTask get_return_object() { return {}; }
        auto initial_suspend() noexcept { return suspend_never(); }
        auto final_suspend() noexcept { return suspend_never(); }
        void return_void() {}
        void unhandled_exception() {}
    };
};

Task<void> secondCor()
{
    try
    {
        int i = 0;
        std::cout << "   secondCor start_point" << ++i << " " << this_thread::get_id() << endl;
        auto k = my_nonwait_cor(200, 31);
        int c = co_await my_cor(999);
        std::cout << "   secondCor " << ++i << " _my_cor result:" << " " << c << endl;
        std::cout << "   secondCor " << ++i << " _my_par_cor result:" << co_await my_par_cor(300, 21) << endl;
        std::cout << "   secondCor " << ++i << " _co_await_k " << co_await k << " " << this_thread::get_id() << endl;
        std::cout << "   secondCor " << ++i << " " << endl;
        co_await void_cor(1);
        std::cout << "   secondCor big cor " << ++i << " " << endl;
        big_cor(5);
        std::cout << "   secondCor " << ++i << " " << endl;
        co_await void_cor(2);
        std::cout << "   secondCor " << ++i << " " << endl;
        co_await call_five();
        std::cout << "   secondCor DONE" << endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    co_return;
}

int main()
{
    std::cout << "       echo 123 " << this_thread::get_id() << endl;

    try
    {
        for(int i = 0; i < 1; ++i)
            auto k = secondCor().operator co_await().await_ready();
    }
    catch (exception e)
    {
        std::cout << e.what();
    }
    std::cout << "       End" << endl;
    for (int i = 0; i < 1000000000; ++i)
    {
    }
    while(true){}
    return 0;
}