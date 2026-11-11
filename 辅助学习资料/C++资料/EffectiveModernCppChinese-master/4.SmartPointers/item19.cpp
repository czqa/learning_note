#include <memory>
#include <vector>
#include <iostream>

class Test
{
public:
    Test()
    {
        std::cout << "Test()" << std::endl;
    }
    Test(int data) : _data(data)
    {
        std::cout << "Test(int data)" << std::endl;
    }
    Test(int data, int data2) : _data(data), _data2(data2)
    {
        std::cout << "Test(int data, int data2)" << std::endl;
    }
    Test(const Test &t)
    {
        std::cout << "Test(const Test& t)" << std::endl;
        _data = t._data;
        _data2 = t._data2;
    }
    Test(Test &&t) noexcept
    {
        std::cout << "Test(Test &&t)" << std::endl;
        _data = t._data;
        _data2 = t._data2;
    }
    ~Test() { std::cout << "~Test()" << std::endl; }
    int _data{10};
    int _data2{100};
};

class Widget;
std::vector<std::shared_ptr<Widget>> processedWidgets;

// class Widget
// {
// public:
//     void process()
//     {
//         processedWidgets.emplace_back(this);
//     }
// };

// class Widget : public std::enable_shared_from_this<Widget>
// {
// public:
//     void process()
//     {
//         processedWidgets.emplace_back(shared_from_this());
//     }
// };

class Widget : public std::enable_shared_from_this<Widget>
{
public:
    template <typename... Ts>
    static std::shared_ptr<Widget> create(Ts &&...params)
    {
        return std::shared_ptr<Widget>(new Widget(std::forward<Ts>(params)...));
    }
    void process()
    {
        processedWidgets.emplace_back(shared_from_this());
    }

private:
    Widget(int data) : _data(data){};

    int _data;
};

int main()
{
    {
        auto w = Widget::create(1);
        // Widget w;
        w->process();
    }
    std::cout << "over" << std::endl;
    return 0;
}