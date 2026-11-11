#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <type_traits>

struct test1
{
    static int SubType;
};

struct test2
{
    typedef int SubType;
};

template <typename T>
class MyClass
{
public:
    void foo()
    {
        typename T::SubType *ptr;
    }
};

template <class T>
using myVector = std::vector<T>;

template <typename T>
struct myVector2
{
    typedef std::vector<T> type;
};

template <>
class myVector2<float>
{
    enum class WineType
    {
        White,
        Red,
        Rose
    };
    WineType type;
};

template <typename T>
class Widget
{
public:
    myVector<T> list;
};

template <typename T>
class Widget2
{
public:
    typename myVector2<T>::type list;
};

template <typename T>
struct Trait1
{
    void foo()
    {
        int mm = 10;
        // typename std::remove_const<T>::type temp1 = mm;
        std::remove_const_t<T> temp1 = mm;
        // typename std::remove_reference<T>::type temp2 = mm;
        std::remove_reference_t<T> temp2 = mm;
    }
};

class MMM
{
public:
    static int aa;
};
int main()
{

    // MyClass<test1> class1;
    // class1.foo();
    MyClass<test2> class2;
    class2.foo();
    myVector<int> myvec = {1, 2, 3};
    myVector2<int>::type myvec2 = {4, 5, 6}; // 此处不用加typename是因为T已经确定啦

    Widget<int> weight;
    Trait1<const int &> trait1;
    trait1.foo();

    return 0;
}