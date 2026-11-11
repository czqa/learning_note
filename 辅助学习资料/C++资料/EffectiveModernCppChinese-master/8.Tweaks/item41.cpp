#include <iostream>
#include <vector>

class Test
{
public:
    Test(int data1)
    {
        // std::cout << "Test(int data1)" << std::endl;
    }
    Test(const Test &rhs) { std::cout << "Test(const Test &rhs)" << std::endl; }
    Test(Test &&rhs) { std::cout << "Test(Test &&rhs)" << std::endl; }
    // ~Test() { std::cout << "~Test()" << std::endl; }
};

// 方法1：对左值和右值重载
class Widget1
{
public:
    Widget1()
    {
        _tests.reserve(10);
    }
    void addTest(const Test &newTest)

    {
        _tests.push_back(newTest);

    } // rvalues
    void addTest(Test &&newTest)

    {
        _tests.push_back(std::move(newTest));
    }

private:
    std::vector<Test> _tests;
};

// 方法2：使用通用引用
class Widget2
{
public:
    Widget2()
    {
        _tests.reserve(10);
    }
    template <typename T>
    void addTest(T &&newTest)

    {
        _tests.push_back(std::forward<T>(newTest));
    }

private:
    std::vector<Test> _tests;
};

// 方法3：传值
class Widget3
{
public:
    Widget3()
    {
        _tests.reserve(10);
    }
    void addTest(Test newTest)
    {
        _tests.push_back(std::move(newTest));
    }

private:
    std::vector<Test> _tests;
};

// 不可拷贝的对象
class Test1
{

public:
    Test1(int data1)
    {
        // std::cout << "Test1(int data1)" << std::endl;
    }
    Test1(const Test1 &rhs) = delete;
    Test1(Test1 &&rhs) { std::cout << "Test1(Test1 &&rhs)" << std::endl; }

    Test1 &operator=(const Test1 &rhs) = delete;
    Test1 &operator=(Test1 &&rhs)
    {
        std::cout << "Test1 &operator=(Test1 &&rhs)" << std::endl;
        return *this;
    }
    // ~Test1() { std::cout << "~Test1()" << std::endl; }
};

class Widget4
{
public:
    // void setTest1(Test1 &&t)
    // {
    //     _t1 = std::move(t);
    // }

    void setTest1(Test1 t)
    {
        _t1 = std::move(t);
    }

private:
    Test1 _t1{10};
};

// 通过赋值的形式进行拷贝
class Test2
{
public:
    Test2(int data1)
    {
        // std::cout << "Test2(int data1)" << std::endl;
    }
    Test2(const Test2 &rhs) { std::cout << "Test2(const Test2 &rhs)" << std::endl; }
    Test2(Test2 &&rhs) { std::cout << "Test2(Test2 &&rhs)" << std::endl; }
    Test2 &operator=(const Test2 &rhs)
    {
        std::cout << "Test2 &operator=(const Test2 &rhs)" << std::endl;
        return *this;
    }
    Test2 &operator=(Test2 &&rhs)
    {
        std::cout << "Test2 &operator=(Test2 &&rhs)" << std::endl;
        return *this;
    }
    // ~Test2() { std::cout << "~Test2()" << std::endl; }
};

class Password
{
public:
    explicit Password(std::string pwd) // 传值
        : text(std::move(pwd))
    {
    }                                 // 构造text
    void changeTo(std::string newPwd) // 传值
    {
        text = std::move(newPwd);
    } // 赋值text

private:
    std::string text; // 密码的text
};

int main()
{
    // Widget3 w1;
    // Test t1(10);
    // std::cout << "========" << std::endl;
    // w1.addTest(t1);
    // std::cout << "========" << std::endl;
    // w1.addTest(Test(20));
    // //  void addTest(Test newTest)  Test newTest = Test(20);
    // std::cout << "==========================" << std::endl;
    // // Test aa = Test(20);

    Widget4 w4;
    w4.setTest1(Test1(10));

    // std::cout << "================================" << std::endl;

    // std::string initPwd("Supercalifragilisticexpialidocious");
    // Password p(initPwd);
    // std::string newPassword = "Beware the Jabberwock";
    // p.changeTo(newPassword);

    return 0;
}