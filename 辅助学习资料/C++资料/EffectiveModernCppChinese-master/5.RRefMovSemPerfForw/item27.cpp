#include <iostream>
#include <vector>
#include <type_traits>

// 1.
class Person
{
public:
    // template <typename T> // 完美转发的构造函数
    // explicit Person(T &&n)
    //     : name(std::forward<T>(n))
    // {
    // }
    explicit Person(std::string name) : _name(std::move(name)) {}

    explicit Person(int idx)
    {
    }

    Person(const Person &rhs) {}
    Person(Person &&rhs) {}

    std::string _name;
};

// 2.
std::vector<std::string> names;

// template <typename T>
// void logAndAdd(T &&name)
// {
//     // ...
//     names.emplace_back(std::forward<T>(name));
// }

// std::string nameFromIdx(int idx)
// {
//     return "name";
// }

// void logAndAdd(int idx) // 新的重载
// {
//     names.emplace_back(nameFromIdx(idx));
// }

template <typename T>
void logAndAddImpl(T &&name, std::false_type)
{
    //    ...
    names.emplace_back(std::forward<T>(name));
}

std::string nameFromIdx(int idx) { return "name"; }
void logAndAddImpl(int idx, std::true_type)
{
    names.emplace_back(nameFromIdx(idx));
}

template <typename T>
void logAndAdd(T &&name)
{
    logAndAddImpl(std::forward<T>(name),
                  std::is_integral<typename std::remove_reference<T>::type>());
}

// 3.
class Person1
{
public:
    template <
        typename T,
        typename = std::enable_if_t<
            !std::is_base_of<Person1, std::decay_t<T>>::value &&
            !std::is_integral<std::remove_reference_t<T>>::value>>
    explicit Person1(T &&n)        // 对于std::strings和可转化为
        : name(std::forward<T>(n)) // std::strings的实参的构造函数
    {
        static_assert(
            std::is_constructible<std::string, T>::value,
            "Parameter n can't be used to construct a std::string");
    }

    explicit Person1(int idx) // 对于整型实参的构造函数
        : name(nameFromIdx(idx))
    {
    }

private:
    std::string name;
};

int main()
{
    // logAndAdd("hkl");
    // logAndAdd(5);
    // short mm = 10;
    // logAndAdd(mm);
    // Person p;
    // Person p1(p);
    // Person1 pp(u"sss"); // char char16_t
    return 0;
}