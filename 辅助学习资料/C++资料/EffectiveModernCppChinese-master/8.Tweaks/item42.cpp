#include <iostream>
#include <vector>

class Test
{
public:
    Test(int data1)
    {
        std::cout << "Test(int data1)" << std::endl;
    }
    Test(const Test &rhs) { std::cout << "Test(const Test &rhs)" << std::endl; }
    Test(Test &&rhs) { std::cout << "Test(Test &&rhs)" << std::endl; }
    Test &operator=(const Test &rhs)
    {
        std::cout << "Test &operator=(const Test &rhs)" << std::endl;
        return *this;
    }
    Test &operator=(Test &&rhs)
    {
        std::cout << "Test &operator=(Test &&rhs)" << std::endl;
        return *this;
    }
    ~Test() { std::cout << "~Test()" << std::endl; }
};

class Test1
{
public:
    explicit Test1(int data) {}
};

int main()
{
    std::vector<Test> tVec;
    tVec.reserve(10);
    std::cout << "===========" << std::endl;
    tVec.push_back(Test(10));
    std::cout << "===========" << std::endl;
    tVec.emplace_back(Test(10));
    std::cout << "======***=====" << std::endl;
    // tVec.insert(tVec.begin(), 10);
    // std::cout << "======***=====" << std::endl;
    // tVec.emplace(tVec.begin(), 10);
    // std::cout << "=======***====" << std::endl;
    std::vector<Test1> tVec1;
    // tVec1.push_back(10);
    tVec1.emplace_back(10);
}