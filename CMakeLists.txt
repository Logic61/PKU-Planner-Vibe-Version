cmake_minimum_required(VERSION 3.16)

project(PKUPlanner VERSION 0.1 LANGUAGES CXX)

# 精准指向Qt6安装路径
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.0/mingw_64")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 为MSVC添加 /Zc:__cplusplus 选项
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(QT_DEPLOY_BIN_DIR "C:/Qt/6.10.0/mingw_64/bin")
find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt6
    HINTS "${QT_DEPLOY_BIN_DIR}"
    REQUIRED
)

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
    dialogs/courseeditdialog.cpp
    dialogs/courseeditdialog.h
    dialogs/courseactiondialog.cpp
    dialogs/courseactiondialog.h
    dialogs/taskeditdialog.cpp
    dialogs/taskeditdialog.h
    models/datamanager.cpp
    models/datamanager.h
    models/taskmodel.cpp
    models/taskmodel.h
    models/task.h
    models/course.h
)

qt_add_executable(PKUPlanner
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(PKUPlanner PRIVATE Qt6::Widgets)

add_custom_command(TARGET PKUPlanner POST_BUILD
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --dir "$<TARGET_FILE_DIR:PKUPlanner>" "$<TARGET_FILE:PKUPlanner>"
    COMMENT "Deploying Qt runtime for PKUPlanner"
)

qt_finalize_executable(PKUPlanner)
