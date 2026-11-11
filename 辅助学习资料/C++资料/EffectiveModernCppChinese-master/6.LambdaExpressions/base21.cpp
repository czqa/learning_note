#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include <functional>
#include <vector>

struct Test
{
    Test() {}
    Test(int data) : _data(data)
    {
        std::cout << "Test()" << std::endl;
    }
    Test(const Test &rhs) : _data(rhs._data) { std::cout << "Test(const Test &rhs)" << std::endl; }
    Test(Test &&rhs) : _data(rhs._data) { std::cout << "Test(Test &&rhs)" << std::endl; }
    int func(int a, int b)
    {
        std::cout << "a is: " << a << std::endl;
        std::cout << "b is: " << b << std::endl;
        return a - b;
    }

private:
    int _data{10};
};

int add(int a, int b)
{
    std::cout << "a is: " << a << std::endl;
    std::cout << "b is: " << b << std::endl;
    return a + b;
}

void fun(int &a, int &b)
{
    a = 100;
    b = 500;
}

void fun1(Test &a)
{
}

void fun2(Test a)
{
}

void fun3(Test &&a)
{
}

int main()
{
    // int a = 5;
    // auto add5 = std::bind(add, std::placeholders::_2, std::placeholders::_1);
    // int result = add5(10, 5);

    // int b = 10;
    // Widget w;
    // auto add10 = std::bind(&Widget::add, &w, b, std::placeholders::_1);

    // int m = 10;
    // int n = 20;
    // auto funBind = std::bind(fun, m, std::placeholders::_1);
    // funBind(n);

    // auto addFunc = my_bind(add, 2);
    // std::cout << addFunc(3) << std::endl; // 输出 5

    // // =================
    Test t(50);
    auto funBind1 = std::bind(fun1, std::ref(t));
    auto funBind2 = std::bind(fun2, t);
    auto funBind3 = std::bind(fun3, t);

    Test t1(50), t2(50), t3(50);
    auto funBind4 = std::bind(fun1, std::move(t1));
    auto funBind5 = std::bind(fun2, std::move(t2));
    auto funBind6 = std::bind(fun3, std::move(t3));

    return 0;
}