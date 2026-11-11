#include <iostream>
#include <vector>

struct Point
{
    int x, y;
    Point &operator=(Point p)
    {
        std::cout << "operator=" << std::endl;
        return *this;
    }
};

int fun(int x)
{
    std::cout << "fun" << std::endl;
    return x;
}

void func(int, int) {}

template <typename Container, typename Index> // 可以工作，
auto testFun(Container &c, Index i)           // 但是需要改良
    -> decltype(c[i])                         // c++11的写法 尾置返回类型语法
{
    // do someing...
    return c[i];
}

template <typename Container, typename Index> // C++14版本，
auto testFun_error(Container &c, Index i)     // 不那么正确 因为auto作为函数返回值的时候走的是模板推导的套路
{
    // do someing...
    return c[i]; // 从c[i]中推导返回类型
}

template <typename Container, typename Index>       // C++14版本，
decltype(auto) testFun_right(Container &c, Index i) // 可以工作，但是还需要改良
{
    // do someing...
    return c[i];
}

template <typename Container, typename Index>
decltype(auto) testFun_better(Container &&c, Index i)
{
    // do someing...
    // return c[i];
    return std::forward<Container>(c)[i];
}

std::vector<int> getIntVector()
{
    return {1, 2, 3};
}

int main()
{
    int a = 10;
    int *aptr = &a;

    int &aref = a;
    int &&arref = std::move(a);
    // decltype(arref) ref__;
    const int ca = 20;
    const int *captr = &ca;

    const int &caref = ca;
    const int &&carref = std::move(ca);

    int *const acptr = &a;
    const int *const cacptr = &a;
    10;

    int array[2] = {0, 1};
    "hello world";
    int(*arrayptr)[2] = &array;
    int(&arrayref)[2] = array;

    func;
    void (*funcptr)(int, int) = func;
    void (&funcref)(int, int) = func;

    // https: // blog.csdn.net/u014609638/article/details/106987131?spm=1001.2101.3001.6661.1&utm_medium=distribute.pc_relevant_t0.none-task-blog-2~default~CTRLIST~Rate-1-106987131-blog-123871931.pc_relevant_multi_platform_whitelistv4&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-2~default~CTRLIST~Rate-1-106987131-blog-123871931.pc_relevant_multi_platform_whitelistv4&utm_relevant_index=1
    int value = 10;
    int *ptr = &value;
    decltype(*ptr) ref = value;
    decltype((value)) ref2 = value;
    decltype((((value)))) ref3 = value;
    decltype(value + 10) value2 = value;

    const int cvalue = 20;
    const int *cptr = &cvalue;
    int *const ptrc = &value;
    decltype(*cptr) cref = cvalue;
    decltype(*ptrc) refc = value;
    decltype((cvalue + 10)) value3 = value;

    Point point{1, 2};
    decltype(point) point2{5, 8};
    decltype(fun(value)) value4;
    // decltype((fun(value))) value5;

    std::vector<bool> vec = {true, false, true};
    decltype(vec[1]) value6;
    std::vector<int> vec2 = {1, 2, 3};
    decltype(vec2[1]) value7 = value;
    std::vector<Point> vec3 = {{1, 2}, {3, 4}, {2, 3}};
    decltype(vec3[1]) value8 = point;

    // testFun(vec, 1) = true;
    // testFun(vec2, 1) = 10;
    // testFun(vec3, 1) = {4, 8};

    // testFun_error(vec, 1) = true;
    // testFun_error(vec2, 1) = 10;
    // testFun_error(vec3, 1) = point;

    // testFun_right(vec, 1) = true;
    // testFun_right(vec2, 1) = 20;
    // testFun_right(vec3, 1) = {5, 15};
    decltype(auto) aa = testFun_right(vec2, 1);

    // auto mm = testFun_right(getIntVector(), 1);
    decltype(auto) nn = testFun_better(getIntVector(), 1);
    decltype(auto) mm = testFun_better(vec2, 1);
    nn = 1587;
    // testFun_better(getIntVector(), 1) = 152;
    std::cout << "over" << std::endl;
}