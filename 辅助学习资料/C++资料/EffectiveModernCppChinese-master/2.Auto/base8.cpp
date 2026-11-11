#include <iostream>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <map>

int add(int i, int j)
{
    return i + j;
}

int modfunc(int i, int j, int m)
{
    m = 20;
    return i % j;
}

struct divide
{
    int operator()(int denominator, int divisor)
    {
        return denominator / divisor;
    }
};

using Fun = int(int, int);

bool isShorter(const std::string &a, int sz)
{
    return a.size() >= sz;
}

int main()
{
    std::vector<std::string> wordVec{"fun", "hello", "world"};

    int sz = 4;
    auto wc = find_if(wordVec.begin(), wordVec.end(), [sz](const std::string &a)
                      { return a.size() >= sz; });

    using namespace std::placeholders;
    auto fun = std::bind(isShorter, _1, sz);
    auto wc2 = find_if(wordVec.begin(), wordVec.end(), fun);

    int mm = 0;
    // auto mod_ = [mm](int i, int j)
    // { return i % j; };

    Fun *mod = [](int i, int j)
    { return i % j; };

    std::map<std::string, int (*)(int, int)> funcMap;

    funcMap.insert({"+", add});
    funcMap.insert({"%", mod});
    // funcMap.insert({"%", mod_});

    std::map<std::string, std::function<int(int, int)>> funcMap2;
    funcMap2.insert({"+", add});
    funcMap2.insert({"%", mod});
    auto modaaa = std::bind(modfunc, _1, _2, 20);

    std::map<std::string, std::function<int(int, int)>> binops = {
        {"+", add},
        {"++", &add},
        {"/", divide()},
        {"*", [](int i, int j)
         { return i * j; }},
        {"%", mod}};

    return 0;
}