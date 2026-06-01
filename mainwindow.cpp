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
#include <algorithm>
#include "components/toastwidget.h"
#include "dialogs/logindialog.h"
#include "services/configservice.h"
#include "dialogs/confirmdialog.h"
#include "ui/theme.h"
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
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("连接教学网");
        msgBox.setText("是否现在连接教学网？");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setStyleSheet(QString(
            "QMessageBox { background: white; border-radius: %1px; }"
            "QLabel { font-size: 14px; color: %2; padding: 8px; }"
            "QPushButton { "
            "  background: white; color: %2; border: 1px solid %3; "
            "  border-radius: %4px; padding: 10px 24px; font-size: 14px; font-weight: 500; min-width: 80px; "
            "} "
            "QPushButton:hover { background: %3; color: white; }"
        ).arg(Theme::CARD_RADIUS).arg(Theme::TEXT_PRIMARY).arg(Theme::PRIMARY)
         .arg(Theme::BUTTON_RADIUS));
        QMessageBox::StandardButton reply = (QMessageBox::StandardButton)msgBox.exec();
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
        QString lerr = err.toLower();
        bool otpRelated = lerr.contains("otp") || lerr.contains("e05");

        QMessageBox msgBox2(this);
        msgBox2.setWindowTitle("登录失败");
        msgBox2.setText(QString("登录失败: %1%2").arg(err).arg(otpRelated ? "" : "\n\n是否重新输入账号密码？"));
        msgBox2.setIcon(QMessageBox::Warning);
        msgBox2.setStyleSheet(QString(
            "QMessageBox { background: white; border-radius: %1px; }"
            "QLabel { font-size: 14px; color: %2; padding: 8px; }"
            "QPushButton { "
            "  background: white; color: %2; border: 1px solid %3; "
            "  border-radius: %4px; padding: 10px 24px; font-size: 14px; font-weight: 500; min-width: 80px; "
            "} "
            "QPushButton:hover { background: %3; color: white; }"
        ).arg(Theme::CARD_RADIUS).arg(Theme::TEXT_PRIMARY).arg(Theme::PRIMARY)
         .arg(Theme::BUTTON_RADIUS));
        if (!otpRelated) {
            msgBox2.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox2.button(QMessageBox::Yes)->setText("重新输入");
            msgBox2.button(QMessageBox::No)->setText("取消");
        }
        msgBox2.exec();

        if (otpRelated || msgBox2.clickedButton() == msgBox2.button(QMessageBox::Yes)) {
            LoginDialog retryDlg(this);
            retryDlg.setUsername(username);
            retryDlg.setPassword(password);
            retryDlg.setOtpVisible(otpRelated);
            if (retryDlg.exec() == QDialog::Accepted) {
                teachingService->login(retryDlg.username(), retryDlg.password(), retryDlg.otp());
            }
        }
    });
    connect(teachingService, &TeachingPlatformService::todoAuthRequired, this, [this](const QString &err) {
        LoginDialog dlg(this);
        dlg.setUsername(ConfigService::instance().getTeachingUsername());
        dlg.setPassword(ConfigService::instance().getTeachingPassword());
        dlg.setOtpVisible(true);

        QMessageBox infoBox(this);
        infoBox.setWindowTitle("需要重新登录");
        infoBox.setText(QString("同步教学网待办需要重新认证：\n%1").arg(err));
        infoBox.setIcon(QMessageBox::Information);
        infoBox.setStyleSheet(QString(
            "QMessageBox { background: white; border-radius: %1px; }"
            "QLabel { font-size: 14px; color: %2; padding: 8px; }"
            "QPushButton { "
            "  background: white; color: %2; border: 1px solid %3; "
            "  border-radius: %4px; padding: 10px 24px; font-size: 14px; font-weight: 500; min-width: 80px; "
            "} "
            "QPushButton:hover { background: %3; color: white; }"
        ).arg(Theme::CARD_RADIUS).arg(Theme::TEXT_PRIMARY).arg(Theme::PRIMARY)
         .arg(Theme::BUTTON_RADIUS));
        infoBox.exec();

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

void MainWindow::onSearchTaskRequested(const QString& courseAndTitle)
{
    PageAnimator::slideToIndex(stack, 1);
    if (sidebar) {
        sidebar->setActivePage(1);
    }
    if (todoPage) {
        // Find task by course+title and highlight it
        const QList<Task> tasks = DataManager::instance().tasks();
        for (int i = 0; i < tasks.size(); ++i) {
            if (tasks[i].course == courseAndTitle.section("::", 0, 0) &&
                tasks[i].title == courseAndTitle.section("::", 1)) {
                QMetaObject::invokeMethod(todoPage, "highlightTask", Qt::QueuedConnection, Q_ARG(int, i));
                break;
            }
        }
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
    qWarning() << "[CourseImport] Full JSON:" << QJsonDocument(data).toJson(QJsonDocument::Compact);

    // parse portal JSON (structure: { "course": [ ... ] })
    if (!data.contains("course") || !data.value("course").isArray()) {
        QJsonDocument doc(data);
        qWarning() << "[CourseImport] Top-level keys:" << data.keys();
        qWarning() << "[CourseImport] Raw JSON:" << doc.toJson(QJsonDocument::Indented);
        QMessageBox::warning(this, "导入失败", "课表数据结构不正确");
        return;
    }

    QJsonArray courseSlots = data.value("course").toArray();
    QList<Course> imported;
    QStringList dayKeys = {"mon","tue","wed","thu","fri","sat","sun"};
    QStringList dayKeysAlt = {"1","2","3","4","5","6","7"};

    for (int i = 0; i < courseSlots.size(); ++i) {
        QJsonValue slotVal = courseSlots.at(i);
        if (!slotVal.isObject()) {
            qWarning() << "[CourseImport] Slot" << i << "not an object, type:" << slotVal.type();
            continue;
        }

        QJsonObject slotObj = slotVal.toObject();
        int slotNum = i + 1;

        for (int d = 0; d < dayKeys.size(); ++d) {
            QString dayKey = dayKeys[d];

            if (!slotObj.contains(dayKey) || !slotObj.value(dayKey).isObject()) {
                if (slotObj.contains(dayKeysAlt[d]) && slotObj.value(dayKeysAlt[d]).isObject()) {
                    dayKey = dayKeysAlt[d];
                } else {
                    continue;
                }
            }

            QJsonObject courseObj = slotObj.value(dayKey).toObject();
            QString rawName = courseObj.value("courseName").toString();
            if (rawName.trimmed().isEmpty()) {
                QStringList altNames = {"name","course_name","课程名称","kcmc","course","title","班级名称"};
                for (const QString &n : altNames) {
                    rawName = courseObj.value(n).toString().trimmed();
                    if (!rawName.isEmpty()) break;
                }
            }

            if (rawName.trimmed().isEmpty()) {
                qWarning() << "[CourseImport] Slot" << slotNum << dayKey << "empty, obj keys:" << courseObj.keys();
                continue;
            }

            // Clean rawName: strip HTML tags and decode entities
            rawName.replace(QRegularExpression("<[^>]*>"), "");
            rawName.replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">");
            rawName.replace("&quot;", "\"").replace("&#39;", "'").replace("&nbsp;", " ");
            rawName = rawName.simplified();

            qWarning() << "[CourseImport] Slot" << slotNum << dayKey << "cleaned:" << rawName;

            // Also try to strip semester suffix like "（2024-2025学年第二学期）"
            QRegularExpression semRe(R"(\s*[（(][^（）()]*((\d{2,4}\s*[-–—]\s*\d{2,4})|学年|学期|semester|term)[^（）()]*[）)]\s*$)",
                                     QRegularExpression::CaseInsensitiveOption);
            rawName.replace(semRe, "");

            Course c;
            // Parse weekType from rawName before stripping markers
            if (rawName.contains("(辅双)")) {
                c.weekType = 2; // 双周
            } else if (rawName.contains("(辅单)") || rawName.contains("(单)") || rawName.contains("单周")) {
                c.weekType = 1; // 单周
            }
            c.name = rawName.split("(主)").first().split("(辅双)").first().split("(辅单)").first().split("(单)").first().trimmed();
            c.day = d + 1;
            c.startPeriod = slotNum;
            c.endPeriod = slotNum;

            // Attempt to extract class info and teacher from rawName
            if (rawName.contains("上课信息：")) {
                int idx = rawName.indexOf("上课信息：") + QString("上课信息：").length();
                QString rest = rawName.mid(idx);

                int teacherPos = rest.indexOf("教师：");
                QString classInfo = teacherPos >= 0 ? rest.left(teacherPos) : rest;

                // Extract classroom name (e.g., 理教408, 二教405), discarding week range/frequency
                QRegularExpression roomRe(QStringLiteral("([一二三四五]教\\d{2,4}|理教\\d{2,4}|文史(?:楼)?\\d{2,4}|理科教学楼\\d{2,4})"));
                QRegularExpressionMatch rm = roomRe.match(classInfo);
                c.location = rm.hasMatch() ? rm.captured(1) : QString();

                if (teacherPos >= 0) {
                    QString teacherPart = rest.mid(teacherPos + QString("教师：").length());
                    QString teacher = teacherPart.split(' ').first().split('<').first().trimmed();
                    c.teacher = teacher;
                }
            }

            // 解析备注中的习题课/实验课/讨论课（类似图片导入的 AI 行为）
            if (rawName.contains("备注：")) {
                int bzPos = rawName.indexOf("备注：");
                QString remark = rawName.mid(bzPos + 3).trimmed();

                QString suffix;
                if (remark.contains("习题课")) suffix = "习题课";
                else if (remark.contains("实验课")) suffix = "实验课";
                else if (remark.contains("讨论课")) suffix = "讨论课";

                if (!suffix.isEmpty()) {
                    struct { QString name; int val; } dayMap[] = {
                        {"周一",1},{"周二",2},{"周三",3},{"周四",4},{"周五",5},{"周六",6},{"周日",7},
                        {"星期一",1},{"星期二",2},{"星期三",3},{"星期四",4},{"星期五",5},{"星期六",6},{"星期日",7},
                        {"礼拜一",1},{"礼拜二",2},{"礼拜三",3},{"礼拜四",4},{"礼拜五",5},{"礼拜六",6},{"礼拜日",7},
                        {"一",1},{"二",2},{"三",3},{"四",4},{"五",5},{"六",6},{"日",7},
                    };

                    int foundDay = -1;
                    for (const auto &entry : dayMap) {
                        if (remark.contains(entry.name)) {
                            foundDay = entry.val;
                            break;
                        }
                    }

                    int startP = -1, endP = -1;
                    QRegularExpression periodRe(R"((\d+)\s*[-—,、~]\s*(\d+)\s*节)");
                    auto pm = periodRe.match(remark);
                    if (pm.hasMatch()) {
                        startP = pm.captured(1).toInt();
                        endP = pm.captured(2).toInt();
                    }

                    int weekT = 0;
                    if (remark.contains("单周")) weekT = 1;
                    else if (remark.contains("双周")) weekT = 2;

                    if (foundDay > 0 && startP > 0 && endP > 0) {
                        Course extra;
                        extra.name = c.name + suffix;
                        extra.day = foundDay;
                        extra.startPeriod = startP;
                        extra.endPeriod = endP;
                        extra.weekType = weekT;
                        extra.teacher = c.teacher;

                        QString locStr = remark;
                        locStr.replace(QRegularExpression(R"((周|礼拜)[一二三四五六七日])"), "");
                        locStr.replace(QRegularExpression(R"(\d+\s*[-—,、]\s*\d+\s*节)"), "");
                        locStr.replace("单周", "").replace("双周", "").replace("每周", "");
                        locStr.replace(suffix, "");
                        locStr.replace("备注：", "").replace("备注", "");
                        locStr = locStr.trimmed();
                        // 只提取教室名（如二教102、理教207），丢弃其他说明文字
                        if (!locStr.isEmpty()) {
                            QRegularExpression roomRe(QStringLiteral("([一二三四五]教\\d{2,4}|理教\\d{2,4}|文史(?:楼)?\\d{2,4}|理科教学楼\\d{2,4})"));
                            auto rm = roomRe.match(locStr);
                            extra.location = rm.hasMatch() ? rm.captured(1) : QString();
                        }

                        imported.append(extra);
                    }
                }
            }

            imported.append(c);
        }
    }

    // 合并相邻同名课程（教学网返回每节课一个slot，需合并为连续色块）
    std::sort(imported.begin(), imported.end(), [](const Course &a, const Course &b) {
        if (a.day != b.day) return a.day < b.day;
        return a.startPeriod < b.startPeriod;
    });
    QList<Course> merged;
    for (const Course &c : imported) {
        if (!merged.isEmpty()) {
            Course &last = merged.last();
            if (last.day == c.day && last.name == c.name && last.endPeriod + 1 >= c.startPeriod) {
                last.endPeriod = qMax(last.endPeriod, c.endPeriod);
                if (c.teacher.length() > last.teacher.length()) last.teacher = c.teacher;
                if (c.location.length() > last.location.length()) last.location = c.location;
                // 单双周冲突：同一门课既有单周又有双周 → 每周
                if (last.weekType != 0 && c.weekType != 0 && last.weekType != c.weekType) {
                    last.weekType = 0;
                } else if (last.weekType == 0 && c.weekType != 0) {
                    last.weekType = c.weekType;
                }
                continue;
            }
        }
        merged.append(c);
    }
    imported = merged;

    qWarning() << "[CourseImport] Courses after merge:" << imported.size();
    for (const Course &cc : imported) {
        qWarning() << "  " << cc.name << "day=" << cc.day << "period=" << cc.startPeriod << "-" << cc.endPeriod
                 << "loc=" << cc.location << "teacher=" << cc.teacher;
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

    qWarning() << "[CourseImport] Final import list:" << imported.size() << "courses";
    for (const Course &c : imported) {
        DataManager::instance().addCourse(c);
        qWarning() << "[CourseImport] Added:" << c.name << "day=" << c.day << "period=" << c.startPeriod << "-" << c.endPeriod;
    }

    QMessageBox::information(this, "导入成功", QString("成功导入 %1 门课程").arg(imported.size()));
}


void MainWindow::handleCourseTableFetchFailed(const QString &err)
{
    QMessageBox::warning(this, "导入失败", QString("获取课表失败: %1").arg(err));
}