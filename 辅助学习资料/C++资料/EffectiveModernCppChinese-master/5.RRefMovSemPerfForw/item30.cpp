#include <vector>

// {}
// void f(const std::vector<int> &v) {}

// NULL / 0
// void f(void *ptr) {}

// static const
// class widget
// {
// public:
//     static const std::size_t MindVal = 28;
// };

// void f(std::size_t value)
// {
// }

// overload / template
void f(int (*pf)(int)) {}
int processVal(int value) { return 0; }
int processVal(int value, int priority) { return 0; }

template <typename T>
T workonVal(T param) { return param; }

template <typename T>
void fwd(T &&param)
{
    f(std::forward<T>(param));
}

int main()
{
    // fwd({1, 2, 3});
    // auto il = {1, 2, 3};
    // fwd(il);
    // fwd(0);
    // f(processVal);
    // fwd(processVal);
    // fwd(workonVal);
    return 0;
}