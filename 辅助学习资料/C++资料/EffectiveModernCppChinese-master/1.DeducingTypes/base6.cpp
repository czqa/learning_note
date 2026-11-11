#include <iostream>
#include <string>

struct A
{
    int a{20};
    void funcA() { std::cout << "A::funA" << std::endl; }
};

struct B
{
    int b{};
    void funcB() { std::cout << "B::funB" << std::endl; }
};

struct CA : public A
{
    int c{25};
    void funcCA()
    {
        std::cout << "c is: " << c << std::endl;
        std::cout << "CA::funcCA" << std::endl;
    }
};

struct Base
{
    virtual void func()
    {
        std::cout << "Base::func()" << std::endl;
    }
};

struct Derived : public Base
{
    virtual void func() override
    {
        std::cout << "Derived::func()" << std::endl;
    }
};

const std::string &shorterString(const std::string &s1,
                                 const std::string &s2)
{
    return s1.size() <= s2.size() ? s1 : s2;
}

std::string &shorterString(std::string &s1,
                           std::string &s2)
{
    auto &r = shorterString(const_cast<const std::string &>(s1),
                            const_cast<const std::string &>(s2));
    // return r;
    return const_cast<std::string &>(r);
}
int main()
{
    // static_cast 1.
    float f = 10.f;
    int i = static_cast<int>(f);

    // static_cast 2.
    A a;
    void *vap = static_cast<void *>(&a);
    int *ip = static_cast<int *>(vap);
    std::cout << "*ip: " << *ip << std::endl;

    // static_cast 3.
    CA ca;
    A a_ = static_cast<A>(ca); // 上行ok
    a_.funcA();
    // CA ca_ = static_cast<CA>(a); // 下行报错

    // static_cast 4.
    Base base;
    Derived derived;

    Base _base = static_cast<Base>(derived); // 上行ok，但是没有多态
    _base.func();
    // Derived _derived = static_cast<Derived>(base);// 下行报错

    // static_cast 5.
    CA *ca_ = static_cast<CA *>(&a);
    ca_->funcCA(); // 下行不安全
    CA &car_ = static_cast<CA &>(a);
    car_.funcCA(); // 下行不安全

    // static_cast 6.
    Base *_baseptr = static_cast<Base *>(&derived); // 上行ok，多态
    _baseptr->func();
    Derived *_derivedptr = static_cast<Derived *>(&base); // 下行不安全
    _derivedptr->func();
    Derived *_derivedptr_ = static_cast<Derived *>(_baseptr); // 下行安全
    _derivedptr_->func();

    // dynamic_cast
    Base *_baseptr_d = dynamic_cast<Base *>(&derived); // 上行ok，多态
    _baseptr_d->func();
    Derived *_derivedptr_d = dynamic_cast<Derived *>(&base); // 下行不安全
    if (_derivedptr_d)
    {
        _derivedptr_d->func();
    }
    Derived *_derivedptr__d = dynamic_cast<Derived *>(_baseptr); // 下行安全
    _derivedptr__d->func();

    // const_cast
    const char *pc = "hello world";
    // char *p = pc;
    char *p = const_cast<char *>(pc);
    // p[2] = 'a'; // 段错误
    std::string aas = "hello";
    std::string bbs = "ll";
    shorterString(aas, bbs);

    // reinterpred_cast 1.
    int *i_ = reinterpret_cast<int *>(i);
    int64_t cpi = reinterpret_cast<int64_t>(pc);

    // reinterpred_cast 2.
    int *vap_ = reinterpret_cast<int *>(&a);

    return 0;
}