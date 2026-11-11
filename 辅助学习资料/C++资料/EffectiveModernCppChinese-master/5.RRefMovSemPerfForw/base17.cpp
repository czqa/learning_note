#include <iostream>

class Test
{
public:
    Test(int data) : _data(data)
    {
        _ptr = new int(50);
        std::cout << "Test()" << std::endl;
    }
    Test(const Test &rhs) : _data(rhs._data) { std::cout << "Test(const Test &rhs)" << std::endl; }
    Test(Test &&rhs) : _data(rhs._data) { std::cout << "Test(Test &&rhs)" << std::endl; }
    Test &operator=(const Test &rhs)
    {
        _data = rhs._data;
        std::cout << "Test &operator=(const Test &rhs)" << std::endl;
        return *this;
    }
    Test &operator=(Test &&rhs)
    {
        _data = rhs._data;
        _ptr = rhs._ptr;
        rhs._ptr = nullptr;
        std::cout << "Test &operator=(Test &&rhs)" << std::endl;
        return *this;
    }
    ~Test()
    {
        // std::cout << "~Test()" << std::endl;
        delete _ptr;
    }

    void setData(int data) { _data = data; }

private:
    int _data{10};
    int *_ptr = nullptr;
};

Test GetTest1()
{
    Test a(1);
    a.setData(2);
    return a;
}

void GetTest1(Test &_test)
{
    // Test a(1);
    new (&_test) Test(1);
    _test.setData(2);
}

// Test temp = a;
Test GetTest2()
{
    return Test(2);
}

void GetTest2(Test &_test)
{
    new (&_test) Test(2);
}

Test optiFailed1(bool flag)
{
    Test a{1}, b{2};
    a.setData(20);
    b.setData(50);
    if (flag)
        return a;
    return b;
}

// URVO  -NRVO
void optiFailed1(Test &_test, bool flag)
{
    Test a{1}, b{2};
    if (flag)
    {
        new (&_test) Test(std::move(a));
        // new (&_test) Test(1);
        _test.setData(20);
        b.setData(50);
    }

    else
    {
        new (&_test) Test(std::move(b));
        // new (&_test) Test(2);
        a.setData(20);
        _test.setData(50);
    }
}

/*
    Test optiFailed1(Test &_test)
    {
        Test a{1}, b{2};
        if (true)
            return _test.Test::Test(b);
        else
            return _test.Test::Test(a);
    }
    Test result;(不构造)  optiFailed1(result);
*/

Test optiSuccess()
{
    Test a{1};
    if (true)
        return a;
    a.setData(50);
    return a;
}

// Test ttt(1);
// Test optiFailed2()
// {
//     return ttt;
// }
// Test temp = ttt;  copy
// URVO -NRVO
// void optiFailed2(Test &_test)
// {
//     new (&_test) Test(ttt);
// }

Test optiFailed3(Test &t)
{
    return t;
}

struct Wrapper
{
    Test t{5};
};

Test optiFailed4()
{
    return Wrapper().t;
}

Test optiFailed5()
{
    Test t{10};
    return std::move(t);
}

// URVO NRVO
void optiFailed5(Test &_test)
{
    Test t{10};
    new (&_test) Test(std::move(t));
}

Test optiFailed6()
{
    return Test{10};
}
// URVO -NRVO
void optiFailed6(Test &t)
{
    // new (&t) Test(10);
    t = Test{10};
}

int main()
{
    // Test result = GetTest2();

    // char buff[sizeof(Test)];
    // Test *ptr = reinterpret_cast<Test *>(buff);
    // optiFailed1(*ptr, true);
    // optiFailed2(*ptr);
    // Test aaa(10);
    // aaa = optiFailed6();
    // std::cout << "=======" << std::endl;
    // Test cc = optiFailed6();

    // Test bbb(10);
    // optiFailed6(bbb);
    // std::cout
    //     << "========" << std::endl;
    // Test result1 = GetTest1();
    // Test t = Test(3);
    // auto t = GetTest1();
    // std::cout << "----------------" << std::endl;
    // auto t2 = GetTest2();
    // auto t = optiFailed1(true);
    // auto t = optiSuccess();
    // auto t = GetTest2();
    // auto t = optiFailed2();
    // Test tt(1);
    auto result = optiFailed5();
    // Test t{10};
    // t = GetTest2();
    // auto t = optiFailed5();
    return 0;
}