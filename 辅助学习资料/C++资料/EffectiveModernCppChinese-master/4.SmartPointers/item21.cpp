#include <iostream>
#include <memory>
#include <vector>

class Airplane
{
private:
    struct AirplaneRep
    {
        unsigned long miles{10}; // 8
        char type{'A'};          // 1       16
    };
    union
    {
        AirplaneRep rep{}; // 16
        Airplane *next;    // 8
    };                     // 16

public:
    unsigned long
    getMiles()
    {
        return rep.miles;
    }
    char getType() { return rep.type; }
    void set(unsigned long m, char t)
    {
        rep.miles = m;
        rep.type = t;
    }
    static void *operator new(size_t size);
    static void operator delete(void *ptr);
    ~Airplane() { std::cout << "Airplane::~Airplane()" << std::endl; }

private:
    static const int BLOCK_SIZE;
    static Airplane *headOfFreeList;
};

Airplane *Airplane::headOfFreeList;
const int Airplane::BLOCK_SIZE = 512;

void *Airplane::operator new(size_t size)
{
    // if (size != sizeof(Airplane))
    // {
    //     return ::operator new(size);
    // }
    Airplane *p = headOfFreeList;
    if (p)
    {
        headOfFreeList = p->next;
    }
    else
    {
        Airplane *newBlock = static_cast<Airplane *>(::operator new(BLOCK_SIZE * sizeof(Airplane)));
        for (int i = 1; i < BLOCK_SIZE - 1; ++i)
        {
            newBlock[i].next = &newBlock[i + 1];
        }
        newBlock[BLOCK_SIZE - 1].next = 0;
        p = newBlock;
        headOfFreeList = &newBlock[1];
    }
    return p;
}

void Airplane::operator delete(void *ptr)
{
    if (ptr == 0)
    {
        return;
    }
    // if (size != sizeof(Airplane))
    // {
    //     ::operator delete(ptr);
    //     return;
    // }
    Airplane *deleteMe = static_cast<Airplane *>(ptr);
    deleteMe->next = headOfFreeList;
    headOfFreeList = deleteMe;
}

int main()
{

    auto upv = std::make_unique<std::vector<int>>(10, 20);
    auto spv = std::make_shared<std::vector<int>>(10, 20);
    // auto spv_ = std::make_shared<std::vector<int>>({10, 20});

    // 创建std::initializer_list
    auto initList = {10, 20};
    // 使用std::initializer_list为形参的构造函数创建std::vector
    auto spv_ = std::make_shared<std::vector<int>>(initList);

    auto aspv = std::make_shared<Airplane>();
    auto aspv_ = std::shared_ptr<Airplane>(new Airplane());

    auto pBigType = std::make_shared<Airplane>();
    std::weak_ptr<Airplane> pBigTypeWeak(pBigType);
    pBigType = nullptr;
    std::cout << "over" << std::endl;
    return 0;
}