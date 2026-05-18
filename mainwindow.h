#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include <QEvent>

class QStackedWidget;
class SidebarWidget;
class TopbarWidget;
class DashboardPage;
class TodoPage;
class CourseDetailDrawer;
class MascotWidget;
class IConfigProvider;
#include "models/course.h"
#include "services/iconfigprovider.h"
#include <QJsonObject>


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(IConfigProvider *configProvider, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void handleSyncTodosFromTeachingPlatform();

private slots:
    void promptTeachingPlatformLogin(bool importCourseAfterLogin = false, bool syncTasksAfterLogin = false);
    void handleImportFromTeachingPlatform();
    void handleCourseTableFetched(const QJsonObject &data);
    void handleCourseTableFetchFailed(const QString &err);
    void initPages();
    void onNavigateToTodoPage();
    void showCourseDrawer(const Course& course);
    void handleAddTaskRequested(Course course);
    void handleEditCourseRequested(Course course);
    void showMascotPopup();
    void onSearchCourseRequested(const QString& courseName);
    void onSearchTaskRequested(int taskIndex);
    void focusSearch();

private:
    QStackedWidget *stack;
    SidebarWidget *sidebar;
    TopbarWidget *topbar;
    DashboardPage *dashboardPage = nullptr;
    TodoPage *todoPage = nullptr;
    CourseDetailDrawer *courseDrawer = nullptr;
    MascotWidget *mascotWidget = nullptr;
    class TeachingPlatformService *teachingService = nullptr;
    QShortcut *searchShortcut = nullptr;
    bool pagesInitialized = false;
    QString searchCourseName;
    IConfigProvider *m_configProvider = nullptr; // not owned
};

#endif