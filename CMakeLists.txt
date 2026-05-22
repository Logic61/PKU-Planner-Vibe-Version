cmake_minimum_required(VERSION 3.16)

project(PKUPlanner VERSION 0.1 LANGUAGES CXX)

# =============================================================================
# Zero-Config Qt Discovery
# cmake/FindQtAuto.cmake scans C:/Qt, D:/Qt, QT_ROOT automatically.
# No manual CMAKE_PREFIX_PATH needed.
# =============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(FindQtAuto)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 为MSVC添加编译选项 - Qt6需要/permissive-
if(MSVC)
    add_compile_options(/permissive- /Zc:__cplusplus /utf-8)
endif()

# 为MinGW添加UTF-8支持
if(MINGW)
    add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8)
endif()

# 确保C++标准符合Qt6要求
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

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
    utils/datetimeutils.h
    dialogs/courseeditdialog.cpp
    dialogs/courseeditdialog.h
    dialogs/courseactiondialog.cpp
    dialogs/courseactiondialog.h
    dialogs/taskeditdialog.cpp
    dialogs/taskeditdialog.h
    models/datamanager.cpp
    models/datamanager.h
    models/datastore.cpp
    models/datastore.h
    models/datarepository.cpp
    models/datarepository.h
    models/taskmodel.cpp
    models/taskmodel.h
    models/task.h
    models/course.h
    models/searchresult.h
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
    services/exportservice.cpp
    services/exportservice.h
    services/weeklysummaryservice.cpp
    services/weeklysummaryservice.h
    dialogs/confirmdialog.cpp
    dialogs/confirmdialog.h
    dialogs/logindialog.cpp
    dialogs/logindialog.h
    widgets/search/searchpopup.cpp
    widgets/search/searchpopup.h
    widgets/dialogs/weeklysummarydialog.cpp
    widgets/dialogs/weeklysummarydialog.h
    widgets/onboarding/onboardingdialog.cpp
    widgets/onboarding/onboardingdialog.h
    services/teachingplatformservice.cpp
    services/teachingplatformservice.h
)

qt_add_executable(PKUPlanner
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(PKUPlanner PRIVATE Qt6::Widgets Qt6::Network)

include(Qt6Deployment OPTIONAL RESULT_VARIABLE _has_deploy)
if(_has_deploy)
    qt_deploy_executable(PKUPlanner)
else()
    add_custom_command(TARGET PKUPlanner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env "PATH=$ENV{PATH}" "${WINDEPLOYQT_EXECUTABLE}" --dir "$<TARGET_FILE_DIR:PKUPlanner>" "$<TARGET_FILE:PKUPlanner>"
        COMMENT "Deploying Qt runtime"
    )
endif()

qt_finalize_executable(PKUPlanner)
