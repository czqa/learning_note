#include <iostream>
#include <string>
#include <tuple>

enum Color
{
    black = 5,
    white = 5,
    red
};

enum School : int;
enum class EnumInt : short;
class A
{
private:
    School _enumSchool;
    EnumInt e_int;
};

enum School : int
{
    teacher,
    student
};

enum Company
{
    manager,
    employee
};

enum class EnumInt : short
{
    e1 = 1,
    e2 = 2,
    e3
};

using UserInfo =             // 类型别名，参见Item9
    std::tuple<std::string,  // 名字
               std::string,  // email地址
               std::size_t>; // 声望

enum UserInfoFields
{
    uiName,
    uiEmail,
    uiReputation
};

enum class UserInfoFieldsScoped
{
    uiName,
    uiEmail,
    uiReputation
};

// c++11
template <typename E>
constexpr typename std::underlying_type<E>::type
toUType(E enumerator) noexcept
{
    return static_cast<typename std::underlying_type<E>::type>(enumerator);
}

// C++14
template <typename E>
constexpr std::underlying_type_t<E>
toUType_(E enumerator) noexcept
{
    return static_cast<std::underlying_type_t<E>>(enumerator);
}

// C++14
template <typename E>
constexpr auto
toUType__(E enumerator) noexcept
{
    return static_cast<std::underlying_type_t<E>>(enumerator);
}

int main()
{
    School a = student;
    int b = student;

    School e = static_cast<School>(10);
    // School c = 10;
    // Company d = student;
    EnumInt aa = EnumInt::e1;
    float bb = static_cast<float>(aa);
    // auto size = sizeof(EnumInt);
    // EnumInt e_i{10};
    // // E e_i_2 = {8};
    // // E e_i_3(10);
    // // E e_i_4 = 10;

    // bool a = EnumInt::e1 > EnumInt::e2;

    // // =========
    UserInfo uInfo{"hkl", "7xxx@email.com", 10};
    auto val = std::get<1>(uInfo); // 获取第一个字段
    auto val_ = std::get<uiEmail>(uInfo);
    auto val__ =
        std::get<static_cast<int>(UserInfoFieldsScoped::uiEmail)>(uInfo);
    auto val___ = std::get<toUType(UserInfoFieldsScoped::uiEmail)>(uInfo);
    UserInfoFieldsScoped aaa = UserInfoFieldsScoped::uiEmail;
    UserInfoFieldsScoped bbb{0};
    return 0;
}