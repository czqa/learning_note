#include <thread>
#include <iostream>
#include <vector>
#include <functional>

using Time = std::chrono::steady_clock::time_point;
enum class Sound
{
    Beep,
    Siren,
    Whistle
};

using Duration = std::chrono::steady_clock::duration;
void setAlarm(Time t, Sound s, Duration d)
{
}

void func(int a, int b, int c)
{
    std::cout << "void func(int a, int b ,int c=10)" << std::endl;
}

void func(int a, int b)
{
    std::cout << "void func(int a, int b)" << std::endl;
}

int main()
{
    auto setSoundL =
        [](Sound s)
    {
        using namespace std::chrono;
        using namespace std::literals; // 对于C++14后缀

        setAlarm(steady_clock::now() + 1h, // C++14写法，但是含义同上
                 s,
                 30s);
    };
    // 调用setSoundL函数之后一小时会响铃30s

    using namespace std::chrono;
    using namespace std::literals;
    using namespace std::placeholders;

    auto setSoundB =
        std::bind(setAlarm,
                  steady_clock::now() + 1h,
                  _1,
                  30s);
    // 构造setSoundB函数之后一小时会响铃30s

    auto setSoundBB =
        std::bind(setAlarm,
                  std::bind(std::plus<>(), std::bind(steady_clock::now), 1h),
                  _1,
                  30s);
    // 调用setSoundBB函数之后一小时会响铃30s
    using func3ParamType = void (*)(int a, int b, int c);
    // auto funB = std::bind(func, 5, std::placeholders::_1, 10);
    auto funB = std::bind(static_cast<func3ParamType>(func), 5, std::placeholders::_1, 10);
    return 0;
}