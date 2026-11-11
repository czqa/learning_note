#include <vector>
#include <functional>

using FilterContainer =
    std::vector<std::function<bool(int)>>;

FilterContainer filters;

void addDivisorFilter()
{
    int divisor = 5;
    filters.emplace_back(
        [&](int value)
        { return value % divisor == 0; });
}

class Widget
{
public:
    void addFilter()
    {
        filters.emplace_back([=](int value)
                             { return value % divisor == 0; });
    }

private:
    int divisor = 10;
};

int main()
{
    addDivisorFilter();
    {
        Widget w;
        w.addFilter();
    }
    return 0;
}