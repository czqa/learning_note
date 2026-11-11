#include <iostream>
#include <string.h>
#include <string>

// copy from 【现代 C++ 语言核心特性解析】
class BigMemoryPool
{
public:
    static const int PoolSize = 4096;
    BigMemoryPool() : pool_(new char[PoolSize]) {}
    ~BigMemoryPool()
    {
        if (pool_ != nullptr)
        {
            delete[] pool_;
        }
    }
    BigMemoryPool(BigMemoryPool &&other)
    {
        std::cout << "move" << std::endl;
        pool_ = other.pool_;
        other.pool_ = nullptr;
    }
    BigMemoryPool(const BigMemoryPool &other) : pool_(new char[PoolSize])
    {
        std::cout << "copy" << std::endl;
        memcpy(pool_, other.pool_, PoolSize);
    }

private:
    char *pool_;
};

BigMemoryPool getPool()
{
    BigMemoryPool memoryPool;
    return memoryPool;
}

BigMemoryPool getPool(const BigMemoryPool aa)
{
    return aa;
}

int gx = 10;
int get_gx()
{
    return gx;
}

int get_x()
{
    int x = 20;
    return x;
}
BigMemoryPool get_pool(const BigMemoryPool &pool)
{
    return pool;
}
// BigMemoryPool tmp = pool;

BigMemoryPool make_pool()
{
    BigMemoryPool pool;
    return get_pool(pool);
}
//  BigMemoryPool tmp2 = tmp;

int main()
{

    const int x = 10;
    // int y = std::move(x);
    const int &z = x;
    const int &&y = std::move(x);
    &z;
    // x++;
    const int cx = 100;
    const int &&cy = std::move(cx);
    // int &y = get_x();

    // int &&y = x;

    // int *p1 = &x++;
    // int *p2 = &++x;
    auto p3 = &"hello world";
    int &&m = get_x();

    // BigMemoryPool aaa;
    BigMemoryPool ccc = make_pool();
    // BigMemoryPool ccc = tmp2
    // BigMemoryPool &&ddd = std::move(aaa);
    // BigMemoryPool &ccc = aaa;
    int mmm = 10;
    // BigMemoryPool bbb = getPool();
    // BigMemoryPool ccc = std::move(bbb);

    // BigMemoryPool aaa = getPool();
    // BigMemoryPool ccc = getPool(bbb);
}
