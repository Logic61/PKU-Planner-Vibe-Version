#include "dashboardpage.h"
#include "../ui/theme.h"
#include "../ui/sidebarwidget.h"
#include "../components/coursecellwidget.h"
#include "../components/ddlpreviewwidget.h"
#include "../components/toastwidget.h"
#include "../models/datamanager.h"
#include "../models/course.h"
#include "../models/task.h"
#include "../services/reminderservice.h"
#include "../services/configservice.h"
#include "../widgets/coursedetail/coursedetaildrawer.h"
#include "../widgets/search/searchpopup.h"
#include "../dialogs/taskeditdialog.h"
#include "../dialogs/courseeditdialog.h"
#include "../dialogs/courseactiondialog.h"
#include "../dialogs/confirmdialog.h"
#include "../utils/datetimeutils.h"

#include <QStackedWidget>
#include <QInputDialog>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QBuffer>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDate>
#include <QDebug>
#include <QTimer>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QParallelAnimationGroup>
#include <QGraphicsDropShadowEffect>
#include <QInputDialog>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QFont>
#include <QProgressBar>
#include <QCheckBox>

namespace {
using DateTimeUtils::dayText;
using DateTimeUtils::scheduleLine;
using DateTimeUtils::scheduleSummaryForGroup;
}

namespace {
void applyShadow(QWidget* w)
{
    auto* shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 20));
    w->setGraphicsEffect(shadow);
}
}

DashboardPage::DashboardPage(IConfigProvider *configProvider, QWidget *parent)
    : QWidget(parent), m_configProvider(configProvider)
{
    // ===== 创建滚动区域 =====
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background:transparent;");

    // 原 this 上的内容改为放到 container 里
    QWidget *container = new QWidget();
    container->setStyleSheet("background:#F7F3EF; font-family: 'Microsoft YaHei','Segoe UI', Arial; color: #222; font-weight:500;");

    // 所有原有布局和控件都添加到 container 上
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    // === CRITICAL: Initialize ddlLayout BEFORE any code that uses it ===
    // Create a temporary empty layout to prevent nullptr access in updateDDLWidget()
    ddlLayout = new QVBoxLayout;

    // === CRITICAL: Create gridContainer as class member before createTopBar() which calls updateWeekInfo() which calls renderCourses() ===
    gridContainer = new QWidget;
    grid = new QGridLayout(gridContainer);
    grid->setSpacing(6);

    // ===== 1. 顶部学期控制栏 =====
    mainLayout->addWidget(createTopBar());

    // ===== 2. 主体区域 =====
    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(12);

    // 左：课程表
    QFrame *courseCard = new QFrame;
    courseCard->setStyleSheet("background:white; border-radius:20px;");
    applyShadow(courseCard);
    QVBoxLayout *courseLayout = new QVBoxLayout(courseCard);

    QWidget *courseHeader = new QWidget;
    QHBoxLayout *headerLayout = new QHBoxLayout(courseHeader);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *courseTitle = new QLabel("本周课程安排");
    courseTitle->setStyleSheet("font-weight:700; font-size:16px; color:#222;");

    QPushButton *addCourseBtn = new QPushButton("+ 添加课程");
    addCourseBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: %2;
        }
    )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));

    QPushButton *importBtn = new QPushButton("导入课表");
    importBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: white;
            color: %1;
            border: 1px solid %1;
            border-radius: 10px;
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: %2;
            color: white;
        }
    )").arg(Theme::PRIMARY).arg(Theme::PRIMARY));

    headerLayout->addWidget(courseTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(addCourseBtn);
    headerLayout->addWidget(importBtn);

    connect(importBtn, &QPushButton::clicked, this, &DashboardPage::importSchedule);
    connect(addCourseBtn, &QPushButton::clicked, this, [this](){
        QDialog *dlg = new QDialog(this);
        dlg->setWindowTitle("添加课程");
        dlg->setWindowFlags(dlg->windowFlags() & ~Qt::WindowContextHelpButtonHint);
        dlg->setStyleSheet("QDialog { background: white; }");

        QVBoxLayout *layout = new QVBoxLayout(dlg);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(16);

        QLabel *icon = new QLabel("💡");
        icon->setAlignment(Qt::AlignCenter);
        icon->setStyleSheet("font-size: 40px;");
        layout->addWidget(icon);

        QLabel *titleLabel = new QLabel("如何添加课程？");
        titleLabel->setStyleSheet("font-size: 18px; font-weight: 700; color: #222;");
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        QLabel *descLabel = new QLabel("双击课表中的灰色时间块即可添加课程。");
        descLabel->setStyleSheet("font-size: 14px; color: #666;");
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setWordWrap(true);
        layout->addWidget(descLabel);

        layout->addStretch();

        QPushButton *btn = new QPushButton("我知道了");
        btn->setStyleSheet(QString(R"(
            QPushButton {
                background: %1;
                color: white;
                border: none;
                border-radius: 8px;
                padding: 10px 24px;
                font-size: 14px;
                font-weight: 600;
            }
            QPushButton:hover {
                background: %2;
            }
        )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));
        connect(btn, &QPushButton::clicked, dlg, &QDialog::accept);
        layout->addWidget(btn, 0, Qt::AlignCenter);

        dlg->setMinimumWidth(280);
        dlg->exec();
    });

    courseLayout->addWidget(courseHeader);
    courseLayout->addWidget(gridContainer);

    contentLayout->addWidget(courseCard, 7); // 70%

    // 右：侧栏
    QWidget *rightPanelWidget = createRightPanel();
    QFrame *rightPanelFrame = new QFrame;
    rightPanelFrame->setStyleSheet("background:white; border-radius:20px;");
    applyShadow(rightPanelFrame);
    QVBoxLayout *rightFrameLayout = new QVBoxLayout(rightPanelFrame);
    rightFrameLayout->setContentsMargins(0,0,0,0);
    rightFrameLayout->addWidget(rightPanelWidget);
    contentLayout->addWidget(rightPanelFrame, 3); // 30%

    mainLayout->addLayout(contentLayout);

    // ===== 3. 底部统计卡片 =====
    QFrame *statsCard = new QFrame;
    statsCard->setStyleSheet("background:white; border-radius:20px;");
    applyShadow(statsCard);
    QVBoxLayout *statsLayout = new QVBoxLayout(statsCard);
    statsLayout->setContentsMargins(16,16,16,16);
    statsLayout->addWidget(createBottomStats());
    mainLayout->addWidget(statsCard);

    // 将 container 设置为滚动区域的内容
    scrollArea->setWidget(container);

    // DashboardPage 自身的布局只放 scrollArea
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    // Connect signals for reactive updates
    connect(&DataManager::instance(), &DataManager::coursesChanged, this, &DashboardPage::renderCourses);
    connect(&DataManager::instance(), &DataManager::tasksChanged, this, &DashboardPage::renderCourses);
    connect(&DataManager::instance(), &DataManager::coursesChanged, this, &DashboardPage::updateBottomStats);
    connect(&DataManager::instance(), &DataManager::tasksChanged, this, &DashboardPage::updateBottomStats);
    connect(&DataManager::instance(), &DataManager::tasksChanged, this, &DashboardPage::updateDDLWidget);

    // Connect to config provider for semester changes
    connect(&ConfigService::instance(), &ConfigService::configChanged, this, [this](){ updateWeekInfo(false); });

    // Initial week info from config provider
    realWeek = m_configProvider->getCurrentWeek();
    updateWeekInfo(false);
}

QWidget* DashboardPage::createTopBar()
{
    QWidget *bar = new QWidget;
    bar->setStyleSheet("background:white; border-radius:16px; padding:8px;");

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(8,8,8,8);

    // 学期进度
    semesterProgress = new QProgressBar;
    semesterProgress->setRange(0,18);
    semesterProgress->setValue(currentWeek);
    semesterProgress->setTextVisible(false);

    semesterProgress->setStyleSheet(QString(R"(
        QProgressBar {
            border:none;
            background:#F5F5F5;
            height:16px;
            border-radius:8px;
        }

        QProgressBar::chunk {
            background:%1;
            border-radius:8px;
        }
    )").arg(Theme::PRIMARY));

    weekLabel = new QLabel;
    weekLabel->setStyleSheet("font-weight:700; font-size:14px; color:#222;");

    QPushButton *prevBtn = new QPushButton("上周");
    QPushButton *nextBtn = new QPushButton("下周");
    QPushButton *todayBtn = new QPushButton("返回本周");

    layout->addWidget(new QLabel("学期进度"), 0);
    layout->addWidget(semesterProgress, 2);
    layout->addWidget(weekLabel, 0);
    layout->addWidget(prevBtn, 0);
    layout->addWidget(nextBtn, 0);
    layout->addWidget(todayBtn, 0);

    connect(prevBtn,&QPushButton::clicked,this,[=](){
        currentWeek--;
        updateWeekInfo(false);
    });

    connect(nextBtn,&QPushButton::clicked,this,[=](){
        currentWeek++;
        updateWeekInfo(false);
    });

    connect(todayBtn,&QPushButton::clicked,this,[=](){
        currentWeek = realWeek;
        updateWeekInfo(false);
    });

    updateWeekInfo(false);

    return bar;
}

void DashboardPage::updateWeekInfo(bool useCurrentWeek)
{
    if (useCurrentWeek) {
        currentWeek = ConfigService::instance().getCurrentWeek();
    }

    int maxWeek = 18;
    QDate startDate = m_configProvider->getSemesterStart();
    QDate endDate = m_configProvider->getSemesterEnd();
    if (startDate.isValid() && endDate.isValid()) {
        maxWeek = qMax(1, startDate.daysTo(endDate) / 7);
    }

    if(currentWeek < 1) currentWeek = 1;
    if(currentWeek > maxWeek) currentWeek = maxWeek;

    semesterProgress->setRange(0, maxWeek);
    semesterProgress->setValue(currentWeek);

    QString type = (currentWeek % 2 == 1) ? "单周" : "双周";

    weekLabel->setText(
        QString("第 %1 周 (%2)")
            .arg(currentWeek)
            .arg(type)
    );
    updateBottomStats();
    renderCourses();
}

QWidget* DashboardPage::createBottomStats()
{
    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(12);

    QStringList icons = {"📚", "📝", "📅", "⏰"};
    QStringList titles = {
        "今日课程",
        "今日DDL",
        "本周DDL",
        "当前时间"
    };

    for(int i = 0; i < titles.size(); i++)
    {
        QString t = titles[i];
        QString icon = icons[i];

        QFrame *card = new QFrame(widget);
        card->setMinimumHeight(120);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        card->setStyleSheet("QFrame{background:white; border-radius:20px;}");

        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(10,10,10,10);
        cl->setSpacing(4);
        cl->addStretch();

        QLabel *iconLabel = new QLabel(icon, card);
        iconLabel->setStyleSheet("font-size:24px; color:#222; background:transparent;");
        iconLabel->setAlignment(Qt::AlignCenter);
        cl->addWidget(iconLabel);

        QLabel *num = new QLabel("0", card);
        num->setStyleSheet(QString("font-size:32px; font-weight:700; color:%1; background:transparent;").arg(Theme::PRIMARY));
        num->setAlignment(Qt::AlignCenter);
        cl->addWidget(num);

        QLabel *title = new QLabel(t, card);
        title->setStyleSheet("font-size:14px; color:#888; background:transparent;");
        title->setAlignment(Qt::AlignCenter);
        cl->addWidget(title);

        cl->addStretch();

        if(t == "今日课程") todayCourseValue = num;
        else if(t == "今日DDL") todayDdlValue = num;
        else if(t == "本周DDL") weekDdlValue = num;
        else if(t == "当前时间") {
            timeLabel = num;
            timeLabel->setStyleSheet(QString("font-size:18px; font-weight:700; color:%1; background:transparent;").arg(Theme::PRIMARY));
            QTimer *timer = new QTimer(this);
            connect(timer,&QTimer::timeout,this,[=](){
                timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd\nhh:mm:ss"));
                updateTodayCourses();
            });
            timer->start(1000);
            timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd\nhh:mm:ss"));
        }

        layout->addWidget(card);
    }

    updateBottomStats();
    return widget;
}

void DashboardPage::updateBottomStats()
{
    qDebug() << "[DashboardPage::updateBottomStats] Called";
    const QList<Course> courses = DataManager::instance().courses();
    const QList<Task> tasks = DataManager::instance().tasks();
    const QDate today = QDate::currentDate();
    const int currentWeekday = today.dayOfWeek();

    int currentYear = 0;
    const int displayWeek = currentWeek;
    const bool isSingle = displayWeek % 2 == 1;
    const int displayWeekday = (isSingle || displayWeek == realWeek) ? today.dayOfWeek() : 1;

    int todayCourseCount = 0;
    for (const Course &course : courses) {
        if (course.day != displayWeekday) {
            continue;
        }
        if (course.weekType == 1 && displayWeek % 2 == 0) {
            continue;
        }
        if (course.weekType == 2 && displayWeek % 2 == 1) {
            continue;
        }
        ++todayCourseCount;
    }

    int todayDdlCount = 0;
    int weekDdlCount = 0;
    for (const Task &task : tasks) {
        qDebug() << "[updateBottomStats] Task:" << task.title << "completed:" << task.completed << "deadline valid:" << task.deadline.isValid();
        if (task.completed || !task.deadline.isValid()) {
            continue;
        }

        const QDate deadlineDate = task.deadline.date();
        if (deadlineDate == today) {
            ++todayDdlCount;
        }

        int deadlineYear = 0;
        const int deadlineWeek = deadlineDate.weekNumber(&deadlineYear);
        if (displayWeek == deadlineWeek && currentYear == deadlineYear) {
            ++weekDdlCount;
        }
    }

    qDebug() << "[updateBottomStats] Results - today courses:" << todayCourseCount
             << "today DDL:" << todayDdlCount << "week DDL:" << weekDdlCount;

    if (todayCourseValue) {
        todayCourseValue->setText(QString::number(todayCourseCount));
    }
    if (todayDdlValue) {
        todayDdlValue->setText(QString::number(todayDdlCount));
    }
    if (weekDdlValue) {
        weekDdlValue->setText(QString::number(weekDdlCount));
    }

    updateTodayCourses();
}

void DashboardPage::updateTodayCourses()
{
    if (!todayCourseLayout) return;

    QLayoutItem *child;
    while ((child = todayCourseLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    const QList<Course> courses = DataManager::instance().courses();
    const QDate today = QDate::currentDate();
    const int displayWeek = currentWeek;
    const bool isSingle = displayWeek % 2 == 1;
    const int displayWeekday = (isSingle || displayWeek == realWeek) ? today.dayOfWeek() : 1;

    const QTime now = QTime::currentTime();

    struct CourseInfo {
        int period;
        QString name;
        QString location;
    };
    QVector<CourseInfo> todayCourses;

    for (const Course &course : courses) {
        if (course.day != displayWeekday) continue;
        if (course.weekType == 1 && displayWeek % 2 == 0) continue;
        if (course.weekType == 2 && displayWeek % 2 == 1) continue;
        if (course.startPeriod <= 0) continue;

        todayCourses.append({course.startPeriod, course.name, course.location});
    }

    std::sort(todayCourses.begin(), todayCourses.end(), [](const CourseInfo& a, const CourseInfo& b) {
        return a.period < b.period;
    });

    if (todayCourses.isEmpty()) {
        QLabel *empty = new QLabel("🎉 今日无课\n好好休息一下");
        empty->setStyleSheet("color:#999; font-size:13px;");
        empty->setAlignment(Qt::AlignCenter);
        todayCourseLayout->addWidget(empty);
        return;
    }

    QVector<QPair<int, int>> periodTimes = {
        {8, 0},   {9, 0},   {10, 10}, {11, 10},
        {13, 0},  {14, 0},  {15, 10}, {16, 10},
        {17, 10}, {18, 40}, {19, 40}, {20, 40}
    };

    for (const auto& course : todayCourses) {
        int period = course.period;
        if (period < 1 || period > 12) continue;

        QTime startTime(periodTimes[period-1].first, periodTimes[period-1].second);
        QTime endTime = startTime.addSecs(50 * 60);

        QString stateText;
        QString color;

        if (now < startTime) {
            color = "#222";
            stateText = "第" + QString::number(period) + "节 " + course.name;
        } else if (now >= startTime && now < endTime) {
            color = "#F44336";
            stateText = "🔴 第" + QString::number(period) + "节 " + course.name;
        } else {
            color = "#999";
            stateText = "✓ 第" + QString::number(period) + "节 " + course.name;
        }

        QLabel *courseLabel = new QLabel(stateText);
        courseLabel->setStyleSheet(QString("color:%1; font-size:13px; padding:4px 0;").arg(color));
        courseLabel->setWordWrap(true);
        todayCourseLayout->addWidget(courseLabel);
    }
}

QWidget* DashboardPage::createRightPanel()
{
    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(12);

    // 今日课程卡
    QFrame *todayCard = new QFrame;
    todayCard->setStyleSheet("background:white; border-radius:20px;");
    QVBoxLayout *todayLayout = new QVBoxLayout(todayCard);

    QLabel *todayTitle = new QLabel("今日课程");
    todayTitle->setStyleSheet("font-weight:700; font-size:14px; color:#222;");
    todayLayout->addWidget(todayTitle);

    todayCourseLayout = new QVBoxLayout;
    todayCourseLayout->setSpacing(8);
    todayLayout->addLayout(todayCourseLayout);

    QLabel *todayContent = new QLabel("🎉 今日无课\n好好休息一下");
    todayContent->setStyleSheet("color:#999; font-size:13px;");
    todayContent->setAlignment(Qt::AlignCenter);
    todayContent->setObjectName("todayContentPlaceholder");
    todayCourseLayout->addWidget(todayContent);

    updateTodayCourses();

    // DDL摘要卡
    QFrame *ddlCard = new QFrame;
    ddlCard->setStyleSheet("background:white; border-radius:20px;");
    ddlLayout = new QVBoxLayout(ddlCard);
    QLabel *ddlTitle = new QLabel("DDL提醒");
    ddlTitle->setStyleSheet("font-weight:700; font-size:16px; margin-bottom: 8px; color:#222;");
    ddlLayout->addWidget(ddlTitle);
    updateDDLWidget();

    layout->addWidget(todayCard);
    layout->addWidget(ddlCard);
    layout->addWidget(createSuggestionCard());
    layout->addStretch();

    return widget;
}

void DashboardPage::updateDDLWidget()
{
    // Clear old items except title
    while (QLayoutItem *child = ddlLayout->takeAt(1)) {
        if(child->widget()) delete child->widget();
        delete child;
    }
    bool hasDDL = false;

    const auto tasks = DataManager::instance().tasks();
    const QDateTime now = QDateTime::currentDateTime();
    std::vector<int> taskIndices;

    for (int i = 0; i < tasks.size(); ++i) {
        const Task &task = tasks[i];
        if (task.completed) {
            continue;
        }

        taskIndices.push_back(i);
    }

    std::sort(taskIndices.begin(), taskIndices.end(), [&tasks](int left, int right) {
        const Task &a = tasks[left];
        const Task &b = tasks[right];
        if (a.deadline != b.deadline) {
            return a.deadline < b.deadline;
        }
        // If deadlines are the same, sort by priority (higher priority first)
        if (a.priority != b.priority) {
            return a.priority > b.priority;
        }
        return a.title < b.title;
    });

    int count = 0;
    for (int taskIndex : taskIndices) {
        const Task &task = tasks[taskIndex];
        const int daysLeft = now.daysTo(task.deadline);
        if (count >= 5) {
            break;
        }

        QFrame *itemFrame = new QFrame;
        itemFrame->setStyleSheet(R"(
            QFrame {
                background:#F8F8F8;
                border-radius:14px;
            }
        )");
        QVBoxLayout *vl = new QVBoxLayout(itemFrame);
        vl->setContentsMargins(12,12,12,12);
        vl->setSpacing(8);

        QHBoxLayout *topRow = new QHBoxLayout;
        topRow->setContentsMargins(0,0,0,0);
        topRow->setSpacing(8);

        QCheckBox *doneBox = new QCheckBox;
        doneBox->setChecked(task.completed);
        doneBox->setStyleSheet(QString(R"(
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
                border-radius: 9px;
                border: 1px solid %1;
                background: white;
            }
            QCheckBox::indicator:checked {
                background: %1;
                border: 1px solid %1;
            }
        )").arg(Theme::PRIMARY));

        QLabel *nameLbl = new QLabel(task.course + " - " + task.title);
        nameLbl->setStyleSheet("font-weight:600; font-size:13px; color:#222;");
        nameLbl->setWordWrap(true);

        topRow->addWidget(doneBox);
        topRow->addWidget(nameLbl, 1);

        QHBoxLayout *metaRow = new QHBoxLayout;
        metaRow->setContentsMargins(0,0,0,0);
        metaRow->setSpacing(8);

        QLabel *timeLbl = new QLabel;
        if (daysLeft < 0) {
            timeLbl->setText("已逾期");
            timeLbl->setStyleSheet(QString("color:%1; font-size:12px; font-weight:600;").arg(Theme::DANGER));
        } else if (daysLeft == 0) {
            timeLbl->setText("今晚截止");
            timeLbl->setStyleSheet("color:#E64A19; font-size:12px; font-weight:600;");
        } else {
            timeLbl->setText(QString("剩余 %1 天").arg(daysLeft));
            timeLbl->setStyleSheet(QString("color:%1; font-size:12px;").arg(Theme::PRIMARY));
        }

        QLabel *courseLbl = new QLabel(task.course);
        courseLbl->setStyleSheet("color:#777; font-size:12px;");

        QPushButton *editBtn = new QPushButton("编辑");
        QPushButton *deleteBtn = new QPushButton("删除");
        for (QPushButton *btn : {editBtn, deleteBtn}) {
            btn->setCursor(Qt::PointingHandCursor);
            btn->setMinimumHeight(30);
            btn->setStyleSheet(QString(R"(
                QPushButton {
                    background: transparent;
                    color: %1;
                    border: 1px solid #E2C9CD;
                    border-radius: 8px;
                    padding: 6px 12px;
                    font-weight:600;
                }
                QPushButton:hover {
                    background: %1;
                    color: white;
                }
            )").arg(Theme::PRIMARY));
        }

        metaRow->addWidget(courseLbl);
        metaRow->addWidget(timeLbl);
        metaRow->addStretch();
        metaRow->addWidget(editBtn);
        metaRow->addWidget(deleteBtn);

        vl->addLayout(topRow);
        vl->addLayout(metaRow);

        connect(doneBox, &QCheckBox::toggled, this, [taskIndex](bool checked) {
            QTimer::singleShot(0, qApp, [taskIndex, checked]() {
                DataManager::instance().markTaskCompleted(taskIndex, checked);
            });
        });

        connect(editBtn, &QPushButton::clicked, this, [this, taskIndex]() {
            const QList<Task> tasks = DataManager::instance().tasks();
            if (taskIndex < 0 || taskIndex >= tasks.size()) {
                return;
            }

            const Task task = tasks[taskIndex];
            TaskEditDialog dialog(this, task.course);
            dialog.setWindowTitle("编辑DDL任务");
            dialog.setTaskData(task);

            if (dialog.exec() == QDialog::Accepted) {
                Task updated = task;
                updated.course = dialog.getCourseName();
                updated.title = dialog.getTitle();
                updated.deadline = dialog.getDeadline();
                updated.priority = dialog.getPriority();
                updated.completed = dialog.getCompleted();
                DataManager::instance().updateTask(taskIndex, updated);
            }
        });

        connect(deleteBtn, &QPushButton::clicked, this, [taskIndex]() {
            QTimer::singleShot(0, qApp, [taskIndex]() {
                DataManager::instance().deleteTask(taskIndex);
            });
        });

        if (task.completed) {
            itemFrame->setStyleSheet(R"(
                QFrame {
                    background:#F5F5F5;
                    border-radius:14px;
                }
            )");
        }

        ddlLayout->addWidget(itemFrame);
        hasDDL = true;
        ++count;
    }
    
    if (!hasDDL) {
        QLabel *empty = new QLabel("✅ 暂无DDL");
        empty->setStyleSheet("color:#999; font-size:13px;");
        ddlLayout->addWidget(empty);
    }
}

void DashboardPage::initGrid()
{
    QStringList days = {"一","二","三","四","五","六","日"};

    // 星期
    for(int col=0; col<7; col++)
    {
        QFrame *dayFrame = new QFrame;
        dayFrame->setStyleSheet("background:#F8F8F8; border-radius:10px;");
        dayFrame->setFixedHeight(36);
        QVBoxLayout *dayLayout = new QVBoxLayout(dayFrame);
        dayLayout->setContentsMargins(0,0,0,0);
        dayLayout->setSpacing(0);

        QLabel *label = new QLabel("周" + days[col]);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-weight:700; color:#444; font-size:12px;");
        dayLayout->addWidget(label);

        grid->addWidget(dayFrame, 0, col+1);
    }

// 时间（节数 + 时间段）
QStringList timeSlots = {
    "8:00-8:50",    // 第1节
    "9:00-9:50",    // 第2节
    "10:10-11:00",  // 第3节
    "11:10-12:00",  // 第4节
    "13:00-13:50",  // 第5节
    "14:00-14:50",  // 第6节
    "15:10-16:00",  // 第7节
    "16:10-17:00",  // 第8节
    "17:10-18:00",  // 第9节
    "18:40-19:30",  // 第10节
    "19:40-20:30",  // 第11节
    "20:40-21:30"   // 第12节
};

for(int row=0; row<12; row++)
{
    QLabel *timeLabel = new QLabel(QString("%1\n%2").arg(row+1).arg(timeSlots[row]));
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setStyleSheet("color:#aaa; font-size:10px; line-height:1.2;");
    grid->addWidget(timeLabel, row+1, 0);
}

    // Initialize all cells as empty first to provide a clickable background
    for(int row=0; row<12; row++)
    {
        for(int col=0; col<7; col++)
        {
            CourseCellWidget *cell = new CourseCellWidget(row+1, col+1);
            grid->addWidget(cell, row+1, col+1);
            connect(cell, &CourseCellWidget::createCourseRequested, this, &DashboardPage::createCourse);
        }
    }
}

int DashboardPage::getNearestDDL(const QString& courseName)
{
    int minDays = 9999;
    const auto tasks = DataManager::instance().tasks();

    for (const Task &task : tasks) {
        if (task.course == courseName && !task.completed) {
            const int daysLeft = QDateTime::currentDateTime().daysTo(task.deadline);
            if (daysLeft < minDays) {
                minDays = daysLeft;
            }
        }
    }
    return minDays == 9999 ? -999 : minDays;
}

void DashboardPage::renderCourses()
{
    qDebug() << "[DashboardPage] renderCourses called.";
    // 1. 彻底清理
    QLayoutItem *child;
    while ((child = grid->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->hide();
            child->widget()->deleteLater();
        }
        delete child;
    }

    updateDDLWidget();
    updateBottomStats();
    
    // 2. 绘制表头
    QStringList days = {"一","二","三","四","五","六","日"};
    for(int col=0; col<7; col++) {
        QFrame *dayFrame = new QFrame;
        dayFrame->setStyleSheet("background:#F8F8F8; border-radius:10px;");
        dayFrame->setFixedHeight(36);
        QVBoxLayout *dayLayout = new QVBoxLayout(dayFrame);
        dayLayout->setContentsMargins(0,0,0,0);
        QLabel *label = new QLabel("周" + days[col]);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-weight:700; color:#444; font-size:12px;");
        dayLayout->addWidget(label);
        grid->addWidget(dayFrame, 0, col+1);
    }

    QStringList timeSlots = {"8:00-8:50", "9:00-9:50", "10:10-11:00", "11:10-12:00", "13:00-13:50", "14:00-14:50", "15:10-16:00", "16:10-17:00", "17:10-18:00", "18:40-19:30", "19:40-20:30", "20:40-21:30"};
    for(int row=0; row<12; row++) {
        QLabel *timeLabel = new QLabel(QString("%1\n%2").arg(row+1).arg(timeSlots[row]));
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setStyleSheet("color:#aaa; font-size:10px;");
        grid->addWidget(timeLabel, row+1, 0);
    }

    // 3. 占用标记
    bool occupied[14][9];
    for(int r=0; r<14; r++) for(int c=0; c<9; c++) occupied[r][c] = false;

    // 4. 渲染课程
    const auto courses = DataManager::instance().courses();
    for (int i = 0; i < courses.size(); ++i) {
        const Course &c = courses[i];
        if (c.day < 1 || c.day > 7 || c.startPeriod < 1 || c.startPeriod > 12) continue;
        
        // 周次过滤
        if (c.weekType == 1 && currentWeek % 2 == 0) continue;
        if (c.weekType == 2 && currentWeek % 2 == 1) continue;

        const int daysLeft = getNearestDDL(c.name);
        CourseCellWidget *cell = new CourseCellWidget(c.startPeriod, c.day);
        cell->setCourse(c.name, c.location, c.teacher, i, daysLeft);

        // 绑定删除信号
        connect(cell, &CourseCellWidget::deleteCourseRequested, this, [this](int idx) {
            qDebug() << "[DashboardPage] deleteCourseRequested received for idx:" << idx;
            if (idx >= 0 && idx < DataManager::instance().courses().size()) {
                qDebug() << "[DashboardPage] Index" << idx << "is valid. Course name:" << DataManager::instance().courses()[idx].name;
                if (ConfirmDialog::confirm(this, "删除课程", "确认删除该课程及其所有关联任务？", "删除", true)) {
                    qDebug() << "[DashboardPage] Confirm dialog accepted. Calling DataManager::deleteCourse(" << idx << ")";
                    DataManager::instance().deleteCourse(idx);
                } else {
                    qDebug() << "[DashboardPage] Confirm dialog rejected.";
                }
            } else {
                qDebug() << "[DashboardPage] Received invalid idx:" << idx << "or courses list size is" << DataManager::instance().courses().size();
            }
        });
        
        // 绑定编辑信号 (双击通常发出此信号)
        connect(cell, &CourseCellWidget::editCourseRequested, this, &DashboardPage::editCourse);
        
        // 绑定直接编辑信号 (右键菜单编辑)
        connect(cell, &CourseCellWidget::editCourseDirectlyRequested, this, &DashboardPage::editCourseDirect);
        
        // 绑定添加 DDL
        connect(cell, &CourseCellWidget::addDDLRequested, this, [this](const QString &name) {
            TaskEditDialog dlg(this, name);
            if (dlg.exec() == QDialog::Accepted) {
                Task t; t.course = dlg.getCourseName(); t.title = dlg.getTitle();
                t.deadline = dlg.getDeadline(); t.priority = dlg.getPriority();
                DataManager::instance().addTask(t);
            }
        });
        
        // 绑定点击与双击详情
        connect(cell, &CourseCellWidget::courseClicked, this, [this](const Course& course) {
            emit openCourseDetail(course);
        });
        connect(cell, &CourseCellWidget::courseDoubleClicked, this, [this](const Course& course) {
            emit openCourseDetail(course);
        });

        int rowSpan = qMax(1, c.endPeriod - c.startPeriod + 1);
        grid->addWidget(cell, c.startPeriod, c.day, rowSpan, 1);
        
        for(int r = c.startPeriod; r <= c.endPeriod && r <= 12; r++) occupied[r][c.day] = true;
    }

    // 5. 填充空位
    for(int row=1; row<=12; row++) {
        for(int col=1; col<=7; col++) {
            if(!occupied[row][col]) {
                CourseCellWidget *emptyCell = new CourseCellWidget(row, col);
                grid->addWidget(emptyCell, row, col);
                connect(emptyCell, &CourseCellWidget::createCourseRequested, this, &DashboardPage::createCourse);
            }
        }
    }
}

void DashboardPage::createCourse(int row, int col)
{
    CourseEditDialog dialog(row, row);
    dialog.setWindowTitle("添加课程");
    
    if (dialog.exec() == QDialog::Accepted) {
        Course c;
        c.name = dialog.getName();
        c.teacher = dialog.getTeacher();
        c.location = dialog.getLocation();
        c.examTime = dialog.getExamTime();
        
        c.startPeriod = dialog.getStart();
        c.endPeriod = dialog.getEnd();
        c.weekType = dialog.getWeekType();
        c.day = col;

        // 检查是否已存在同名课程
        QList<Course> existingCourses = DataManager::instance().courses();
        int foundIndex = -1;
        for (int i = 0; i < existingCourses.size(); ++i) {
            if (existingCourses[i].name == c.name) {
                foundIndex = i;
                break;
            }
        }

        if (foundIndex < 0) {
            DataManager::instance().addCourse(c);
        } else {
            // 已存在：尝试自动合并更完整的信息
            Course original = existingCourses[foundIndex];
            Course merged = original;

            // 对字符串字段，优先保留更完整（非空）的值
            if (merged.teacher.isEmpty() && !c.teacher.isEmpty()) merged.teacher = c.teacher;
            if (merged.location.isEmpty() && !c.location.isEmpty()) merged.location = c.location;
            if (merged.examTime.isEmpty() && !c.examTime.isEmpty()) merged.examTime = c.examTime;

            // 时间/节次字段，0认为未设置
            if (merged.day == 0 && c.day != 0) merged.day = c.day;
            if (merged.startPeriod == 0 && c.startPeriod != 0) merged.startPeriod = c.startPeriod;
            if (merged.endPeriod == 0 && c.endPeriod != 0) merged.endPeriod = c.endPeriod;
            if (merged.weekType == 0 && c.weekType != 0) merged.weekType = c.weekType;

            // 检查是否存在冲突（只检查教师、位置、考试时间）
            // 同名课程在不同时段是允许的
            bool conflict = false;
            if (!original.teacher.isEmpty() && !c.teacher.isEmpty() && original.teacher != c.teacher) conflict = true;
            if (!original.location.isEmpty() && !c.location.isEmpty() && original.location != c.location) conflict = true;
            if (!original.examTime.isEmpty() && !c.examTime.isEmpty() && original.examTime != c.examTime) conflict = true;

                if (!conflict) {
                    // 无冲突：新增课程，并合并已有信息
                    Course newCourse = c;
                    if (!merged.teacher.isEmpty()) {
                        newCourse.teacher = merged.teacher;
                    }
                    if (!merged.location.isEmpty()) {
                        newCourse.location = merged.location;
                    }
                    if (!merged.examTime.isEmpty()) {
                        newCourse.examTime = merged.examTime;
                    }
                    DataManager::instance().addCourse(newCourse);
                } else {
                // 有冲突：提示用户选择
                QMessageBox msg(this);
                msg.setWindowTitle("课程冲突");
                msg.setText(QString("已存在课程“%1”，选择处理方式：").arg(c.name));
                QPushButton *keepBtn = msg.addButton("保留已有信息", QMessageBox::AcceptRole);
                QPushButton *overwriteBtn = msg.addButton("使用新信息覆盖", QMessageBox::DestructiveRole);
                QPushButton *manualBtn = msg.addButton("手动合并并编辑", QMessageBox::ActionRole);
                msg.setIcon(QMessageBox::Question);
                msg.exec();

                if (msg.clickedButton() == keepBtn) {
                    // 保留已有，无操作
                } else if (msg.clickedButton() == overwriteBtn) {
                    DataManager::instance().updateCourse(foundIndex, c);
                } else if (msg.clickedButton() == manualBtn) {
                    CourseEditDialog editDialog(original.startPeriod, original.endPeriod, this);
                    editDialog.setWindowTitle("合并课程信息");
                    // 预填充为 merged（已有更完整的值）
                    editDialog.setCourseData(merged.name, merged.teacher, merged.location, merged.examTime, merged.startPeriod, merged.endPeriod, merged.weekType);
                    if (editDialog.exec() == QDialog::Accepted) {
                        Course finalC = merged;
                        finalC.name = editDialog.getName();
                        finalC.teacher = editDialog.getTeacher();
                        finalC.location = editDialog.getLocation();
                        finalC.examTime = editDialog.getExamTime();
                        finalC.startPeriod = editDialog.getStart();
                        finalC.endPeriod = editDialog.getEnd();
                        finalC.weekType = editDialog.getWeekType();
                        DataManager::instance().updateCourse(foundIndex, finalC);
                    }
                }
            }
        }
    }
}

void DashboardPage::editCourse(int index)
{
    const auto courses = DataManager::instance().courses();
    if (index < 0 || index >= courses.size()) return;

    Course c = courses[index];
    
    CourseActionDialog actionDialog(this);
    actionDialog.exec();
    
    if (actionDialog.editSelected()) {
        CourseEditDialog dialog(c.startPeriod, c.endPeriod);
        dialog.setWindowTitle("编辑课程");
        dialog.setCourseData(c.name, c.teacher, c.location, c.examTime, c.startPeriod, c.endPeriod, c.weekType);
           QString originalName = c.name;
        
        if (dialog.exec() == QDialog::Accepted) {
            c.name = dialog.getName();
            c.teacher = dialog.getTeacher();
            c.location = dialog.getLocation();
            c.examTime = dialog.getExamTime();
            c.startPeriod = dialog.getStart();
            c.endPeriod = dialog.getEnd();
            c.weekType = dialog.getWeekType();

            
                // 如果课程名称相同，则更新所有同名课程
                if (c.name == originalName) {
                    const auto allCourses = DataManager::instance().courses();
                    for (int i = 0; i < allCourses.size(); ++i) {
                        if (allCourses[i].name == originalName) {
                            Course sameCourse = allCourses[i];
                            sameCourse.teacher = c.teacher;
                            sameCourse.location = c.location;
                            sameCourse.examTime = c.examTime;
                            sameCourse.weekType = c.weekType;
                            DataManager::instance().updateCourse(i, sameCourse);
                        }
                    }
                } else {
                    // 课程名称改了，只更新当前课程
                    DataManager::instance().updateCourse(index, c);
                }
        }
    } else if (actionDialog.deleteSelected()) {
        if (ConfirmDialog::confirm(
            this,
            "删除课程",
            "删除课程后，关联的任务也将被删除，是否继续？",
            "删除",
            true
        )) {
            DataManager::instance().deleteCourse(index);
            ToastWidget::showToast(this, "课程已删除", 3000);
        }
    } else if (actionDialog.ddlSelected()) {
        TaskEditDialog taskDialog(this, c.name);
        if (taskDialog.exec() == QDialog::Accepted) {
            Task newTask;
            newTask.course = taskDialog.getCourseName();
            newTask.title = taskDialog.getTitle();
            newTask.deadline = taskDialog.getDeadline();
            newTask.priority = taskDialog.getPriority();
            newTask.completed = false;

            DataManager::instance().addTask(newTask);
        }
    }
}

void DashboardPage::editCourseDirect(int index)
{
    const auto courses = DataManager::instance().courses();
    if (index < 0 || index >= courses.size()) return;

    Course c = courses[index];

    CourseEditDialog dialog(c.startPeriod, c.endPeriod);
    dialog.setWindowTitle("编辑课程");
    dialog.setCourseData(c.name, c.teacher, c.location, c.examTime, c.startPeriod, c.endPeriod, c.weekType);

    if (dialog.exec() == QDialog::Accepted) {
        const QString newName = dialog.getName();
        const QString newTeacher = dialog.getTeacher();
        const QString newLocation = dialog.getLocation();
        const QString newExamTime = dialog.getExamTime();
        int newStart = dialog.getStart();
        int newEnd = dialog.getEnd();
        int newWeekType = dialog.getWeekType();

        bool basicInfoChanged = (newTeacher != c.teacher ||
                                 newLocation != c.location ||
                                 newExamTime != c.examTime);

        bool timeChanged = (newStart != c.startPeriod ||
                           newEnd != c.endPeriod ||
                           newWeekType != c.weekType);

        c.teacher = newTeacher;
        c.location = newLocation;
        c.examTime = newExamTime;

        if (newName != c.name) {
            c.name = newName;
        }

        if (timeChanged) {
            c.startPeriod = newStart;
            c.endPeriod = newEnd;
            c.weekType = newWeekType;
        }

        const QString originalName = courses[index].name;
        const auto allCourses = DataManager::instance().courses();
        for (int i = 0; i < allCourses.size(); ++i) {
            if (allCourses[i].name == originalName) {
                Course updated = allCourses[i];
                updated.name = c.name;
                updated.teacher = c.teacher;
                updated.location = c.location;
                updated.examTime = c.examTime;
                if (timeChanged) {
                    updated.startPeriod = c.startPeriod;
                    updated.endPeriod = c.endPeriod;
                    updated.weekType = c.weekType;
                }
                DataManager::instance().updateCourse(i, updated);
            }
        }
    }
}

void DashboardPage::applyCourseUpdate(const Course& updatedCourse)
{
    if (updatedCourse.name.trimmed().isEmpty()) {
        return;
    }

    const auto courses = DataManager::instance().courses();
    for (int i = 0; i < courses.size(); ++i) {
        if (courses[i].name == updatedCourse.name) {
            Course merged = courses[i];
            merged.teacher = updatedCourse.teacher;
            merged.contact = updatedCourse.contact;
            merged.location = updatedCourse.location;
            merged.examTime = updatedCourse.examTime;
            merged.note = updatedCourse.note;
            merged.folderPath = updatedCourse.folderPath;
            DataManager::instance().updateCourse(i, merged);
        }
    }
}

void DashboardPage::refreshCourseUrgency()
{
    renderCourses();
    updateBottomStats();
}

QWidget* DashboardPage::createSuggestionCard()
{
    QFrame *card = new QFrame;
    card->setStyleSheet(QString("background:white; border-radius:%1px;").arg(Theme::CARD_RADIUS));
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(8);

    QLabel *title = new QLabel("今日建议");
    title->setStyleSheet(QString("font-weight:700; font-size:14px; color:%1;").arg(Theme::TEXT_PRIMARY));
    layout->addWidget(title);

    const auto tasks = DataManager::instance().tasks();
    const auto courses = DataManager::instance().courses();
    QDateTime now = QDateTime::currentDateTime();

    int overdueCount = 0;
    for (const Task& t : tasks) {
        if (t.isOverdue()) overdueCount++;
    }
    if (overdueCount > 0) {
        QLabel *tip = new QLabel(QString("⚠ %1个逾期任务需处理").arg(overdueCount));
        tip->setStyleSheet(QString("color:%1;font-size:12px;padding:8px;background:%2;border-radius:8px;").arg(Theme::DANGER).arg(Theme::PRIMARY_LIGHT));
        layout->addWidget(tip);
    }

    QMap<QString, int> courseTaskCount;
    for (const Task& t : tasks) {
        if (!t.completed) courseTaskCount[t.course]++;
    }
    for (auto it = courseTaskCount.constBegin(); it != courseTaskCount.constEnd(); ++it) {
        if (it.value() >= 3) {
            QLabel *tip = new QLabel(QString("📚 %1 有%2个待办").arg(it.key()).arg(it.value()));
            tip->setStyleSheet(QString("color:%1;font-size:12px;padding:8px;background:%2;border-radius:8px;").arg(Theme::WARNING).arg("#FFF8E1"));
            layout->addWidget(tip);
            break;
        }
    }

    int urgentCount = 0;
    for (const Task& t : tasks) {
        if (!t.completed && t.daysLeft() >= 0 && t.daysLeft() <= 2) urgentCount++;
    }
    if (urgentCount > 0) {
        QLabel *tip = new QLabel(QString("⏰ %1个任务即将到期").arg(urgentCount));
        tip->setStyleSheet(QString("color:%1;font-size:12px;padding:8px;background:#FFF3E0;border-radius:8px;").arg(Theme::WARNING));
        layout->addWidget(tip);
    }

    int total = tasks.size();
    int completed = 0;
    for (const Task& t : tasks) {
        if (t.completed) completed++;
    }
    double rate = total > 0 ? completed * 100.0 / total : 0;
    if (total > 5 && rate >= 80) {
        QLabel *tip = new QLabel(QString("🎉 完成率%1%，继续保持！").arg(qRound(rate)));
        tip->setStyleSheet(QString("color:%1;font-size:12px;padding:8px;background:#E8F5E9;border-radius:8px;").arg(Theme::SUCCESS));
        layout->addWidget(tip);
    }

    if (layout->count() == 1) {
        QLabel *empty = new QLabel("先添加课程和任务\n系统将生成建议");
        empty->setStyleSheet(QString("color:%1;font-size:12px;").arg(Theme::TEXT_TERTIARY));
        empty->setAlignment(Qt::AlignCenter);
        layout->addWidget(empty);
    }

    return card;
}

void DashboardPage::importSchedule()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("导入课表");
    msgBox.setText("请选择导入方式：");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStyleSheet(QString(
        "QMessageBox { background: white; border-radius: %1px; }"
        "QLabel { font-size: 14px; color: %2; padding: 8px; }"
        "QPushButton { "
        "  background: white; color: %2; border: 1px solid %3; "
        "  border-radius: %4px; padding: 10px 24px; font-size: 14px; font-weight: 500; min-width: 100px; "
        "} "
        "QPushButton:hover { background: %5; color: white; border-color: %3; }"
    ).arg(Theme::CARD_RADIUS).arg(Theme::TEXT_PRIMARY).arg(Theme::PRIMARY)
      .arg(Theme::BUTTON_RADIUS).arg(Theme::PRIMARY_DARK));

    QPushButton *imageBtn = msgBox.addButton("图片导入", QMessageBox::ActionRole);
    QPushButton *csvBtn = msgBox.addButton("CSV/TXT导入", QMessageBox::ActionRole);
    QPushButton *cancelBtn = msgBox.addButton("取消", QMessageBox::RejectRole);

    imageBtn->setCursor(Qt::PointingHandCursor);
    csvBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setCursor(Qt::PointingHandCursor);

    msgBox.exec();

    if (msgBox.clickedButton() == imageBtn) {
        importFromImage();
        return;
    } else if (msgBox.clickedButton() == csvBtn) {
    } else {
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "导入课表",
        QString(),
        "课表文件 (*.txt *.csv *.json);;所有文件 (*)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导入失败", "无法读取文件");
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    QList<Course> importedCourses;

    if (fileName.endsWith(".json")) {
        QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject obj = arr[i].toObject();
                Course c;
                c.name = obj["name"].toString();
                c.teacher = obj["teacher"].toString();
                c.location = obj["location"].toString();
                c.day = obj["day"].toInt(1);
                c.startPeriod = obj["startPeriod"].toInt(1);
                c.endPeriod = obj["endPeriod"].toInt(2);
                c.weekType = obj["weekType"].toInt(0);
                if (!c.name.isEmpty()) {
                    importedCourses.append(c);
                }
            }
        } else if (doc.isObject()) {
            QJsonObject root = doc.object();
            if (root.contains("courses") && root["courses"].isArray()) {
                QJsonArray arr = root["courses"].toArray();
                for (int i = 0; i < arr.size(); ++i) {
                    QJsonObject obj = arr[i].toObject();
                    Course c;
                    c.name = obj["name"].toString();
                    c.teacher = obj["teacher"].toString();
                    c.location = obj["location"].toString();
                    c.day = obj["day"].toInt(1);
                    c.startPeriod = obj["startPeriod"].toInt(1);
                    c.endPeriod = obj["endPeriod"].toInt(2);
                    c.weekType = obj["weekType"].toInt(0);
                    if (!c.name.isEmpty()) {
                        importedCourses.append(c);
                    }
                }
            }
        }
    } else {
        QStringList lines = content.split("\n", Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || trimmed.startsWith("#")) continue;

            QStringList parts;
            if (trimmed.contains(",")) {
                parts = trimmed.split(",");
            } else if (trimmed.contains("\t")) {
                parts = trimmed.split("\t");
            } else if (trimmed.contains(";")) {
                parts = trimmed.split(";");
            } else {
                continue;
            }

            if (parts.size() < 3) continue;

            Course c;
            c.name = parts[0].trimmed();
            c.teacher = parts.size() > 1 ? parts[1].trimmed() : "";

            QString timeStr = parts.size() > 2 ? parts[2].trimmed() : "";
            parseTimeString(timeStr, c);

            c.location = parts.size() > 3 ? parts[3].trimmed() : "";

            if (!c.name.isEmpty()) {
                importedCourses.append(c);
            }
        }
    }

    if (importedCourses.isEmpty()) {
        QMessageBox::warning(this, "导入失败", "未找到有效的课程数据\n请检查文件格式是否正确");
        return;
    }

    if (!DataManager::instance().courses().isEmpty()) {
        QMessageBox::StandardButton reply = ConfirmDialog::confirm3(this, "导入课表",
            "当前已有课程，是否覆盖？\n选择\"是\"将清空现有课程并导入新课表\n选择\"否\"将追加到现有课程",
            "是", "否", false);

        if (reply == QMessageBox::Yes) {
            auto courses = DataManager::instance().courses();
            for (int i = courses.size() - 1; i >= 0; --i) {
                DataManager::instance().deleteCourse(i);
            }
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    for (const Course& c : importedCourses) {
        DataManager::instance().addCourse(c);
    }

    QMessageBox::information(this, "导入成功", QString("成功导入 %1 门课程").arg(importedCourses.size()));
    renderCourses();
}

void DashboardPage::parseTimeString(const QString& timeStr, Course& c)
{
    QRegularExpression re("([周星期])*([1-7])[\\s]*([0-9]+)-([0-9]+)");
    QRegularExpressionMatch match = re.match(timeStr);
    if (match.hasMatch()) {
        c.day = match.captured(2).toInt();
        c.startPeriod = match.captured(3).toInt();
        c.endPeriod = match.captured(4).toInt();
    } else {
        c.day = 1;
        c.startPeriod = 1;
        c.endPeriod = 2;
    }
}

void DashboardPage::importFromImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择图片",
        QString(),
        "图片文件 (*.png *.jpg *.jpeg);;所有文件 (*)");

    if (fileName.isEmpty()) {
        return;
    }

    bool ok;
    QString apiKey = QInputDialog::getText(this, "Gemini API Key",
        "请输入 Gemini API Key:", QLineEdit::Password, "", &ok);

    if (!ok || apiKey.isEmpty()) {
        QMessageBox::warning(this, "导入失败", "未输入 API Key");
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "导入失败", "无法读取图片文件");
        return;
    }

    QByteArray imageData = file.readAll();
    file.close();

    m_loadingDialog = new QProgressDialog("正在识别课程表...", "这可能需要几秒钟", 0, 0, this);
    m_loadingDialog->setWindowTitle("处理中");
    m_loadingDialog->setWindowModality(Qt::WindowModal);
    m_loadingDialog->setCancelButton(nullptr);
    m_loadingDialog->setMinimumDuration(0);
    m_loadingDialog->setFixedSize(360, 140);
    m_loadingDialog->setStyleSheet(QString(
        "QProgressDialog { "
        "  background: white; border-radius: %1px; border: 1px solid %2; "
        "} "
        "QLabel { "
        "  color: %3; font-size: 15px; font-weight: 500; padding: 10px; "
        "} "
        "QLabel#qt_spinbox_label { "
        "  color: %4; font-size: 13px; "
        "} "
        "QProgressBar { "
        "  border: none; height: 4px; background: %5; "
        "} "
        "QProgressBar::chunk { "
        "  background: %6; border-radius: 2px; "
        "} "
    ).arg(Theme::CARD_RADIUS).arg(Theme::BORDER)
      .arg(Theme::TEXT_PRIMARY).arg(Theme::TEXT_SECONDARY)
      .arg(Theme::PRIMARY_LIGHT).arg(Theme::PRIMARY));
    m_loadingDialog->show();

    callGeminiAPI(apiKey, imageData);
}

void DashboardPage::callGeminiAPI(const QString& apiKey, const QByteArray& imageData)
{
    QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=%1").arg(apiKey));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray base64Image = imageData.toBase64();

    QJsonObject inlineData;
    inlineData["mime_type"] = "image/png";
    inlineData["data"] = QString::fromLatin1(base64Image);

    QJsonObject imagePart;
    imagePart["inline_data"] = inlineData;

    QJsonObject textPart;
    QString prompt = QString::fromUtf8(
        "You are parsing a Peking University course schedule screenshot.\n"
        "The screenshot may contain complex nested course descriptions like:\n"
        "- 上课信息\n"
        "- 教师\n"
        "- 备注\n"
        "- 习题课\n"
        "- 实验课\n"
        "- 主\n"
        "- 备\n"
        "- 考试信息\n"
        "Your task is to extract ALL actual weekly class sessions and convert them into structured JSON.\n"
        "Return ONLY valid JSON.\n"
        "Do NOT output markdown.\n"
        "Do NOT explain anything.\n"
        "IMPORTANT - SPATIAL REASONING PRIORITY:\n"
        "This screenshot is a VISUAL timetable grid.\n"
        "You must determine regular class time primarily based on the course block's visual position inside the timetable grid:\n"
        "- horizontal position = day of week\n"
        "- vertical position = class periods\n"
        "Use the actual grid alignment as the highest priority source for determining:\n"
        "- day\n"
        "- startPeriod\n"
        "- endPeriod\n"
        "Do NOT rely only on OCR text order.\n"
        "Do NOT infer class time from nearby unrelated text.\n"
        "A course block may contain messy text, but its actual scheduled time should be determined by where the block is visually located in the timetable.\n"
        "For example:\n"
        "- If a course block is visually located under Wednesday column:\n"
        "  day = 3\n"
        "- If a course block spans rows for periods 5-6:\n"
        "  startPeriod = 5\n"
        "  endPeriod = 6\n"
        "Even if OCR text is noisy, prioritize the visual grid layout.\n"
        "\n"
        "EXCEPTION:\n"
        "If tutorial/lab/discussion sessions (such as 习题课 / 实验课 / 讨论课)\n"
        "appear only in text remarks and are NOT shown as a visual block in the timetable grid,\n"
        "then parse their time from the textual description.\n"
        "Grid position should be used for regular timetable blocks.\n"
        "Text parsing should be used for extra sessions mentioned in remarks.\n"
        "\n"
        "Output format:\n"
        "{\n"
        "  \"courses\": [\n"
        "    {\n"
        "      \"day\": 1,\n"
        "      \"startPeriod\": 1,\n"
        "      \"endPeriod\": 2,\n"
        "      \"examTime\": \"\",\n"
        "      \"location\": \"\",\n"
        "      \"name\": \"\",\n"
        "      \"teacher\": \"\",\n"
        "      \"weekType\": 0\n"
        "    }\n"
        "  ]\n"
        "}\n"
        "Rules:\n"
        "1. Extract the real course name only. Remove labels such as (主), (备), 主, 备, 上课信息, 备注, 教师, 考试信息.\n"
        "2. If teacher field exists (教师：xxx), extract only actual teacher name.\n"
        "3. If 习题课/实验课/讨论课 appears with separate time/location, create an additional course object with name: original name + suffix (e.g., \"线性代数A(II)习题课\").\n"
        "4. Parse week frequency: 每周→weekType=0, 单周→weekType=1, 双周→weekType=2. Default to 0 if unclear.\n"
        "5. Parse day: 周一→1, 周二→2, 周三→3, 周四→4, 周五→5, 周六→6, 周日→7.\n"
        "6. Parse periods: 1~2节 or 1-2节 → startPeriod=1, endPeriod=2.\n"
        "7. If multiple classrooms listed (e.g., 三教506、三教308), store all in location field.\n"
        "8. Ignore exam schedules. Set examTime to empty string.\n"
        "9. Only output actual recurring teaching sessions. Do NOT output exam-only events, remarks, course IDs, credits, or notes.\n"
        "10. Return ONLY valid JSON."
    );
    textPart["text"] = prompt;

    QJsonArray parts;
    parts.append(textPart);
    parts.append(imagePart);

    QJsonObject content;
    content["parts"] = parts;

    QJsonArray contents;
    contents.append(content);

    QJsonObject root;
    root["contents"] = contents;

    QByteArray jsonData = QJsonDocument(root).toJson(QJsonDocument::Compact);

    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);
    }

    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onGeminiReplyFinished(reply);
    });
}

void DashboardPage::onGeminiReplyFinished(QNetworkReply* reply)
{
    if (m_loadingDialog) {
        m_loadingDialog->close();
        m_loadingDialog->deleteLater();
        m_loadingDialog = nullptr;
    }

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "导入失败", QString("API请求失败: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull()) {
        QMessageBox::warning(this, "导入失败", "无法解析API返回的数据");
        return;
    }

    QJsonObject root = doc.object();
    if (!root.contains("candidates") || root["candidates"].toArray().isEmpty()) {
        QMessageBox::warning(this, "导入失败", "API返回格式不正确");
        return;
    }

    QJsonArray candidates = root["candidates"].toArray();
    QJsonObject firstCandidate = candidates[0].toObject();
    if (!firstCandidate.contains("content") || !firstCandidate["content"].toObject().contains("parts")) {
        QMessageBox::warning(this, "导入失败", "API返回格式缺少内容");
        return;
    }

    QJsonObject content = firstCandidate["content"].toObject();
    QJsonArray parts = content["parts"].toArray();
    if (parts.isEmpty()) {
        QMessageBox::warning(this, "导入失败", "API返回没有内容");
        return;
    }

    QString jsonText = parts[0].toObject()["text"].toString();

    jsonText = jsonText.trimmed();
    if (jsonText.startsWith("```")) {
        int firstNewline = jsonText.indexOf('\n');
        int lastTripleBacktick = jsonText.lastIndexOf("```");
        if (firstNewline > 0 && lastTripleBacktick > firstNewline) {
            jsonText = jsonText.mid(firstNewline + 1, lastTripleBacktick - firstNewline - 1);
        }
    }
    jsonText = jsonText.trimmed();

    QJsonDocument coursesDoc = QJsonDocument::fromJson(jsonText.toUtf8());
    if (coursesDoc.isNull()) {
        qDebug() << "Failed to parse JSON. Raw response:" << jsonText.left(500);
        QMessageBox::warning(this, "导入失败", QString("无法解析课程数据\n\n原始响应:\n%1").arg(jsonText.left(200)));
        return;
    }

    QJsonArray coursesArray;
    if (coursesDoc.isArray()) {
        coursesArray = coursesDoc.array();
    } else if (coursesDoc.isObject()) {
        QJsonObject coursesObj = coursesDoc.object();
        if (coursesObj.contains("courses") && coursesObj["courses"].isArray()) {
            coursesArray = coursesObj["courses"].toArray();
        } else {
            QMessageBox::warning(this, "导入失败", "JSON格式不正确");
            return;
        }
    } else {
        QMessageBox::warning(this, "导入失败", "无法解析课程JSON");
        return;
    }

    QList<Course> importedCourses;
    for (int i = 0; i < coursesArray.size(); ++i) {
        QJsonObject obj = coursesArray[i].toObject();
        Course c;
        c.name = obj["name"].toString();
        c.teacher = obj["teacher"].toString();
        c.location = obj["location"].toString();
        c.examTime = obj["examTime"].toString();
        c.day = obj["day"].toInt(1);
        c.startPeriod = obj["startPeriod"].toInt(1);
        c.endPeriod = obj["endPeriod"].toInt(2);
        c.weekType = obj["weekType"].toInt(0);

        if (!c.name.isEmpty()) {
            importedCourses.append(c);
        }
    }

    if (importedCourses.isEmpty()) {
        QMessageBox::warning(this, "导入失败", "未找到有效的课程数据");
        return;
    }

    if (!DataManager::instance().courses().isEmpty()) {
        QMessageBox::StandardButton reply = ConfirmDialog::confirm3(this, "导入课表",
            "当前已有课程，是否覆盖？\n选择\"是\"将清空现有课程并导入新课表\n选择\"否\"将追加到现有课程",
            "是", "否", false);

        if (reply == QMessageBox::Yes) {
            auto courses = DataManager::instance().courses();
            for (int i = courses.size() - 1; i >= 0; --i) {
                DataManager::instance().deleteCourse(i);
            }
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    for (const Course& c : importedCourses) {
        DataManager::instance().addCourse(c);
    }

    QMessageBox::information(this, "导入成功", QString("成功导入 %1 门课程").arg(importedCourses.size()));
    renderCourses();
}