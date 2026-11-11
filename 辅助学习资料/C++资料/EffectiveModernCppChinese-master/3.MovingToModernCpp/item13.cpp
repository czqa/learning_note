#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

typedef std::vector<int>::iterator IterT; // typedef
typedef std::vector<int>::const_iterator ConstIterT;

template <class C>
auto cbegin(const C &container) -> decltype(std::begin(container))
{
    return std::begin(container);
}

template <class C>
auto cend(const C &container) -> decltype(std::end(container))
{
    return std::end(container);
}

template <typename C, typename V>
void findAndInsert(C &container,       // 在容器中查找第一次
                   const V &targetVal, // 出现targetVal的位置，
                   const V &insertVal) // 然后在那插入insertVal
{
    // using std::cbegin;
    // using std::cend;

    auto it = std::find(cbegin(container), // 非成员函数cbegin
                        cend(container),   // 非成员函数cend
                        targetVal);
    container.insert(it, insertVal);
}

int main()
{
    std::vector<int> avec = {1, 2, 3, 4, 5};
    // ========= 使用iterator
    // std::vector<int>::iterator it =
    //     std::find(avec.begin(), avec.end(), 2);
    // avec.insert(it, 22);

    // ========= 使用const_iterator after C++11
    std::vector<int>::const_iterator cit = std::find(avec.cbegin(), avec.cend(), 2);
    std::cout << *cit << std::endl;
    avec.insert(cit, 22);

    // ========= 使用const_iterator before C++11
    // ConstIterT ci =
    //     std::find(static_cast<ConstIterT>(avec.begin()), // cast
    //               static_cast<ConstIterT>(avec.end()),   // cast
    //               2);
    // avec.insert(static_cast<IterT>(ci), 22); // 这句话报错 因为iterator与const_iterator无法转换
    const std::vector<int> caVec = {1, 2, 3, 4};
    auto cit = std::begin(caVec);

    return 0;
}