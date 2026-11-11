#include <iostream>
#include <string>
#include <vector>

class Test
{

public:
    Test(int data1, int data2) { std::cout << "Test(int data1, int data2)" << std::endl; }
    Test(const Test &rhs) { std::cout << "Test(const Test &rhs)" << std::endl; }
    Test(Test &&rhs) { std::cout << "Test(Test &&rhs)" << std::endl; }
    // ~Test() { std::cout << "~Test()" << std::endl; }
};

int main()
{
    std::vector<Test> tests;
    tests.reserve(6);
    // Test tt(10, 20);
    std::cout << "=============" << std::endl;
    // tests.emplace_back(tt);
    // tests.emplace_back(Test(20, 50));
    tests.emplace_back(50, 60);
    std::cout << "=============" << std::endl;
    // tests.push_back(tt);
    // tests.push_back(Test(20, 50));
    tests.push_back({50, 60});

    std::vector<std::string> strs;
    strs.push_back("a");
    strs.push_back("aa");
    strs.push_back("aaa");

    strs.emplace_back("b");
    strs.emplace_back("bb");
    strs.emplace_back("bbb");
    return 0;
}