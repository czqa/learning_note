#pragma once

// #include <vector>
// #include <string>
// #include <iostream>
// #include "Gadget.h"

// class Widget
// {
// public:
//     Widget();

// private:
//     std::string name;
//     std::vector<double> data;
//     Gadget g1, g2, g3; // Gadget是用户自定义的类型
// };

// ==========================

// #include <vector>
// #include <string>
// #include <iostream>

// class Widget
// {
// public:
//     Widget();
//     ~Widget();

// private:
//     struct Impl;
//     Impl *pImpl;
// };

// ========================

#include <vector>
#include <string>
#include <memory>
#include <iostream>

class Widget
{
public:
    Widget();
    ~Widget();
    Widget(Widget &&rhs);
    Widget &operator=(Widget &&rhs);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};
