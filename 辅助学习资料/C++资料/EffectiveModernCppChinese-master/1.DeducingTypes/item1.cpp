#include <iostream>
// void fun(int a) {}
// void fun(const int a) {}

template <typename T>
void f(T param)
{
    std::cout << param << std::endl;
}
void func(int, int) {}
int func2(int) { return 10; }
// int (*func2ptr)(int) = func2;

using F = int(int);
// typedef int F(int);

int main()
{
    // const F *aaa = func2;
    // F *const bbb = func2;
    // int (*const func2p)(int) = func2;
    // int (&func2r)(int) = func2;
    // const F &ccc = func2;

    int a = 10;
    int *aptr = &a;
    int &aref = a;
    int &&arref = std::move(a);

    const int ca = 20;
    const int *captr = &ca;
    const int &caref = ca;
    const int &&carref = std::move(ca);
    int *const acptr = &a;
    const int *const cacptr = &a;
    10;

    char mmm = 'a';
    f(&mmm);
    int array[2] = {0, 1};
    "hello world";
    int(*arrayptr)[2] = &array;
    int(&arrayref)[2] = array;

    func;
    void (*funcptr)(int, int) = func;
    void (&funcref)(int, int) = func;

    return 0;
}
