#include <iostream>
#include <string>
#include <vector>

class Base
{
public:
    virtual void mf1() const &
    {
        std::cout << "Base:: virtual mf1" << std::endl;
    };
    void mf2()
    {
        std::cout << "Base:: mf2" << std::endl;
    }
    virtual void mf3()
    {
        std::cout << "Base:: virtual mf3" << std::endl;
    }

    virtual void mf4() final
    {
        std::cout << "Base:: virtual mf4" << std::endl;
    }
};

class Derived : public Base
{
public:
    void mf1() const & override
    {
        std::cout << "Derived:: virtual mf1" << std::endl;
    };
    void mf2()
    {
        std::cout << "Derived:: mf2" << std::endl;
    }
    void mf3() override
    {
        std::cout << "Derived:: virtual mf3" << std::endl;
    }

    // void mf4() override
    // {
    //     std::cout << "Derived:: virtual mf4" << std::endl;
    // }
};

class Widget
{
public:
    using DataType = std::vector<double>;
    DataType &data() & // 对于左值Widgets,
    {
        return values;
    } // 返回左值

    DataType data() && // 对于右值Widgets,
    {
        return std::move(values);
    } // 返回右值

private:
    DataType values;
};

Widget makeWidget()
{
    return Widget();
}

int main()
{
    // Base b1;
    // b1.mf1();
    // b1.mf2();

    Derived d1;
    // d1.mf1();
    // d1.mf2();

    Base *b2 = &d1;
    b2->mf1();
    b2->mf2();
    b2->mf3();

    Widget w;
    auto vals1 = w.data(); // 拷贝w.values到vals1
    auto vals2 = makeWidget().data();
}