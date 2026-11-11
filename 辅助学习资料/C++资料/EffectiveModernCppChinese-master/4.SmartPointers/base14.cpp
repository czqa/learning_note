#include <iostream>
#include <vector>
#include <string>

class Test
{
public:
    Test() { std::cout << "Test() addr is: " << this << std::endl; }
    Test(int a) : _a(a)
    {
        std::cout << "Test(int a) addr is: " << this << std::endl;
    }
    Test(int a, int b) { std::cout << "Test(int a, int b) addr is: " << this << std::endl; }
    ~Test() { std::cout << "~Test() addr is: " << this << std::endl; }

    void *operator new(size_t size)
    {
        std::cout << "operator new(size_t size)" << std::endl;
        return malloc(size);
    }

    void *operator new(size_t size, void *buf)
    {
        std::cout << "operator new(size_t size, void* buf)" << std::endl;
        return buf;
    }

    void operator delete(void *ptr, size_t size)
    {
        std::cout << "operator delete(void* ptr, size_t size)" << std::endl;
        free(ptr);
    }

    void operator delete(void *ptr, void *buf)
    {
        std::cout << "operator delete(void* ptr, void* buf)" << std::endl;
        free(ptr);
    }

private:
    int _a;
};

int main()
{
    // void *p1 = malloc(sizeof(Test) * 10);
    // free(p1);
    // int *p2 = (int *)malloc(sizeof(Test) * 10);
    // free((++p2)); // 会崩溃，因为p2指向别处，他就不知道应该释放多少堆内存啦

    // Test *p3 = new Test();
    // delete p3;

    void *buf = malloc(sizeof(Test));
    Test *p4 = new (buf) Test(500);
    delete p4; // 会调用operator delete(void* ptr)
    return 0;
}