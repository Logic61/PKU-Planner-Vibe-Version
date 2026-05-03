#include "dashboardpage.h"
#include "../components/coursecellwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QProgressBar>
#include <QPushButton>
#include <QCheckBox>
#include <QTimer>
#include <QDateTime>
#include <QScrollArea>
#include <algorithm>
#include <vector>
#include <utility>

#include "../models/datamanager.h"
#include "../dialogs/courseeditdialog.h"
#include "../dialogs/courseactiondialog.h"
#include "../dialogs/taskeditdialog.h"

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
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

    // === CRITICAL: Create grid BEFORE createTopBar() which calls renderCourses() ===
    QWidget *gridContainer = new QWidget;
    grid = new QGridLayout(gridContainer);
    grid->setSpacing(6);

    // ===== 1. 顶部学期控制栏 =====
    mainLayout->addWidget(createTopBar());

    // ===== 2. 主体区域 =====
    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(12);

    // 左：课程表
    QFrame *courseCard = new QFrame;
    courseCard->setStyleSheet("background:white; border-radius:16px; border:1px solid #ECECEC;");
    QVBoxLayout *courseLayout = new QVBoxLayout(courseCard);

    QLabel *courseTitle = new QLabel("课程表");
    courseTitle->setStyleSheet("font-weight:700; font-size:16px; color:#222;");
    courseLayout->addWidget(courseTitle);

    courseLayout->addWidget(gridContainer);

    contentLayout->addWidget(courseCard, 7); // 70%

    // 右：侧栏
    QWidget *rightPanelWidget = createRightPanel();
    contentLayout->addWidget(rightPanelWidget, 3); // 30%

    mainLayout->addLayout(contentLayout);

    // ===== 3. 底部统计卡片 =====
    mainLayout->addWidget(createBottomStats());

    // 将 container 设置为滚动区域的内容
    scrollArea->setWidget(container);

    // DashboardPage 自身的布局只放 scrollArea
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    // Connect signals for reactive updates
    connect(&DataManager::instance(), &DataManager::coursesChanged, this, &DashboardPage::renderCourses);
    connect(&DataManager::instance(), &DataManager::tasksChanged, this, &DashboardPage::renderCourses);
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

    semesterProgress->setStyleSheet(R"(
        QProgressBar {
            border:none;
            background:#F5F5F5;
            height:16px;
            border-radius:8px;
        }

        QProgressBar::chunk {
            background:#8B1E2D;
            border-radius:8px;
        }
    )");

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
        updateWeekInfo();
    });

    connect(nextBtn,&QPushButton::clicked,this,[=](){
        currentWeek++;
        updateWeekInfo();
    });

    connect(todayBtn,&QPushButton::clicked,this,[=](){
        currentWeek = realWeek;
        updateWeekInfo();
    });

    updateWeekInfo();

    return bar;
}

void DashboardPage::updateWeekInfo()
{
    if(currentWeek < 1) currentWeek = 1;
    if(currentWeek > 18) currentWeek = 18;

    semesterProgress->setValue(currentWeek);

    QString type = (currentWeek % 2 == 1) ? "单周" : "双周";

    weekLabel->setText(
        QString("第 %1 周 (%2)")
            .arg(currentWeek)
            .arg(type)
    );
    renderCourses();
}

QWidget* DashboardPage::createBottomStats()
{
    QWidget *widget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0,0,0,0);

    QStringList titles = {
        "今日课程",
        "今日DDL",
        "本周DDL",
        "当前时间"
    };

    for(auto t : titles)
    {
        QFrame *card = new QFrame;
        card->setStyleSheet(
            "background:white;border-radius:12px;padding:8px; border:1px solid #ECECEC;"
        );

        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(8,8,8,8);

        QLabel *num = new QLabel("0");
        num->setStyleSheet(
            "font-size:18px;font-weight:700;color:#8B1E2D;"
        );

        QLabel *title = new QLabel(t);
        title->setStyleSheet("color:#666;font-size:12px;");

        cl->addWidget(num);
        cl->addWidget(title);

        if(t == "当前时间")
        {
            timeLabel = num;
            timeLabel->setStyleSheet("font-size:14px;font-weight:bold;color:#8B1E2D;");

            QTimer *timer = new QTimer(this);
            connect(timer,&QTimer::timeout,this,[=](){
                timeLabel->setText(
                    QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
                );
            });
            timer->start(1000);
            timeLabel->setText(
                QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
            );
        }

        layout->addWidget(card);
    }

    return widget;
}

QWidget* DashboardPage::createRightPanel()
{
    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(12);

    // 今日课程卡
    QFrame *todayCard = new QFrame;
    todayCard->setStyleSheet("background:white; border-radius:16px; border:1px solid #ECECEC;");
    QVBoxLayout *todayLayout = new QVBoxLayout(todayCard);
    
    QLabel *todayTitle = new QLabel("今日课程");
    todayTitle->setStyleSheet("font-weight:700; font-size:14px; color:#222;");
    todayLayout->addWidget(todayTitle);
    
    QLabel *todayContent = new QLabel("暂无课程");
    todayContent->setStyleSheet("color:#999;");
    todayLayout->addWidget(todayContent);

    // DDL摘要卡
    QFrame *ddlCard = new QFrame;
    ddlCard->setStyleSheet("background:white; border-radius:16px; border:1px solid #ECECEC;");
    ddlLayout = new QVBoxLayout(ddlCard);
    QLabel *ddlTitle = new QLabel("DDL提醒");
    ddlTitle->setStyleSheet("font-weight:700; font-size:16px; margin-bottom: 8px; color:#222;");
    ddlLayout->addWidget(ddlTitle);
    updateDDLWidget();

    layout->addWidget(todayCard);
    layout->addWidget(ddlCard);
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
                background:#FAFAFA;
                border:1px solid #ECECEC;
                border-radius:12px;
            }
        )");
        QVBoxLayout *vl = new QVBoxLayout(itemFrame);
        vl->setContentsMargins(10,10,10,10);
        vl->setSpacing(8);

        QHBoxLayout *topRow = new QHBoxLayout;
        topRow->setContentsMargins(0,0,0,0);
        topRow->setSpacing(8);

        QCheckBox *doneBox = new QCheckBox;
        doneBox->setChecked(task.completed);
        doneBox->setStyleSheet(R"(
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
                border-radius: 9px;
                border: 1px solid #8B1E2D;
                background: white;
            }
            QCheckBox::indicator:checked {
                background: #8B1E2D;
                border: 1px solid #8B1E2D;
            }
        )");

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
            timeLbl->setStyleSheet("color:#D32F2F; font-size:12px; font-weight:600;");
        } else if (daysLeft == 0) {
            timeLbl->setText("今晚截止");
            timeLbl->setStyleSheet("color:#E64A19; font-size:12px; font-weight:600;");
        } else {
            timeLbl->setText(QString("剩余 %1 天").arg(daysLeft));
            timeLbl->setStyleSheet("color:#8B1E2D; font-size:12px;");
        }

        QLabel *courseLbl = new QLabel(task.course);
        courseLbl->setStyleSheet("color:#777; font-size:12px;");

        QPushButton *editBtn = new QPushButton("编辑");
        QPushButton *deleteBtn = new QPushButton("删除");
        for (QPushButton *btn : {editBtn, deleteBtn}) {
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(R"(
                QPushButton {
                    background: transparent;
                    color: #8B1E2D;
                    border: 1px solid #E2C9CD;
                    border-radius: 8px;
                    padding: 4px 10px;
                }
                QPushButton:hover {
                    background: #8B1E2D;
                    color: white;
                }
            )");
        }

        metaRow->addWidget(courseLbl);
        metaRow->addWidget(timeLbl);
        metaRow->addStretch();
        metaRow->addWidget(editBtn);
        metaRow->addWidget(deleteBtn);

        vl->addLayout(topRow);
        vl->addLayout(metaRow);

        connect(doneBox, &QCheckBox::toggled, this, [taskIndex](bool checked) {
            DataManager::instance().markTaskCompleted(taskIndex, checked);
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
                DataManager::instance().updateTask(taskIndex, updated);
            }
        });

        connect(deleteBtn, &QPushButton::clicked, this, [taskIndex]() {
            DataManager::instance().deleteTask(taskIndex);
        });

        if (task.completed) {
            itemFrame->setStyleSheet(R"(
                QFrame {
                    background:#F3F3F3;
                    border:1px solid #E6E6E6;
                    border-radius:12px;
                }
            )");
        }

        ddlLayout->addWidget(itemFrame);
        hasDDL = true;
        ++count;
    }
    
    if (!hasDDL) {
        QLabel *empty = new QLabel("暂无DDL");
        empty->setStyleSheet("color:#999;");
        ddlLayout->addWidget(empty);
    }
}

void DashboardPage::initGrid()
{
    QStringList days = {"一","二","三","四","五","六","日"};

    // 星期
    for(int col=0; col<7; col++)
    {
        QLabel *label = new QLabel("周" + days[col]);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-weight:700; color:#444; font-size:12px;");
        grid->addWidget(label, 0, col+1);
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
    QLayoutItem *child;
    while ((child = grid->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    updateDDLWidget();
    initGrid();

    int index = 0;
    const auto courses = DataManager::instance().courses();
    for (const Course &c : courses)
    {
        bool showCourse = true;
        if (c.weekType == 1 && currentWeek % 2 == 0) {
            showCourse = false;
        }
        if (c.weekType == 2 && currentWeek % 2 == 1) {
            showCourse = false;
        }

        if (showCourse) {
            const int daysLeft = getNearestDDL(c.name);
            CourseCellWidget *cell = new CourseCellWidget(c.startPeriod, c.day);
            cell->setCourse(c.name, c.location, c.teacher, index, daysLeft);
            
            connect(cell, &CourseCellWidget::editCourseRequested,
                    this, &DashboardPage::editCourse);

            grid->addWidget(
                cell,
                c.startPeriod,
                c.day,
                c.endPeriod - c.startPeriod + 1,
                1
            );
        }
        index++;
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

        DataManager::instance().addCourse(c);
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
        
        if (dialog.exec() == QDialog::Accepted) {
            c.name = dialog.getName();
            c.teacher = dialog.getTeacher();
            c.location = dialog.getLocation();
            c.examTime = dialog.getExamTime();
            c.startPeriod = dialog.getStart();
            c.endPeriod = dialog.getEnd();
            c.weekType = dialog.getWeekType();
            // c.day remains unchanged from edit interface (unless col added to dialog)

            DataManager::instance().updateCourse(index, c);
        }
    } else if (actionDialog.deleteSelected()) {
        DataManager::instance().deleteCourse(index);
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