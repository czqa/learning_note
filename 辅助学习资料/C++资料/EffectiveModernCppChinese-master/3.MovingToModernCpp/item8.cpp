#include <iostream>
#include <memory>

void f(int)
{
    std::cout << "f(int)" << std::endl;
}
void f(bool)
{
    std::cout << "f(bool)" << std::endl;
}
void f(void *)
{
    std::cout << "f(void *)" << std::endl;
}
void f(long)
{
}

int f1(std::shared_ptr<int> spi)
{
    return 5;
}

double f2(std::unique_ptr<int> upi)
{
    return 5.;
}

bool f3(int *pi)
{
    return true;
}

template <typename FuncType, typename PtrType>
decltype(auto) func(FuncType f, // C++14
                    PtrType ptr)
{
    return f(ptr);
}
// bool func_(bool (*f)(int *), // C++14
//            int ptr)
// {
//     return f(ptr);
// }

int main()
{
    int *aa = 0;
    int *bb = NULL;
    int *cc = nullptr;

    auto a = 0;
    // auto b = NULL;
    auto c = nullptr;

    f(0);
    // f(NULL); // 编译不过
    f(nullptr);

    // auto result1 = func(f3, 0); // 错误！
    // f3(0);
    // auto result2 = func(f2, NULL);    // 错误！
    auto result3 = func(f3, nullptr); // 没问题

    int aaa = 0;
    // int *m = aaa;
    // std::shared_ptr<int> spi = aaa;
    return 0;
}
