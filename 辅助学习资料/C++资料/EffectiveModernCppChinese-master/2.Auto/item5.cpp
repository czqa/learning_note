#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

template <typename It> // 对从b到e的所有元素使用
void dwim(It b, It e)  // dwim（“do what I mean”）算法
{
    while (b != e)
    {
        // typename std::iterator_traits<It>::value_type currValue = *b;
        auto currValue = *b;
        std::cout << currValue << std::endl;
        b++;
    }
}

int main()
{
    int a = 10;
    const float &b = a;
    // float tmp = a;
    // const float&b = tmp;

    std::vector<int> vec = {1, 2, 3, 4};
    dwim(vec.cbegin(), vec.cend());

    std::unordered_map<std::string, int> m{{"hello", 10}, {"world", 5}, {"heihei", 20}};
    for (const std::pair<std::string, int> &p : m)
    {
    }
    for (const auto &p : m)
    {
    }

    std::pair<const std::string, int> const_testMap = {"hello", 5};
    const std::pair<std::string, int> &testMapRef = const_testMap;
    return 0;
}