#include <iostream>
#include <string>
#include <memory>

int a = 0;
class SizeComp
{
public:
    SizeComp(size_t n) : sz(n) {}
    bool operator()(const std::string &s) const { return s.size() >= sz; }

private:
    size_t sz;
};

class A
{
public:
    void print() { std::cout << "Class A" << std::endl; }
    void test()
    {
        auto foo = [this]
        {
            print();
            x = 5;
        };
        foo();
    }

private:
    int x;
};

int main()
{
    size_t sz = 10, ss = 50;

    auto SizeCompLambda = [=](const std::string &a)
    {
        return a.size() >= sz;
    };
    auto size = sizeof(SizeCompLambda);

    int b = 0;
    static int c = 0;

    // auto fun1 = [a, b, c] {};
    auto fun1 = [b] // a c不需要被捕获 可以直接使用
    {
        std::cout << "b is : " << b << std::endl;
        a = 10;
        c = 12;
    };

    auto func_test = [&sz]()
    { std::cout << "sz is : " << sz << std::endl; };

    sz = 50;
    func_test();

    auto fun2 = [](int mm)
    {
        mm = 20;
        return mm;
    };
    auto ssize = sizeof(fun2);
    auto important = std::make_unique<int>(1);
    auto add = [v1 = 1, v2 = std::move(important)](int x, int y) -> int
    {
        return x + y + v1 + (*v2);
    };

    auto fun3 = [b = b + 2] // a c无法被捕获 但是可以直接使用
    {
        std::cout << "b is : " << b << std::endl;
    };

    auto fun4 = [](int i)
    {if(i>0) return i; 
    else return -i; };

    // auto x2 = []()
    // { return {1, 2}; };

    auto mm = fun4(-10);

    return 0;
}
