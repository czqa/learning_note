#include <iostream>
#include <vector>
#include <string>
void fun(int a[])
{
}

void fun2(int (*ptr2)[5])
{
}

bool fun3(int a, int b)
{
    return true;
}

typedef bool (*PF)(int, int);
typedef bool P(int, int);
void fun4(int c, bool (*fun3Ptr)(int, int)) {}
void fun5(int c, bool fun3Ptr(int, int)) {}
void fun6(int c, PF fun3Ptr) {}
void fun7(int c, P fun3) {}
void fun8(int c, P *fun3Ptr) {}

typedef int int_name;
int_name ooo = 10;
using int_name = int;

// typedef bool (*fun3Ptr)(int, int); // fun3Ptr ==  bool (*)(int, int)
// typedef bool fun(int, int);        // fun ==  bool (int, int)
using fun3Ptr = bool (*)(int, int);
using fun3xxx = bool(int, int);
// fun3xxx == fun3Ptr*

void func2(int c, fun3xxx fun)
{
    fun(5, 8);
}
// bool(int, int) fun6(int c) { return nullptr; }  //报错
// bool (*fun9(int c))(int, int) { return nullptr; }
fun3Ptr fun3(int c);

// using PF = bool (*)(int, int);
// typedef bool (*PF)(int, int);
// PF fun9(int c);

int main()
{
    int a = 10;
    int *const(&b) = &a;

    int array[5] = {0, 0, 0, 0, 0};

    std::vector<int> aaaaaa;
    int *ptr = array; // 将数组名直接赋值给一个指针会发生【退化】
    // int(*ptr2)[5] = array;
    // int *ptr3[5] = {&a, &a, &a, &a, &a};
    int(*ptr3)[5] = &array; // int (*) [5]  但是对数 组名取地址那么就没有退化这一说了 c++ primer P147
    // int(&ref)[5] = array;   // int (&) [5]

    &"hello world"; // const char [12]
    char str[12] = "hello world";
    // char str1[12] = "hello world";
    const char *strrrr = "hello world";

    // &"hello world";
    // char *strPtr = &"hello world";
    // const char *strPtr = &"hello world";
    // const char *strPtr[12] = &"hello world";
    const char(*strPtr)[12] = &"hello world"; // strPtr 的数据类型是const char(*)[12]
    const char *strPtrArray[2] = {str, str};  // strPtrArray 的数据类型是const char*[2]

    const char(&strRef)[12] = "hello world"; // strPtr 的数据类型是const char(&)[12]

    // const char &strPtrArray[2] = {str, str}; //引用的数组是不被允许滴

    // 函数部分
    bool (*fun3Ptr)(int, int) = fun3; // 函数退化成函数指针
    fun3Ptr = fun3;                   // 函数退化成函数指针
    fun3Ptr = &fun3;
    bool xx = fun3Ptr(1, 2);
    bool yy = (*fun3Ptr)(1, 2);

    bool (&funRef)(int, int) = fun3;
    return 0;
}
