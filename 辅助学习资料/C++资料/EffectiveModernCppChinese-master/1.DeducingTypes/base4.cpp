#include <iostream>
using namespace std;
struct Test1
{
    int a;   // 4
    short b; // 2 + 2
    int c;   // 4
    short d; // 2  +  2
};

struct Test2
{
    int a;   // 4
    int c;   // 4
    short b; // 2
    short d; // 2
};

struct Test3
{
    int a;
    short b;
};

struct Test4 : public Test3
{
    short c;
};

class A
{
public:
    virtual void vAfunc1() { cout << "A::vAfunc1()" << endl; };
    virtual void vAfunc2() { cout << "A::vAfunc2()" << endl; };
    //     void func1() { cout << "A::func1()" << endl; };
    //     void func2() { cout << "A::func2()" << endl; };

    // private:
    //     int data1_;
    //     int data2_;
};

class B : public A
{
public:
    virtual void vBfunc1() { cout << "B::vBfunc1()" << endl; };
    virtual void vBfunc2() { cout << "B::vBfunc2()" << endl; };

    virtual void vAfunc1() override { cout << "B::vAfunc1()" << endl; };
    void func2() { cout << "B::func2()" << endl; };

private:
    int data3_;
};

using Fun = void();
int main()
{
    auto count1 = sizeof(Test3);
    auto count2 = sizeof(Test4);

    A a;
    Fun *pFun = nullptr;
    auto mm = &a;
    auto t1 = (int64_t *)(&a);
    auto t2 = (int64_t *)*t1;
    auto t3 = (Fun *)*(t2);
    t3();

    A a1;
    std::cout << "a1虚表的地址为 " << (int64_t *)*(int64_t *)(&a1) << std::endl;
    std::cout << "a1虚表中第一个函数地址为 " << (int64_t *)*((int64_t *)*(int64_t *)(&a1)) << std::endl;
    std::cout << "a1虚表中第二个函数地址为 " << (int64_t *)*((int64_t *)*(int64_t *)(&a1) + 1) << std::endl;
    std::cout << "over" << std::endl;

    return 0;
}