#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> names;

template <typename T>
void logAndAdd(T &&name)
{
    // ...
    names.emplace_back(std::forward<T>(name));
}

std::string nameFromIdx(int idx)
{
    return "name";
}

void logAndAdd(int idx) // 新的重载
{
    names.emplace_back(nameFromIdx(idx));
}

class Person
{
public:
    template <typename T> // 完美转发的构造函数
    explicit Person(T &&n)
        : name(std::forward<T>(n))
    {
    }

    explicit Person(int idx)
    {
    }

    Person(const Person &rhs) {}
    Person(Person &&rhs) {}

    std::string name;
};

// class SpecialPerson : public Person
// {
// public:
//     SpecialPerson(const SpecialPerson &rhs) // 拷贝构造函数，调用基类的
//         : Person(rhs)                       // 完美转发构造函数！
//     {
//     }

//     SpecialPerson(SpecialPerson &&rhs) // 移动构造函数，调用基类的
//         : Person(std::move(rhs))       // 完美转发构造函数！
//     {
//     }
// };

int main()
{
    std::string petName("Darla");
    logAndAdd(petName);                   // 传递左值std::string
    logAndAdd(std::string("Persephone")); // 传递右值std::string
    logAndAdd("Patty Dog");               // 传递字符串字面值

    // short nameIdx = 10;
    // logAndAdd(nameIdx); // 传递左值short

    Person p("Nancy");
    // auto cloneOfP(p); // 拷贝p
    return 0;
}