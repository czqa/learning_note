#include <vector>
#include <iostream>

class A
{
public:
    A() { std::cout << "A::A()" << std::endl; }
    A(int data) : _data(data) { std::cout << "A::A(int data)" << std::endl; }
    A(const A &a) : _data(a._data)
    {
        std::cout << "A::A(const A &a)" << std::endl;
    }
    A(A &&a) : _data(a._data)
    {
        std::cout << "A::A(A &&a)" << std::endl;
    }
    ~A() { std::cout << "A::~A()" << std::endl; }

    int _data{100};
};

std::vector<A> func()
{
    return {A(1), A(2), A(3), A(4), A(5)};
}

std::vector<bool> features()
{
    return {true, true, true, false, false};
}

class MyArray
{
public:
    class MyArraySize
    {
    public:
        MyArraySize(int size) : theSize(size) {}
        int size() const { return theSize; }
        operator int() const { return theSize; }

    private:
        int theSize;
    };

    MyArray(MyArraySize size) : size_(size), data_(new int[size.size()]) {}
    int operator[](int index)
    {
        return data_[index];
    }
    bool operator==(const MyArray &temp)
    {
        return data_ == temp.data_;
    }
    MyArraySize size() { return size_; }

private:
    int *data_;
    MyArraySize size_;
};

class MyArray_
{
public:
    MyArray_(int size) : size_(size), data_(new int[size]) {}

private:
    int *data_;
    int size_;
};

void func1(MyArray arr)
{
}

int main()
{
    // func1(MyArray(10));
    bool highPriority = features()[4];
    auto highPriority_ = static_cast<bool>(features()[4]);

    // A aaa = func()[2];
    A &bbb = func()[3];

    MyArray arr(10);
    auto size = arr.size();
    int size_ = arr.size();
    return 0;
}