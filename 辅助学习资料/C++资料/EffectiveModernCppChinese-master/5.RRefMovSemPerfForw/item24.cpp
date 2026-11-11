#include <vector>
#include <iostream>

template <typename T>
void func(T &&param)
{
    // param是左值     T -> int&     int& && -> int&
    std::cout << "T&&" << std::endl;
}

template <typename T>
void func2(const T &&param)
{
    std::cout << "const T&&" << std::endl;
}

int main()
{
    int a = 10;
    func(10);
    func(a);
    func<int>(10);

    // func2(a);
    std::vector<int> vec{1, 2, 3};
    vec.push_back(4);

    return 0;
}