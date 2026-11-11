#include <iostream>

template <typename T>
void f(T param)
{
}

template <typename T>
void g(std::initializer_list<T> param)
{
}

// auto createInitList()
// {
//     return {1, 2, 3};
// }

int main()
{
    auto x1 = 27;
    auto x2(27);
    auto x3{27};    // int
    auto x4 = {27}; // initializer_list<int>

    // auto x5{10, 27};
    // auto x6 = {10, 27, 7};
    // f({1, 2, 3});
    g({1, 2, 3});
    return 0;
}
