#include <iostream>
#include <string>

template <typename T>
void fun(T &&t)
{
    int b = 20;
    T &a = b;
}

int main()
{
    int a = 10;
    // auto &&b = a;

    fun(a);
}