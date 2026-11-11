#include <iostream>
#include <string>
#include <vector>
// https://blog.csdn.net/weixin_46098577/article/details/116596183
// ============全局函数做友元============
namespace space1
{
    class Building
    {
    public:
        Building(std::string sittingRoom, std::string bedRoom) : m_SittingRoom(sittingRoom), m_BedRoom(bedRoom) {}
        std::string m_SittingRoom;
        friend void laoWang(Building &building);

    private:
        std::string m_BedRoom;
    };

    void laoWang(Building &building)
    {
        std::cout << "global function ======" << building.m_SittingRoom << std::endl;
        std::cout << "global function ======" << building.m_BedRoom << std::endl;
    }
}

// =============类做友元==============
namespace space2
{
    class Building
    {
        friend class LaoWang;

    public:
        Building(std::string sittingRoom, std::string bedRoom) : m_SittingRoom(sittingRoom), m_BedRoom(bedRoom){};
        std::string m_SittingRoom;

    private:
        std::string m_BedRoom;
    };

    class LaoWang
    {
    public:
        LaoWang()
        {
            std::cout << "class ======" << m_building.m_SittingRoom << std::endl;
            std::cout << "class ======" << m_building.m_BedRoom << std::endl;
        }

    private:
        Building m_building{"sr", "br"};
    };

}

// =============成员函数做友元==============
namespace space3
{
    class Building;
    class LaoWang
    {
    public:
        LaoWang();
        void visit();

    private:
        Building *m_building;
    };

    class Building
    {
    public:
        Building(std::string sittingRoom, std::string bedRoom) : m_SittingRoom(sittingRoom), m_BedRoom(bedRoom){};
        std::string m_SittingRoom;

    private:
        friend void LaoWang::visit();
        std::string m_BedRoom;
    };

    LaoWang::LaoWang()
    {
        m_building = new Building("sr", "br");
        std::cout << "class function ======" << m_building->m_SittingRoom << std::endl;
        // std::cout << "class function ======" << m_building->m_BedRoom << std::endl;
    }
    void LaoWang::visit()
    {
        std::cout << "class function ======" << m_building->m_SittingRoom << std::endl;
        std::cout << "class function ======" << m_building->m_BedRoom << std::endl;
    }
}

int main()
{
    space1::Building building("sr", "br");
    space1::laoWang(building);

    // space2::LaoWang();

    // space3::LaoWang laowang;
    // laowang.visit();
    return 0;
}