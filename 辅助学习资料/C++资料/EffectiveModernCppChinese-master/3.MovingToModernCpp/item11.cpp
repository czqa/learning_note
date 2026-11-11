#include <iostream>
#include <string>
#include <vector>

struct B;

struct A
{
    A(int data) : _data(_data){};
    A(const A &a) = delete;
    A &operator=(const A &) = delete;

private:
    friend B;
    int _data;
};

struct B
{
    B()
    {
        A a(10);
        // A b = a;
    }
};

bool isLucky(int a)
{
    return a == 7;
}

bool isLucky(char) = delete;

template <typename T>
void processPointer(T *ptr)
{
}

template <>
void processPointer<void>(void *ptr) = delete;

int main()
{
    A a(10);
    // A aaa = a;

    B b;

    isLucky(32);
    // isLucky('a');

    int aa = 10;
    void *aaptr = reinterpret_cast<void *>(&aa);
    // processPointer(aaptr);
    return 0;
}