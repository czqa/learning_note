#include <iostream>
#include <typeinfo>

class A
{
public:
    A()
    {
        data1_ = new int();
        data2_ = new int();
    }
    virtual void vAfunc1() { std::cout << "A::vAfunc1()" << std::endl; };
    virtual void vAfunc2() { std::cout << "A::vAfunc2()" << std::endl; };

    void func1() { std::cout << "A::func1()" << std::endl; };
    void func2() { std::cout << "A::func2()" << std::endl; };

    virtual ~A()
    {
        std::cout << "A::~A" << std::endl;
        delete data1_;
        delete data2_;
    }

private:
    int *data1_{};
    int *data2_{};
};

class B : public A
{
public:
    B()
    {
        data3_ = new int();
    }
    virtual void vAfunc1() override { std::cout << "B::vAfunc1()" << std::endl; };
    virtual void vBfunc1() { std::cout << "B::vBfunc1()" << std::endl; };
    virtual void vBfunc2() { std::cout << "B::vBfunc2()" << std::endl; };

    void func2() { std::cout << "B::func2()" << std::endl; };

    virtual ~B()
    {
        std::cout << "B::~B" << std::endl;
        delete data3_;
    }

private:
    int *data3_{};
};

class C : public A
{
public:
    C()
    {
        data4_ = new int();
    }
    virtual void vAfunc1() override { std::cout << "C::vAfunc1()" << std::endl; };
    void func3() { std::cout << "C::func3()" << std::endl; };

    virtual ~C()
    {
        std::cout << "C::~C" << std::endl;
        delete data4_;
    }

private:
    int *data4_{};
};

void test(A *a)
{
    if (typeid(*a) == typeid(A))
    {
        A *pa = reinterpret_cast<A *>(a);
        pa->func1();
    }
    if (typeid(*a) == typeid(B))
    {
        B *pb = reinterpret_cast<B *>(a);
        pb->func2();
    }
    if (typeid(*a) == typeid(C))
    {
        C *pc = reinterpret_cast<C *>(a);
        pc->func3();
    }
}

using Fun = void();
int main()
{
    B b;
    Fun *pFun = nullptr;
    std::cout << "虚函数表地址 " << (int64_t *)*(int64_t *)(&b) << std::endl;
    std::cout << "虚函数表第1个函数地址 " << (int64_t *)*((int64_t *)*(int64_t *)(&b)) << std::endl;
    pFun = (Fun *)*((int64_t *)*(int64_t *)(&b));
    pFun();
    std::cout << "虚函数表第2个函数地址 " << (int64_t *)*((int64_t *)*(int64_t *)(&b) + 1) << std::endl;
    pFun = (Fun *)*((int64_t *)*(int64_t *)(&b) + 1);
    pFun();
    // std::cout << "虚函数表第3个函数地址 " << (int64_t *)*((int64_t *)*(int64_t *)(&b) + 2) << std::endl;
    // pFun = (Fun *)*((int64_t *)*(int64_t *)(&b) + 2);
    // pFun();
    // std::cout << "虚函数表第4个函数地址 " << (int64_t *)*((int64_t *)*(int64_t *)(&b) + 3) << std::endl;
    // pFun = (Fun *)*((int64_t *)*(int64_t *)(&b) + 3);
    // pFun();
    std::cout << "虚函数表第5个函数地址 " << (int64_t *)*((int64_t *)*(int64_t *)(&b) + 4) << std::endl;
    pFun = (Fun *)*((int64_t *)*(int64_t *)(&b) + 4);
    pFun();
    std::cout << "虚函数表第6个函数地址 " << (int64_t *)*((int64_t *)*(int64_t *)(&b) + 5) << std::endl;
    pFun = (Fun *)*((int64_t *)*(int64_t *)(&b) + 5);
    pFun();

    A a;
    a.vAfunc1();
    a.func1();

    b.vAfunc1();
    b.func1();
    b.func2();

    A aa = b;
    std::cout << "aa虚函数表地址 " << (int64_t *)*(int64_t *)(&aa) << std::endl;
    aa.vAfunc1(); // B::vfunc1
    aa.func1();
    aa.func2();
    std::cout << " =================== " << std::endl;

    A *aaptr = &b;
    std::cout << "aaptr虚函数表地址 " << (int64_t *)*(int64_t *)(aaptr) << std::endl;
    aaptr->vAfunc1();
    aaptr->func1();
    aaptr->func2();

    std::type_info *base_type = (std::type_info *)*((int64_t *)*(int64_t *)(&a) - 1);
    std::cout << "typeinfo is:" << base_type->name() << std::endl;
    std::cout << "the result of typeid(Base).name():" << typeid(A).name() << std::endl;
    std::cout << "====== " << typeid(a).name() << std::endl;
    std::cout << "====== " << typeid(&a).name() << std::endl;

    A *base = &b;
    test(base);
    C c;
    base = &c;
    test(base);

    A *mm = dynamic_cast<A *>(&b);
    B *nn = dynamic_cast<B *>(mm);

    A qqq;
    B *ppp = dynamic_cast<B *>(&qqq);

    return 0;
}
