cmake_minimum_required(VERSION 3.16)

project(PKUPlanner VERSION 0.1 LANGUAGES CXX)

# 精准指向Qt6安装路径
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.0/mingw_64")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    ui/sidebarwidget.cpp
    ui/sidebarwidget.h
    ui/topbarwidget.cpp
    ui/topbarwidget.h
    pages/dashboardpage.cpp
    pages/dashboardpage.h
    pages/todopage.cpp
    pages/todopage.h
    components/coursecellwidget.cpp
    components/coursecellwidget.h
)

qt_add_executable(PKUPlanner
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(PKUPlanner PRIVATE Qt6::Widgets)

set_target_properties(PKUPlanner PROPERTIES
    WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(PKUPlanner)