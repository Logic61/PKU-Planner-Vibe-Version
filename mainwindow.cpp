#include "mainwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QWidget>
#include <QTimer>
#include <QMessageBox>

#include "ui/sidebarwidget.h"
#include "ui/topbarwidget.h"

#include "pages/dashboardpage.h"
#include "pages/todopage.h"
#include "widgets/coursedetail/coursedetaildrawer.h"
#include "pages/statspage.h"
#include "pages/settingspage.h"
#include "dialogs/taskeditdialog.h"
#include "dialogs/courseeditdialog.h"
#include "models/datamanager.h"
#include "utils/pageanimator.h"
#include "widgets/mascot/mascotwidget.h"
#include "widgets/dialogs/weeklysummarydialog.h"
#include "services/weeklysummaryservice.h"
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget;
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);

    sidebar = new SidebarWidget;
    sidebar->setFixedWidth(200);

    QWidget *right = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0,0,0,0);

    topbar = new TopbarWidget;

    stack = new QStackedWidget;

    stack->addWidget(new QWidget());
    stack->addWidget(new QWidget());
    stack->addWidget(new QWidget());
    stack->addWidget(new QWidget());

    rightLayout->addWidget(topbar);
    rightLayout->addWidget(stack);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(right);

    courseDrawer = new CourseDetailDrawer(central);
    courseDrawer->hide();

    connect(topbar, &TopbarWidget::searchCourseRequested, this, &MainWindow::onSearchCourseRequested);
    connect(topbar, &TopbarWidget::searchTaskRequested, this, &MainWindow::onSearchTaskRequested);

    QMetaObject::invokeMethod(this, "initPages", Qt::QueuedConnection);
}

void MainWindow::initPages()
{
    if (pagesInitialized) return;
    pagesInitialized = true;

    QWidget *oldPage0 = stack->widget(0);
    QWidget *oldPage1 = stack->widget(1);
    QWidget *oldPage2 = stack->widget(2);
    QWidget *oldPage3 = stack->widget(3);
    stack->removeWidget(oldPage0);
    stack->removeWidget(oldPage1);
    stack->removeWidget(oldPage2);
    stack->removeWidget(oldPage3);
    delete oldPage0;
    delete oldPage1;
    delete oldPage2;
    delete oldPage3;

    dashboardPage = new DashboardPage;
    todoPage = new TodoPage;
    stack->insertWidget(0, dashboardPage);
    stack->insertWidget(1, todoPage);

    StatsPage *statsPage = new StatsPage;
    stack->insertWidget(2, statsPage);

    SettingsPage *settingsPage = new SettingsPage;
    stack->insertWidget(3, settingsPage);

    const auto &courses = DataManager::instance().courses();
    if (courses.isEmpty()) {
        QTimer::singleShot(500, this, [](){
            QMessageBox welcome(nullptr);
            welcome.setWindowTitle("欢迎使用 Course Helper");
            welcome.setText("欢迎使用课程管理助手！\n\n点击课程表空白处添加您的第一门课程，开始管理您的学习吧。");
            welcome.setIcon(QMessageBox::Information);
            welcome.exec();
        });
    }

    connect(dashboardPage, &DashboardPage::navigateToTodoPageRequested,
            this, &MainWindow::onNavigateToTodoPage);
    connect(dashboardPage, &DashboardPage::openCourseDetail,
            this, &MainWindow::showCourseDrawer);
    connect(courseDrawer, &CourseDetailDrawer::courseUpdated,
            dashboardPage, &DashboardPage::applyCourseUpdate);
    connect(courseDrawer, &CourseDetailDrawer::taskUpdated,
            todoPage, &TodoPage::reloadTasks);
    connect(courseDrawer, &CourseDetailDrawer::taskUpdated,
            dashboardPage, &DashboardPage::refreshCourseUrgency);

    connect(sidebar, &SidebarWidget::pageChanged, this, [this, statsPage](int index){
        if (index >= 0 && index < stack->count()) {
            PageAnimator::slideToIndex(stack, index);
        }
        if (index == 2) {
            statsPage->refreshData();
        }
    });

    connect(&DataManager::instance(), &DataManager::tasksChanged, statsPage, &StatsPage::refreshData);

    connect(courseDrawer, &CourseDetailDrawer::addTaskRequested, this, &MainWindow::handleAddTaskRequested);
    connect(courseDrawer, &CourseDetailDrawer::editCourseRequested, this, &MainWindow::handleEditCourseRequested);

    mascotWidget = new MascotWidget(this);
    connect(sidebar, &SidebarWidget::mascotClicked, this, &MainWindow::showMascotPopup);

    if (WeeklySummaryService::shouldShowOnStartup()) {
        QTimer::singleShot(800, this, [](){
            WeeklySummaryDialog dlg;
            dlg.exec();
            WeeklySummaryService::markSummaryShown();
        });
    }
}

void MainWindow::onNavigateToTodoPage()
{
    PageAnimator::slideToIndex(stack, 1);
}

void MainWindow::showCourseDrawer(const Course& course)
{
    if (!courseDrawer) return;
    courseDrawer->loadCourse(course);
    courseDrawer->openDrawer();
}

void MainWindow::handleAddTaskRequested(Course course)
{
    if (todoPage) {
        PageAnimator::slideToIndex(stack, 1);
    }
    TaskEditDialog dlg(this, course.name);
    if (dlg.exec() == QDialog::Accepted) {
        Task t;
        t.course = dlg.getCourseName();
        t.title = dlg.getTitle();
        t.deadline = dlg.getDeadline();
        t.priority = dlg.getPriority();
        t.completed = false;
        DataManager::instance().addTask(t);
    }
}

void MainWindow::handleEditCourseRequested(Course course)
{
    CourseEditDialog dlg(course.startPeriod, course.endPeriod, this);
    dlg.setCourseData(course.name, course.teacher, course.location, course.examTime, course.startPeriod, course.endPeriod, course.weekType);
    if (dlg.exec() == QDialog::Accepted) {
        Course updated = course;
        updated.name = dlg.getName();
        updated.teacher = dlg.getTeacher();
        updated.location = dlg.getLocation();
        updated.examTime = dlg.getExamTime();
        updated.startPeriod = dlg.getStart();
        updated.endPeriod = dlg.getEnd();
        updated.weekType = dlg.getWeekType();

        int foundIndex = -1;
        const QList<Course> all = DataManager::instance().courses();
        for (int i = 0; i < all.size(); ++i) {
            const Course &cc = all[i];
            if (cc.name == course.name && cc.day == course.day && cc.startPeriod == course.startPeriod && cc.endPeriod == course.endPeriod) {
                foundIndex = i;
                break;
            }
        }
        if (foundIndex >= 0) {
            DataManager::instance().updateCourse(foundIndex, updated);
        }
    }
}

void MainWindow::showMascotPopup()
{
    if (!mascotWidget) {
        mascotWidget = new MascotWidget(this);
    }
    mascotWidget->showPopup();
}

void MainWindow::onSearchCourseRequested(const QString& courseName)
{
    searchCourseName = courseName;
    const QList<Course> courses = DataManager::instance().courses();
    for (const Course &c : courses) {
        if (c.name == courseName) {
            showCourseDrawer(c);
            break;
        }
    }
}

void MainWindow::onSearchTaskRequested(int taskIndex)
{
    PageAnimator::slideToIndex(stack, 1);
    if (sidebar) {
        sidebar->setActivePage(1);
    }
    if (todoPage) {
        QMetaObject::invokeMethod(todoPage, "highlightTask", Qt::QueuedConnection, Q_ARG(int, taskIndex));
    }
}