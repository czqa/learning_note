#include <iostream>
#include <vector>
#include <string>
#include <memory>

class Widget
{
public:
    bool isValidated() const { return true; }
    bool isProcessed() const { return true; }
    bool isArchived() const { return true; }

private:
};

class IsValAndArch
{
public:
    using DataType = std::unique_ptr<Widget>;

    explicit IsValAndArch(DataType &&ptr)
        : pw(std::move(ptr)) {}

    bool operator()() const
    {
        return pw->isValidated() && pw->isArchived();
    }

private:
    DataType pw;
};

int main()
{
    // auto pw = std::make_unique<Widget>();
    // auto func = [pw = std::move(pw)]
    // { return pw->isValidated() && pw->isArchived(); };

    auto func = [pw = std::make_unique<Widget>()]
    { return pw->isValidated() && pw->isArchived(); };

    auto func1 = IsValAndArch(std::make_unique<Widget>());
    return 0;
}