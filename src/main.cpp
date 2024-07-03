#include <coroutine>
#include <iostream>
#include <thread>
#include <memory>
#include <vector>
#include <queue>
#include <optional>

#include "coro.hpp"

using namespace std;


Task<int> my_cor()
{
    cout << "my_cor" << this_thread::get_id() << endl;
    co_return 123;
}

Task<void> void_cor(int a)
{
    cout << "void_cor" << a << endl;
    co_return;
}

Task<int> my_par_cor(int a, int b)
{
    cout << "my_par_cor" << endl;
    co_return a + b;
}

Task<int> my_nonwait_cor(int a, int b)
{
    cout << "my_nonwait_cor" << endl;
    co_return a + b;
}

Task<void> call_two()
{
    co_await void_cor(1);
    co_await void_cor(2);
}

Task<void> call_four()
{
    co_await call_two();
    co_await call_two();
}

Task<void> call_more()
{
    co_await call_four();
    co_await call_four();
    co_await call_four();
    co_await call_four();
    co_await call_four();
    co_await call_four();
}

Task<void> secondCor()
{
    int i = 0;
    cout << "secondCor " << ++i << " " << this_thread::get_id() << endl;
    auto k = my_nonwait_cor(200,31);
    cout << "secondCor " << ++i << " " << endl;
    auto c = co_await my_cor();
    cout << "secondCor " << ++i << " " << typeid(c).name() << " " << c << endl;
    cout << "secondCor " << ++i << " " << co_await my_par_cor(300, 21) << endl;
    cout << "secondCor " << ++i << " " << endl;
    co_await void_cor(1);
    cout << "secondCor " << ++i << " " << endl;
    co_await void_cor(2);
    cout << "secondCor " << ++i << " " << endl;
    co_await call_more();
    cout << "secondCor " << ++i << " " << endl;
    cout << "secondCor DONE" << endl;
    co_return;
}

int main()
{
    cout << "echo 123 " << this_thread::get_id() << endl;

    auto s = secondCor();

    cout << "End" << endl;
    while (true)
    {
        usleep(10);
    };
    return 0;
}