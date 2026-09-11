# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(test)

# 使用 C++11 标准
set(CMAKE_CXX_STANDARD 11)

# 自动处理 Qt 的 Q_OBJECT 宏，生成 moc 代码
set(CMAKE_AUTOMOC ON)

# 自动处理 .ui 文件，生成 ui_widget.h
set(CMAKE_AUTOUIC ON)

# 自动处理 Qt 资源文件（.qrc）
set(CMAKE_AUTORCC ON)

# 指定 .ui 文件的搜索目录
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/forms)

# 查找 Qt5 Widgets 模块
find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

# 生成可执行程序 test，并列出项目源文件
add_executable(test
    main.cpp
    widget/widget.cpp
    widget/widget.h
    forms/widget.ui
)

# 链接 Qt5 Widgets 库
target_link_libraries(test PRIVATE
    Qt5::Widgets
    Qt5::Network
)
