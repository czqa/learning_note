#include <iostream>
#include <string>
#include <vector>
#include <array>

int get_value()
{
    return 1;
}

constexpr int get_value_c()
{
    return 1;
}

constexpr int abs_(int x)
{

    if (x > 0)
        return x;
    else
        return -x;

    // return x > 0 ? x : -x;
}

// constexpr int next(int param)
// {
//     return ++param;
// }
constexpr void func()
{
    // int a = 50;
    // a = 60;
}

class A
{
public:
    constexpr A() : x1(5)
    {
        int a = 10;
        a = 50;
    }
    constexpr A(int i) : x1(i) {}
    constexpr int get()
    {
        return x1;
    }

    ~A() = default;

    int x1;
};

class Point
{
public:
    constexpr Point(double xVal = 0, double yVal = 0) noexcept
        : x(xVal), y(yVal)
    {
    }

    constexpr double xValue() const noexcept { return x; }
    constexpr double yValue() const noexcept { return y; }

    // constexpr Point(Point &&p) : x(p.x), y(p.y)
    // {
    // }

    // ~Point() = delete;
    constexpr void setX(double newX) noexcept { x = newX; }
    constexpr void setY(double newY) noexcept { y = newY; }

private:
    double x, y;
};

// 返回p相对于原点的镜像
constexpr Point reflection(const Point &p) noexcept
{
    Point result;             // 创建non-const Point
    result.setX(-p.xValue()); // 设定它的x和y值
    result.setY(-p.yValue());
    return result; // 返回它的副本
}

constexpr Point midpoint(const Point &p1, const Point &p2) noexcept
{
    return {(p1.xValue() + p2.xValue()) / 2,  // 调用constexpr
            (p1.yValue() + p2.yValue()) / 2}; // 成员函数
}

int main()
{

    // 1.常量的不确定性
    int e = 10;
    const int a = 10;
    const int b = 1 + 2;
    const int c = e;
    const int d = get_value();

    constexpr int f = get_value_c();
    // constexpr int g = abs_(e);

    constexpr int h = abs_(a);

    char array_a[a];
    char array_b[b];
    char array_c[c];
    char array_d[d];

    char array_e[e];

    std::array<int, a> array_aa{};
    std::array<int, b> array_bb{};
    // std::array<int, c> array_cc{};
    // std::array<int, d> array_dd{}; //这里报错的原因是模板参数要求是编译期常量

    constexpr int value = abs_(-100);
    const int value_1 = abs_(-1000);
    auto value_2 = abs_(-1000);
    const int value_3 = abs_(e);
    // constexpr int value_4 = abs_(e);

    constexpr A a1;

    constexpr A a2(10);
    constexpr A a3(5);
    // constexpr int mmm = a3.get();

    // a3.func(50);
    Point p0(5.6, 8.6);
    Point p5(0.0, 0.0);
    // constexpr Point p1(9.4, 27.7); // 和之前一样
    // constexpr Point p2(28.8, 5.3);
    // constexpr auto mid = midpoint(p1, p2);
    // constexpr auto reflectedMid = reflection(mid);
    // auto mid2 = midpoint(p0, p5);
}