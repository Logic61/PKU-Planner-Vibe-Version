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
#include "widgets/onboarding/onboardingdialog.h"
#include "dialogs/taskeditdialog.h"
#include "dialogs/courseeditdialog.h"
#include "models/datamanager.h"
#include "models/course.h"
#include "utils/pageanimator.h"
#include "widgets/mascot/mascotwidget.h"
#include "widgets/dialogs/weeklysummarydialog.h"
#include "services/weeklysummaryservice.h"
#include "services/teachingplatformservice.h"
#include <QInputDialog>
#include "components/toastwidget.h"
#include "dialogs/logindialog.h"
#include "services/configservice.h"
#include "dialogs/confirmdialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>
#include <QApplication>

MainWindow::MainWindow(IConfigProvider *configProvider, QWidget *parent)
    : QMainWindow(parent), m_configProvider(configProvider)
{
    // 设置初始窗口大小
    resize(1200, 800);
    
    // 窗口居中显示
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geom = screen->availableGeometry();
        move(geom.left() + (geom.width() - width()) / 2,
             geom.top() + (geom.height() - height()) / 2);
    }

    QWidget *central = new QWidget;
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);

    sidebar = new SidebarWidget;
    sidebar->setFixedWidth(240);

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
    central->installEventFilter(this);

    searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(searchShortcut, &QShortcut::activated, this, &MainWindow::focusSearch);

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

    dashboardPage = new DashboardPage(m_configProvider);
    todoPage = new TodoPage;
    stack->insertWidget(0, dashboardPage);
    stack->insertWidget(1, todoPage);

    StatsPage *statsPage = new StatsPage;
    stack->insertWidget(2, statsPage);

    SettingsPage *settingsPage = new SettingsPage;
    stack->insertWidget(3, settingsPage);
    connect(settingsPage, &SettingsPage::syncTodosFromTeachingPlatformRequested,
        this, &MainWindow::handleSyncTodosFromTeachingPlatform);

    const auto &courses = DataManager::instance().courses();
    if (courses.isEmpty()) {
        QTimer::singleShot(500, this, [this](){
            OnboardingDialog dlg(this);
            dlg.exec();
        });
    }

    connect(dashboardPage, &DashboardPage::navigateToTodoPageRequested,
            this, &MainWindow::onNavigateToTodoPage);
    connect(dashboardPage, &DashboardPage::importFromTeachingPlatformRequested, this, &MainWindow::handleImportFromTeachingPlatform);
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

    connect(sidebar, &SidebarWidget::connectTeachingPlatformRequested, this, [this](){
        promptTeachingPlatformLogin();
    });

    connect(&DataManager::instance(), &DataManager::tasksChanged, statsPage, &StatsPage::refreshData);

    connect(courseDrawer, &CourseDetailDrawer::addTaskRequested, this, &MainWindow::handleAddTaskRequested);
    connect(courseDrawer, &CourseDetailDrawer::editCourseRequested, this, &MainWindow::handleEditCourseRequested);

    mascotWidget = new MascotWidget(this);
    mascotWidget->raise();
    connect(sidebar, &SidebarWidget::mascotClicked, this, &MainWindow::showMascotPopup);

    if (WeeklySummaryService::shouldShowOnStartup()) {
        QTimer::singleShot(800, this, [](){
            WeeklySummaryDialog dlg;
            dlg.exec();
            WeeklySummaryService::markSummaryShown();
        });
    }

    // Ask user whether to connect to teaching platform on startup
    QTimer::singleShot(1000, this, [this](){
        QMessageBox::StandardButton reply = QMessageBox::question(this, "连接教学网", "是否现在连接教学网？", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            promptTeachingPlatformLogin();
        }
    });
}


void MainWindow::promptTeachingPlatformLogin(bool importCourseAfterLogin, bool syncTasksAfterLogin)
{
    // Use the LoginDialog which supports OTP and remember credentials
    LoginDialog dlg(this);
    // prefill saved credentials if present
    dlg.setUsername(ConfigService::instance().getTeachingUsername());
    dlg.setPassword(ConfigService::instance().getTeachingPassword());

    if (dlg.exec() != QDialog::Accepted) return;

    QString username = dlg.username();
    QString password = dlg.password();
    QString otp = dlg.otp();
    bool remember = dlg.remember();

    if (!teachingService) {
        teachingService = new TeachingPlatformService(this);
    } else {
        QObject::disconnect(teachingService, nullptr, this, nullptr);
    }

    connect(teachingService, &TeachingPlatformService::loginSuccess, this,
            [this, importCourseAfterLogin, syncTasksAfterLogin]() {
        ToastWidget::showToast(this, "已登录教学网", 2000);

        if (importCourseAfterLogin) {
            teachingService->fetchCourseTable();
            return;
        }

        if (syncTasksAfterLogin) {
            teachingService->fetchTodoTasks();
            return;
        }
    });


    connect(teachingService, &TeachingPlatformService::courseTableFetched, this, &MainWindow::handleCourseTableFetched);
    connect(teachingService, &TeachingPlatformService::courseTableFetchFailed, this, &MainWindow::handleCourseTableFetchFailed);
    connect(teachingService, &TeachingPlatformService::loginFailed, this, [this, username, password](const QString &err){
        // if failure indicates OTP required, prompt for OTP and retry
        QString lerr = err.toLower();
        if (lerr.contains("otp") || lerr.contains("e05")) {
            LoginDialog otpDlg(this);
            otpDlg.setUsername(username);
            otpDlg.setPassword(password);
            otpDlg.setOtpVisible(true);
            if (otpDlg.exec() == QDialog::Accepted) {
                QString otp2 = otpDlg.otp();
                teachingService->login(username, password, otp2);
                return;
            }
        }
        QMessageBox::warning(this, "登录失败", QString("登录失败: %1").arg(err));
    });
    connect(teachingService, &TeachingPlatformService::todoAuthRequired, this, [this](const QString &err) {
        LoginDialog dlg(this);
        dlg.setUsername(ConfigService::instance().getTeachingUsername());
        dlg.setPassword(ConfigService::instance().getTeachingPassword());
        dlg.setOtpVisible(true);

        QMessageBox::information(
            this,
            "需要重新登录",
            QString("同步教学网待办需要重新认证：\n%1").arg(err)
        );

        if (dlg.exec() != QDialog::Accepted) {
            return;
        }

        const QString username = dlg.username();
        const QString password = dlg.password();
        const QString otp = dlg.otp();

        if (dlg.remember()) {
            ConfigService::instance().setTeachingUsername(username);
            ConfigService::instance().setTeachingPassword(password);
        }

        teachingService->fetchTodoTasksWithCredentials(username, password, otp);
    });

    connect(teachingService, &TeachingPlatformService::tasksFetched, this, [this](const QList<QJsonObject> &tasks){
        DataManager::instance().updateTasksFromPlatform(tasks);
        ToastWidget::showToast(this, "已同步待办任务", 3000);
    });
    connect(teachingService, &TeachingPlatformService::fetchFailed, this, [this](const QString &err){
        QMessageBox::warning(this, "同步失败", QString("同步任务失败: %1").arg(err));
    });

    // Optionally remember credentials
    if (remember) {
        ConfigService::instance().setTeachingUsername(username);
        ConfigService::instance().setTeachingPassword(password);
    }

    teachingService->login(username, password, otp);
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

void MainWindow::focusSearch()
{
    if (topbar && topbar->getSearchEdit()) {
        topbar->getSearchEdit()->setFocus();
        topbar->getSearchEdit()->selectAll();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && courseDrawer && courseDrawer->isDrawerOpen()) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QWidget *drawer = courseDrawer;
        QWidget *clickedWidget = QApplication::widgetAt(mouseEvent->globalPosition().toPoint());

        bool clickedOnDrawer = false;
        QWidget *w = clickedWidget;
        while (w) {
            if (w == drawer) {
                clickedOnDrawer = true;
                break;
            }
            w = w->parentWidget();
        }

        if (!clickedOnDrawer) {
            courseDrawer->closeDrawer();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}


void MainWindow::handleImportFromTeachingPlatform()
{
    if (!teachingService || !teachingService->hasCredentials()) {
        promptTeachingPlatformLogin(true);
        return;
    }

    teachingService->fetchCourseTable();
}

void MainWindow::handleSyncTodosFromTeachingPlatform()
{
    if (!teachingService || !teachingService->hasCredentials()) {
        promptTeachingPlatformLogin(false, true);
        return;
    }

    teachingService->fetchTodoTasks();
}


void MainWindow::handleCourseTableFetched(const QJsonObject &data)
{
    // parse portal JSON (structure: { "course": [ ... ] })
    if (!data.contains("course") || !data.value("course").isArray()) {
        QMessageBox::warning(this, "导入失败", "课表数据结构不正确");
        return;
    }

    QJsonArray courseSlots = data.value("course").toArray();
    QList<Course> imported;
    QStringList dayKeys = {"mon","tue","wed","thu","fri","sat","sun"};

    for (int i = 0; i < courseSlots.size(); ++i) {
        QJsonValue slotVal = courseSlots.at(i);
        if (!slotVal.isObject()) continue;

        QJsonObject slotObj = slotVal.toObject();
        int slotNum = i + 1; // period index

        for (int d = 0; d < dayKeys.size(); ++d) {
            QString dayKey = dayKeys[d];

            if (!slotObj.contains(dayKey) || !slotObj.value(dayKey).isObject()) continue;

            QJsonObject courseObj = slotObj.value(dayKey).toObject();
            QString rawName = courseObj.value("courseName").toString();

            if (rawName.trimmed().isEmpty()) continue;

            Course c;
            c.name = rawName.split("(主)").first().split("(辅双)").first().trimmed();
            c.day = d + 1;
            c.startPeriod = slotNum;
            c.endPeriod = slotNum;

            // Attempt to extract class info and teacher from rawName
            if (rawName.contains("上课信息：")) {
                int idx = rawName.indexOf("上课信息：") + QString("上课信息：").length();
                QString rest = rawName.mid(idx);

                int teacherPos = rest.indexOf("教师：");
                QString classInfo = teacherPos >= 0 ? rest.left(teacherPos) : rest;

                c.location = classInfo.split('\n').first().trimmed();

                if (teacherPos >= 0) {
                    QString teacherPart = rest.mid(teacherPos + QString("教师：").length());
                    QString teacher = teacherPart.split(' ').first().split('<').first().trimmed();
                    c.teacher = teacher;
                }
            }

            imported.append(c);
        }
    }

    if (imported.isEmpty()) {
        QMessageBox::warning(this, "导入失败", "未解析到有效课程");
        return;
    }

    if (!DataManager::instance().courses().isEmpty()) {
        QMessageBox::StandardButton reply = ConfirmDialog::confirm3(
            this,
            "导入课表",
            "当前已有课程，是否覆盖？\n选择\"是\"将清空现有课程并导入新课表\n选择\"否\"将追加到现有课程",
            "是",
            "否",
            false
        );

        if (reply == QMessageBox::Yes) {
            auto courses = DataManager::instance().courses();
            for (int i = courses.size() - 1; i >= 0; --i) {
                DataManager::instance().deleteCourse(i);
            }
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    for (const Course &c : imported) {
        DataManager::instance().addCourse(c);
    }

    QMessageBox::information(this, "导入成功", QString("成功导入 %1 门课程").arg(imported.size()));
}


void MainWindow::handleCourseTableFetchFailed(const QString &err)
{
    QMessageBox::warning(this, "导入失败", QString("获取课表失败: %1").arg(err));
}