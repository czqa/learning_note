#include <iostream>
#include <memory>

class Widget
{
public:
    Widget() { std::cout << "default ctor" << std::endl; }
    // Widget(Widget &&rhs) // rhs是右值引用
    //     : name(std::move(rhs.name))
    // {
    //     std::cout << "move ctor" << std::endl;
    // }
    // Widget(const Widget &rhs) // rhs是const左值引用
    //     : name(rhs.name)
    // {
    //     std::cout << "copy ctor" << std::endl;
    // }

    template <typename T>
    void setName(T &&newName) // newName是通用引用
    {
        name = std::forward<T>(newName);
        // name = std::move(newName);
    }

    void setName(std::string &&newName) // newName是右值引用
    {
        std::cout << "move setName" << std::endl;
        name = std::move(newName);
        // name = newName;
    }
    void setName(const std::string &newName)
    {
        std::cout << "copy setName" << std::endl;
        name = newName;
    }

private:
    std::string name;
};

Widget func(Widget &&w)
{
    // return std::move(w);
    return w;
}

//  void func(Widget &&w, Widget &_w)
//  { return _w.Widget::Widget(w);}
//

int main()
{
    std::string name = "hkl";
    // Widget w = func(Widget());
    Widget w;
    w.setName(name);
    w.setName(std::move(name));
    w.setName("XiaoMing");
    // std::string temp("XiaoMing");
    // std::string name = temp;

    // "XiaoMing"
    auto a = &"XiaoMing";
    return 0;
}