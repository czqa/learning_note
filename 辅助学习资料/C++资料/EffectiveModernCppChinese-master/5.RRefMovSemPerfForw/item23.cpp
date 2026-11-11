#include <iostream>
#include <memory>
#include <type_traits>
namespace
{
    template <typename T>
    typename std::remove_reference<T>::type &&move(T &&param) // c++11
    {
        using ReturnType = typename std::remove_reference<T>::type &&;
        return static_cast<ReturnType>(param);
    }

    template <typename T>
    decltype(auto) move2(T &&param) // C++14
    {
        using ReturnType = std::remove_reference_t<T> &&;
        return static_cast<ReturnType>(param);
    }
} // namespace

struct A
{
    A(int value) : b(value) { std::cout << "create" << std::endl; }
    A(const A &value)
    {
        std::cout << "copy" << std::endl;
        b = value.b;
    }
    A(A &&value)
    {
        std::cout << "move" << std::endl;
        b = value.b;
        value.b = 0;
    }

    int b;
};

class Annotation
{
public:
    explicit Annotation(const A a) : a_param(std::move(a)) {}

private:
    A a_param;
};

void process(const A &lvalArg)
{
    std::cout << "deal lvalArg" << std::endl;
}
void process(A &&rvalArg)
{
    std::cout << "deal rvalArg" << std::endl;
}

template <typename T>
void logAndProcess(T &&param)
{
    // process(param);
    // process(std::move(param));
    process(std::forward<T>(param)); // 实参用右值初始化时，转换为一个右值
}

template <typename T>
typename std::remove_reference<T>::type &&mymove(T &&param)
{
    using ReturnType = typename std::remove_reference<T>::type &&;
    return static_cast<ReturnType>(param);
}

int main()
{
    // int mmm = 10;
    // int &&nnn = mymove(10);
    // Annotation aa{10};

    // const A cc{12};
    // const A &mm = std::move(cc);

    A mm{10};
    logAndProcess(mm);
    logAndProcess(std::move(mm));
}