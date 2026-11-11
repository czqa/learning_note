#include <iostream>
#include <memory>

int func(int a)
{
    int b = 10;
    a = b;
    return a;
}

void func2(int *a)
{
    *a = 10;
}

void func3(int *a)
{
    a = new int(10);
}

void fun4(int **a)
{
    *a = new int(10);
}

int main()
{
    int c = 30;
    // int d = func(c);
    // func2(&c);
    // func3(&c);

    int *d = nullptr;
    fun4(&d);
    return 0;
}