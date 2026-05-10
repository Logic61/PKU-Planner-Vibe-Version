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
    ui/theme.cpp
    ui/theme.h
    pages/dashboardpage.cpp
    pages/dashboardpage.h
    pages/todopage.cpp
    pages/todopage.h
    components/coursecellwidget.cpp
    components/coursecellwidget.h
    components/ddlpreviewwidget.cpp
    components/ddlpreviewwidget.h
    components/taskcardwidget.cpp
    components/taskcardwidget.h
    components/basecardwidget.cpp
    components/basecardwidget.h
    components/toastwidget.cpp
    components/toastwidget.h
    services/reminderservice.cpp
    services/reminderservice.h
    components/emptystatewidget.cpp
    components/emptystatewidget.h
    utils/pageanimator.h
    utils/pageanimator.cpp
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
    widgets/coursedetail/courseheaderwidget.cpp
    widgets/coursedetail/courseheaderwidget.h
    widgets/coursedetail/coursefilepage.cpp
    widgets/coursedetail/coursefilepage.h
    widgets/coursedetail/coursetaskpage.cpp
    widgets/coursedetail/coursetaskpage.h
    widgets/coursedetail/courseinfopage.cpp
    widgets/coursedetail/courseinfopage.h
    widgets/coursedetail/coursetabbar.cpp
    widgets/coursedetail/coursetabbar.h
    widgets/coursedetail/coursedetaildrawer.cpp
    widgets/coursedetail/coursedetaildrawer.h
    widgets/coursedetail/coursestatswidget.cpp
    widgets/coursedetail/coursestatswidget.h
    widgets/mascot/mascotwidget.cpp
    widgets/mascot/mascotwidget.h
    pages/statspage.cpp
    pages/statspage.h
    pages/settingspage.cpp
    pages/settingspage.h
    services/configservice.cpp
    services/configservice.h
    services/mascotstateservice.cpp
    services/mascotstateservice.h
    models/mascotstate.h
    services/dataservice.cpp
    services/dataservice.h
    services/exportservice.cpp
    services/exportservice.h
    services/searchservice.cpp
    services/searchservice.h
    services/weeklysummaryservice.cpp
    services/weeklysummaryservice.h
    dialogs/confirmdialog.cpp
    dialogs/confirmdialog.h
    widgets/search/searchpopup.cpp
    widgets/search/searchpopup.h
    widgets/dialogs/weeklysummarydialog.cpp
    widgets/dialogs/weeklysummarydialog.h
    widgets/onboarding/onboardingdialog.cpp
    widgets/onboarding/onboardingdialog.h
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
