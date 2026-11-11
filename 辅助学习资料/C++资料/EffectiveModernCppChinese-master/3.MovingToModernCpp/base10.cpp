#include <iostream>
#include <string>
#include <vector>

struct A
{
    A(int param) : a(param), b(50.f)
    {
    }

    int a = 10;
    float b = 20.f;
    std::string c{"hhh"};
};

struct B
{
    B() { std::cout << "B()" << std::endl; }
    B(int data) : data1(data) { std::cout << "B(int data)" << std::endl; }
    virtual void vfunc() { std::cout << "B::vfunc" << std::endl; }
    int data1;
};

struct C
{
    C(int data) : data2(data) { std::cout << "C(int data)" << std::endl; }

    B b;
    int data2;
};

struct BB : public B
{
    BB(int param) : data3(param) { std::cout << "BB(int param)" << std::endl; }
    int data3;
};

struct D
{
    D(int param) : data4(param) {}
    int data4;
};

struct E : public D
{
    E(int param, int d) : D(d), data5(param) {}

    int data5;
};

struct F
{
    F() = default;
    F(int a) : data6(a) {}
    F(int a, int b) : data6(a), data7(b) {}
    F(int a, int b, int c) : data6(a), data7(b), data8(c) {}
    F(int a, int b, int c, int d) : data6(a), data7(b), data8(c), data9(d) {}
    void func() { std::cout << "F::func" << std::endl; }
    int data6;
    int data7;
    int data8;
    int data9;
};

struct G : public F
{
    using F::F;
    void func() { std::cout << "G::func" << std::endl; }
};

int main()
{
    // A a(1);
    // B b;
    C c(5);
    BB bb(10);
    F f(5);
    G g(5, 0);
    return 0;
}