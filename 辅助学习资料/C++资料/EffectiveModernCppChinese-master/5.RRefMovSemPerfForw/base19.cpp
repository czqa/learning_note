#include <iostream>
#include <vector>
#include <type_traits>

template <typename T>
struct Func1
{
    using type = T;
};

template <int a>
struct Func2
{
    int data[a];
};

// template <>
// struct Func1<int>
// {
//     using type = int;
//     using b_type = float;
// };

class Person
{
public:
    Person() = default;

    template <typename T, typename = typename Func1<T>::b_type>
    Person(T &&n) : _name(std::forward<T>(n))
    {
        std::cout << "Person(T &&n)" << std::endl;
    }

    Person(const Person &p)
    {
        std::cout << "Person(const Person &p)" << std::endl;
    }

private:
    std::string _name;
};

namespace
{
    template <bool, typename _Tp = void>
    struct enable_if
    {
    };

    // Partial specialization for true.
    template <typename _Tp>
    struct enable_if<true, _Tp>
    {
        typedef _Tp type;
    };
}

// class Person
// {
// public:
//     template <
//         typename T,
//         typename = typename std::enable_if<
//             !std::is_same<Person,
//                           typename std::decay<T>::type>::value>::type>
//     explicit Person(T &&n);
// };

int main()
{

    // template <typename T>
    // struct Func1
    // {
    //     using type = T;
    // };

    // template <int a>
    // struct Func2
    // {
    //     int data[a];
    // };

    Func1<int>::type value = 10;
    // Func1<int>::type_b value1 = 10;
    // int value = 10;
    Func2<1> f;

    Person p;
    Person p1(p);
    return 0;
}