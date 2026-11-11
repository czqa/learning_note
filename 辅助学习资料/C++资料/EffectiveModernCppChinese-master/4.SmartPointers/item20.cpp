#include <memory>
#include <iostream>
#include <unordered_map>

class Widget
{
public:
    Widget(int id) : _id(id) { std::cout << "Widget(int id)" << std::endl; }
    ~Widget() { std::cout << "~Widget()" << std::endl; }

private:
    int _id{-1};
};

std::unique_ptr<const Widget> loadWidget(int id)
{
    // 耗时操作
    std::unique_ptr<const Widget> uptr{new Widget(id)};
    return uptr;
}

std::shared_ptr<const Widget> fastLoadWidget(int id)
{
    static std::unordered_map<int, std::weak_ptr<const Widget>> cache;
    auto objPtr = cache[id].lock();
    if (!objPtr)
    {
        objPtr = loadWidget(id);
        cache[id] = objPtr;
    }
    return objPtr;
}

class B;
class A
{
public:
    A() { std::cout << "A()" << std::endl; }
    ~A() { std::cout << "~A()" << std::endl; }

    std::shared_ptr<B> b_ptr;
};

class C
{
public:
    C() { std::cout << "C()" << std::endl; }
    ~C() { std::cout << "~C()" << std::endl; }

    std::shared_ptr<B> b_ptr;
};

class B
{
public:
    B() { std::cout << "B()" << std::endl; }
    ~B() { std::cout << "~B()" << std::endl; }

    std::shared_ptr<A> a_ptr;
    // A *a_ptr;
};

int main()
{
    {
        auto spw = std::make_shared<Widget>(10);
        std::weak_ptr<Widget> wpw(spw);
        spw = nullptr;
        std::cout << "over" << std::endl;
    }

    // {
    //     auto widgetSPtr = fastLoadWidget(0);
    //     widgetSPtr = fastLoadWidget(1);
    //     std::cout << "over" << std::endl;

    //     auto widgetSPtr1 = fastLoadWidget(1);
    //     widgetSPtr = fastLoadWidget(3);
    // }

    // {
    //     std::shared_ptr<B> b = std::make_shared<B>();

    //     std::shared_ptr<A> a = std::make_shared<A>();
    //     std::shared_ptr<C> c = std::make_shared<C>();
    //     a->b_ptr = b;
    //     c->b_ptr = b;
    //     b->a_ptr = a;
    // }
    return 0;
}