#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QDockWidget>
#include <QDateTimeEdit>
#include <QDateTime>
#include <QTime>
#include <QBrush>
#include <QColor>
#include <QListWidgetItem>
#include <QToolBar>
#include <QLabel>
#include <QApplication>
#include <QTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QStatusBar>

#include <QMenu>
#include <QDialog>
#include <QTabWidget>
#include <QFormLayout>
#include <QGroupBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QProgressBar>
#include <QMessageBox>
#include <QComboBox>
#include <QMap>
#include <QPair>
#include <QSet>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    applyTheme();
    
    // 初始化工具栏
    QToolBar *toolBar = new QToolBar("主工具栏", this);
    toolBar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, toolBar);
    
    QLabel *weekInfoLabel = new QLabel(this);
    weekInfoLabel->setObjectName("weekInfoLabel");
    toolBar->addWidget(weekInfoLabel);
    
    QLabel *weekStateLabel = new QLabel(this);
    weekStateLabel->setObjectName("weekStateLabel");
    toolBar->addWidget(weekStateLabel);
    
    // Add Mascot Btn
    m_mascotBtn = new QPushButton("(❁´◡`❁)", this);
    m_mascotBtn->setObjectName("mascotBtn");
    m_mascotBtn->setFlat(true);
    m_mascotBtn->setCursor(Qt::PointingHandCursor);
    m_mascotBtn->setStyleSheet("font-size: 24px; padding: 0 10px; border: none; background: transparent;");
    toolBar->addWidget(m_mascotBtn);
    
    // You can connect the mascot clicked signal
    connect(m_mascotBtn, &QPushButton::clicked, this, [this]() {
        // Implement popup summary later
    });
    
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);
    
    // 添加夜间模式切换按钮
    QToolButton *themeToggleBtn = new QToolButton(this);
    themeToggleBtn->setText("🌙 主题");
    themeToggleBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    themeToggleBtn->setStyleSheet("QToolButton { background-color: #E2E8F0; padding: 6px 12px; border-radius: 8px; color: #1E293B; font-weight: bold; margin-left: 10px; }"
                                  "QToolButton:hover { background-color: #CBD5E1; }");
    connect(themeToggleBtn, &QToolButton::clicked, this, &MainWindow::onToggleThemeClicked);
    toolBar->addWidget(themeToggleBtn);
    
    // 开关侧边栏操作美化按钮
    QToolButton *toggleBtn = new QToolButton(this);
    // 这里待会儿要关联，先留空 // toggleBtn->setDefaultAction(m_todoDock->toggleViewAction()); // dock还没创建
    toggleBtn->setText("📋 待办");
    toggleBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggleBtn->setStyleSheet("QToolButton { background-color: #E2E8F0; padding: 6px 12px; border-radius: 8px; color: #1E293B; font-weight: bold; margin-left: 10px; }"
                             "QToolButton:hover { background-color: #CBD5E1; }");
    toolBar->addWidget(toggleBtn);
    
    // 添加时间标签
    m_timeLabel = new QLabel(this);
    m_timeLabel->setObjectName("timeLabel");
    toolBar->addWidget(m_timeLabel);
    
    // 启动定时器刷新时间
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTime);
    timer->start(1000); // 每秒刷新一次
    updateTime(); // 初始调用一次
    
    // 初始化待办事项侧边栏
    QDockWidget *dock = new QDockWidget("📝 待办事项 (To-Do)", this);
    dock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    
    toggleBtn->setDefaultAction(dock->toggleViewAction());
    
    QWidget *dockContents = new QWidget(dock);
    QVBoxLayout *dockLayout = new QVBoxLayout(dockContents);
    dockLayout->setContentsMargins(15, 15, 15, 15);
    dockLayout->setSpacing(12);
    
    QVBoxLayout *inputLayout = new QVBoxLayout();
    inputLayout->setSpacing(10);
    
    m_todoInput = new QLineEdit(dockContents);
    m_todoInput->setPlaceholderText("填写待办任务...");
    m_todoInput->setEnabled(false); // 默认禁用，选中课程再启用
    m_todoInput->setMinimumHeight(40);
    
    QHBoxLayout *subInputLayout = new QHBoxLayout();
    subInputLayout->setSpacing(10);
    m_todoDateInput = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1), dockContents);
    m_todoDateInput->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_todoDateInput->setCalendarPopup(true);
    m_todoDateInput->setEnabled(false);
    m_todoDateInput->setMinimumHeight(40);
    
    m_addTodoBtn = new QPushButton("添加记录", dockContents);
    m_addTodoBtn->setObjectName("addBtn");
    m_addTodoBtn->setEnabled(false);
    m_addTodoBtn->setMinimumHeight(40);
    m_addTodoBtn->setCursor(Qt::PointingHandCursor);
    
    subInputLayout->addWidget(m_todoDateInput, 1);
    subInputLayout->addWidget(m_addTodoBtn, 1);
    
    inputLayout->addWidget(m_todoInput);
    inputLayout->addLayout(subInputLayout);
    
    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(8);
    m_todoCourseFilterCombo = new QComboBox(dockContents);
    m_todoCourseFilterCombo->addItem("所有课程");
    m_todoTimeFilterCombo = new QComboBox(dockContents);
    m_todoTimeFilterCombo->addItems({"全部时间", "今天", "本周", "已逾期"});
    m_todoSortCombo = new QComboBox(dockContents);
    m_todoSortCombo->addItems({"默认排序(状态+DDL)", "按课程", "按截止时间按早到晚", "按紧急度"});
    
    filterLayout->addWidget(new QLabel("过滤:"));
    filterLayout->addWidget(m_todoCourseFilterCombo, 1);
    filterLayout->addWidget(m_todoTimeFilterCombo, 1);
    filterLayout->addWidget(new QLabel("排序:"));
    filterLayout->addWidget(m_todoSortCombo, 1);
    
    m_todoTableWidget = new QTableWidget(0, 5, dockContents);
    m_todoTableWidget->setHorizontalHeaderLabels({"任务", "课程", "DDL", "剩余", "状态/操作"});
    m_todoTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_todoTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_todoTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_todoTableWidget->verticalHeader()->setVisible(false);

    dockLayout->addLayout(inputLayout);
    dockLayout->addLayout(filterLayout);
    dockLayout->addWidget(m_todoTableWidget);
    
    dock->setWidget(dockContents);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    
    // Connect filter signals
    connect(m_todoCourseFilterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshTodoList);
    connect(m_todoTimeFilterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshTodoList);
    connect(m_todoSortCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshTodoList);
    
    // 待办点击事件
    connect(m_todoTableWidget, &QTableWidget::cellClicked, this, [this](int row, int col) {
        // Toggle complete if checking
        if (col == 4) {
            QTableWidgetItem *statusItem = m_todoTableWidget->item(row, 4);
            if (!statusItem) return;
            QString courseName = statusItem->data(Qt::UserRole).toString();
            int todoIndex = statusItem->data(Qt::UserRole + 1).toInt();
            
            for (Course &c : m_courses) {
                if (c.name == courseName) {
                    if (todoIndex >= 0 && todoIndex < c.todos.size()) {
                        c.todos[todoIndex].isCompleted = !c.todos[todoIndex].isCompleted;
                        refreshTodoList(); // 重新加载表格以反映变化
                        refreshSchedule(); // 重新刷新课表以更新神态和颜色
                    }
                    break;
                }
            }
        }
    });
    
    // 添加交互
    connect(m_addTodoBtn, &QPushButton::clicked, this, &MainWindow::onAddTodoClicked);
    connect(m_todoInput, &QLineEdit::returnPressed, this, &MainWindow::onAddTodoClicked);
    
    // 双击进入课程详情
    connect(m_todoTableWidget, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        QTableWidgetItem *statusItem = m_todoTableWidget->item(row, 4);
        if (!statusItem) return;
        QString courseName = statusItem->data(Qt::UserRole).toString();
        for (int i = 0; i < m_courses.size(); ++i) {
            if (m_courses[i].name == courseName) {
                showCourseDetailsDialog(i);
                break;
            }
        }
    });
    
    // 添加待办事项切换按钮到工具栏
    QAction *toggleDockAction = dock->toggleViewAction();
    toggleBtn->setDefaultAction(toggleDockAction);
    toggleBtn->setText("📋 待办");
    
    // 初始化课表视图
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setShowGrid(true); // 显示网格使结构清晰
    setCentralWidget(m_tableWidget);
    
    initCalendarView();
    loadInitialData();
    
    // 连接表格双击事件进入课程详情
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &MainWindow::onCellDoubleClicked);
    
    // 修改吉祥物的点击事件进入全局 DashBoard 视图
    disconnect(m_mascotBtn, &QPushButton::clicked, this, nullptr);
    connect(m_mascotBtn, &QPushButton::clicked, this, &MainWindow::showDashboardDialog);

    // 在刷新逻辑中更新周数标签
    refreshSchedule();
    
    // 获取当周信息并更新 UI
    QDate termStartDate(2026, 2, 23);
    int daysDiff = termStartDate.daysTo(QDate::currentDate());
    if (daysDiff < 0) daysDiff = 0;
    int currentWeek = daysDiff / 7 + 1;
    bool isCurrentWeekOdd = (currentWeek % 2 != 0);
    
    QDate cur = QDate::currentDate();
    weekInfoLabel->setText(QString("%1/%2/%3 (第%4周)").arg(cur.year()).arg(cur.month()).arg(cur.day()).arg(currentWeek));
    weekStateLabel->setText(QString("【当前：%1】").arg(isCurrentWeekOdd ? "单周" : "双周"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initCalendarView()
{
    m_tableWidget->setRowCount(12);
    m_tableWidget->setColumnCount(7);
    
    QStringList days;
    days << "周一" << "周二" << "周三" << "周四" << "周五" << "周六" << "周日";
    m_tableWidget->setHorizontalHeaderLabels(days);
    
    QStringList timeSlots = {
        "08:00", "09:00", "10:10", "11:10",
        "13:00", "14:00", "15:10", "16:10",
        "17:10", "18:40", "19:40", "20:40"
    };

    // 每节课50分钟，计算结束时间
    QStringList endTimes = {
        "08:50", "09:50", "11:00", "12:00",
        "13:50", "14:50", "16:00", "17:00",
        "18:00", "19:30", "20:30", "21:30"
    };

    QStringList periods;
    for (int i = 0; i < 12; ++i) {
        periods << QString("第%1节\n%2-%3").arg(i + 1).arg(timeSlots[i]).arg(endTimes[i]);
    }
    m_tableWidget->setVerticalHeaderLabels(periods);
    
    // 显示网格
    m_tableWidget->setShowGrid(true);
    m_tableWidget->setFrameShape(QFrame::NoFrame);
    
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // 不可编辑
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    connect(m_tableWidget, &QTableWidget::cellClicked, this, &MainWindow::onCellClicked);
}

void MainWindow::loadInitialData()
{
    // 模拟数据 (修改掉所有带红色的配色)
    Course c1{"数据结构", "理教101", "张老师", QDateTime(), "", 1, 3, 2, Course::Odd, "#D0E8FF", {}}; // 浅蓝
    c1.todos.append({"完成作业1", QDateTime::currentDateTime().addDays(1), false, "", ""});
    c1.todos.append({"阅读第一章 DDL", QDateTime::currentDateTime().addSecs(3600), false, "", ""});
    
    Course c2{"大学物理", "理教102", "李老师", QDateTime(), "", 2, 1, 2, Course::All, "#E8D0FF", {}}; // 浅紫
    c2.todos.append({"完成实验报告", QDateTime::currentDateTime().addDays(2), false, "", ""});
    
    Course c3{"面向对象编程", "二教201", "王老师", QDateTime(), "", 3, 5, 2, Course::Even, "#D0FFE8", {}}; // 浅绿
    c3.todos.append({"预习下一课 DDL", QDateTime::currentDateTime().addDays(3), false, "", ""});

    Course c4{"线代A(II)", "理教203", "陈老师", QDateTime(), "", 3, 1, 2, Course::All, "#C4B5FD", {}}; // 紫色
    c4.todos.append({"线性代数练习", QDateTime::currentDateTime().addDays(4), false, "", ""});
    Course c5{"高数A(二)", "理教408", "赵老师", QDateTime(), "", 2, 2, 2, Course::All, "#93C5FD", {}}; // 蓝
    c5.todos.append({"高数习题", QDateTime::currentDateTime().addDays(5), false, "", ""});
    Course c6{"西方文化选读", "文史314", "刘老师", QDateTime(), "", 2, 3, 1, Course::All, "#FDE047", {}}; // 黄
    c6.todos.append({"准备课堂讨论", QDateTime::currentDateTime().addDays(6), false, "", ""});
    Course c7{"程设", "二教405", "孙老师", QDateTime(), "", 3, 4, 1, Course::All, "#6EE7B7", {}}; // 绿
    c7.todos.append({"项目提交", QDateTime::currentDateTime().addDays(2), false, "", ""});
    Course c8{"AI基", "理教211", "周老师", QDateTime(), "", 1, 5, 1, Course::All, "#5EEAD4", {}}; // 青
    c8.todos.append({"阅读AI论文", QDateTime::currentDateTime().addDays(3), false, "", ""});
    Course c9{"史纲", "理教107", "吴老师", QDateTime(), "", 3, 7, 1, Course::All, "#A5B4FC", {}}; // 靛青

    m_courses.append(c4);
    m_courses.append(c5);
    m_courses.append(c6);
    m_courses.append(c7);
    m_courses.append(c8);
    m_courses.append(c9);
    
    // 当前时间段的课 (测试高亮)
    QTime now = QTime::currentTime();
    int currentSection = -1;
    
    struct TimeRange { QTime start; QTime end; };
    TimeRange ranges[12] = {
        {QTime(8,0), QTime(8,50)}, {QTime(9,0), QTime(9,50)},
        {QTime(10,10), QTime(11,0)}, {QTime(11,10), QTime(12,0)},
        {QTime(13,0), QTime(13,50)}, {QTime(14,0), QTime(14,50)},
        {QTime(15,10), QTime(16,0)}, {QTime(16,10), QTime(17,0)},
        {QTime(17,10), QTime(18,0)}, {QTime(18,40), QTime(19,30)},
        {QTime(19,40), QTime(20,30)}, {QTime(20,40), QTime(21,30)}
    };

    for (int i = 0; i < 12; ++i) {
        if (now >= ranges[i].start && now <= ranges[i].end) {
            currentSection = i + 1;
            break;
        }
    }
    
    if (currentSection != -1) {
        Course cRealTime{"实时测试课", "机房", "张老师", QDateTime(), "", QDate::currentDate().dayOfWeek(), currentSection, 1, Course::All, "", {}};
        cRealTime.todos.append({"提交代码作业", QDateTime::currentDateTime().addSecs(3600), false, "", ""});
        m_courses.append(cRealTime);
    }
    
    m_courses.append(c1);
    m_courses.append(c2);
    m_courses.append(c3);
}

void MainWindow::refreshSchedule()
{
    // 清空内容并移除所有的单元格合并
    m_tableWidget->clearContents();
    for (int r = 0; r < m_tableWidget->rowCount(); ++r) {
        for (int c = 0; c < m_tableWidget->columnCount(); ++c) {
            m_tableWidget->setSpan(r, c, 1, 1);
        }
    }
    
    // 检测课程时间重叠
    QMap<QPair<int, int>, QList<int>> cellToCourses; // 单元格（行、列）对应的课程索引列表
    for (int i = 0; i < m_courses.size(); ++i) {
        const Course &course = m_courses[i];
        int startRow = course.start - 1; // 转换为0-based行号
        int col = course.day - 1; // 转换为0-based列号
        if (startRow < 0 || startRow >= 12 || col < 0 || col >= 7) continue;
        
        // 记录该课程占据的所有单元格
        for (int r = startRow; r < startRow + course.duration && r < 12; ++r) {
            QPair<int, int> cell(r, col);
            cellToCourses[cell].append(i);
        }
    }
    
    // 收集冲突的课程索引
    QSet<int> conflictCourseIndices;
    for (auto it = cellToCourses.begin(); it != cellToCourses.end(); ++it) {
        if (it.value().size() > 1) {
            // 该单元格有多个课程，记录所有冲突课程
            for (int idx : it.value()) {
                conflictCourseIndices.insert(idx);
            }
        }
    }
    
    // 如果有冲突，提示用户
    if (!conflictCourseIndices.isEmpty()) {
        QString conflictMsg = "检测到以下课程时间冲突：\n";
        for (int idx : conflictCourseIndices) {
            const Course &course = m_courses[idx];
            conflictMsg += QString("• %1（周%2 第%3-%4节）\n")
                              .arg(course.name)
                              .arg(course.day)
                              .arg(course.start)
                              .arg(course.start + course.duration - 1);
        }
        QMessageBox::warning(this, "时间冲突", conflictMsg);
    }
    
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDate currentDate = currentDateTime.date();
    QTime currentTime = currentDateTime.time();
    
    int currentDayOfWeek = currentDate.dayOfWeek();
    
    // 开学第一周周一为 2026-02-23
    QDate termStartDate(2026, 2, 23);
    int daysDiff = termStartDate.daysTo(currentDate);
    if (daysDiff < 0) daysDiff = 0;
    int currentWeek = daysDiff / 7 + 1;
    bool isCurrentWeekOdd = (currentWeek % 2 != 0);
    
    int currentSection = -1;
    struct TimeRange { QTime start; QTime end; };
    TimeRange ranges[12] = {
        {QTime(8,0), QTime(8,50)}, {QTime(9,0), QTime(9,50)},
        {QTime(10,10), QTime(11,0)}, {QTime(11,10), QTime(12,0)},
        {QTime(13,0), QTime(13,50)}, {QTime(14,0), QTime(14,50)},
        {QTime(15,10), QTime(16,0)}, {QTime(16,10), QTime(17,0)},
        {QTime(17,10), QTime(18,0)}, {QTime(18,40), QTime(19,30)},
        {QTime(19,40), QTime(20,30)}, {QTime(20,40), QTime(21,30)}
    };

    for (int i = 0; i < 12; ++i) {
        if (currentTime >= ranges[i].start && currentTime <= ranges[i].end) {
            currentSection = i + 1; // 1-based class period
            break;
        }
    }

    QStringList colorPalette = m_isDarkMode ? QStringList{
        "#2E2C4A", "#4A2826", "#1E3B5C", "#4A3B22", "#472138", "#4A3D22"
    } : QStringList{
        "#E7E0FF", "#FFE1DE", "#D4E4FF", "#FFE2C6", "#FCD5E5", "#FDE397"
    };
    
    int colorIndex = 0;
    double globalMaxUrgency = 0.0;
    
    for (const Course &c : m_courses) {
        int row = c.start - 1;
        int col = c.day - 1;
        
        if (row >= 12 || col >= 7) continue;
        
        // 合并单元格
        if (c.duration > 1) {
            m_tableWidget->setSpan(row, col, c.duration, 1);
        }
        
        QTableWidgetItem *item = new QTableWidgetItem();
        m_tableWidget->setItem(row, col, item);
        
        // 计算紧急度和最近 DDLI
        double maxUrgency = 0.0;
        QString nearestDdlText = "无紧迫任务";
        qint64 minSecsToDdl = std::numeric_limits<qint64>::max();

        for (const TodoItem &todo : c.todos) {
            if (todo.isCompleted) continue;
            qint64 secsToDdl = currentDateTime.secsTo(todo.deadline);
            double urgency = 0.0;
            if (secsToDdl <= 86400) { // 今日内或已逾期 (<= 24h)
                urgency = 1.0;
            } else if (secsToDdl <= 3 * 86400) { // 未来3天内 (1-3天)
                urgency = 0.5 + 0.5 * (3 * 86400 - secsToDdl) / (2 * 86400.0);
            } else if (secsToDdl <= 14 * 86400) { // 14天内
                urgency = 0.5 * (14 * 86400 - secsToDdl) / (11 * 86400.0);
            }

            if (urgency > maxUrgency) maxUrgency = urgency;
            if (secsToDdl < minSecsToDdl) {
                minSecsToDdl = secsToDdl;
                nearestDdlText = todo.text + " (" + todo.deadline.toString("MM-dd HH:mm") + ")";
            }
        }
        
        // 紧急度色彩映射
        QColor bgColor;
        if (!c.color.isEmpty()) {
            bgColor = QColor(c.color);
        } else {
            bgColor = QColor(colorPalette[colorIndex % colorPalette.size()]);
            colorIndex++;
        }
        
        if (maxUrgency > 0.01) {
            QColor urgentColor;
            // 夜间模式使用更柔和的红色（如柔和粉红/珊瑚红），以保证白字清晰并且不扎眼
            QColor redColor = m_isDarkMode ? QColor(248, 113, 113) : QColor(239, 68, 68); 
            
            // maxUrgency ranges from 0.0 to 1.0 (1.0 = <= 1 day, 0.0 = > 14 days)
            // if DDL is 7 days, maxUrgency is ~ 0.5 * (7/11) ~ 0.31 -> slightly red
            // Blend bgColor to redColor based on maxUrgency
            double t = maxUrgency; // 0 to 1
            
            // Adjust the blend slightly to make it exponential so red comes strongly only near the end
            t = std::pow(t, 1.5); 
            
            urgentColor.setRed(bgColor.red() + t * (redColor.red() - bgColor.red()));
            urgentColor.setGreen(bgColor.green() + t * (redColor.green() - bgColor.green()));
            urgentColor.setBlue(bgColor.blue() + t * (redColor.blue() - bgColor.blue()));
            
            bgColor = urgentColor;
        }

        // 合并课程名和地点
        QString text = QString("%1\n📍 %2").arg(c.name, c.location);
        
        bool matchWeek = (c.weekType == Course::All) || 
                         (c.weekType == Course::Odd && isCurrentWeekOdd) || 
                         (c.weekType == Course::Even && !isCurrentWeekOdd);
        
        if (!matchWeek) {
            // 非本周课程，颜色更淡或灰色叠加
            item->setBackground(QBrush(QColor(m_isDarkMode ? "#27272A" : "#F1F5F9")));
            item->setForeground(QBrush(QColor(m_isDarkMode ? "#A1A1AA" : "#94A3B8")));
            if (c.weekType == Course::Odd) text += "\n(单周)";
            if (c.weekType == Course::Even) text += "\n(双周)";
        } else {
            item->setBackground(QBrush(bgColor));
            if (m_isDarkMode) {
                // 夜间模式统一使用纯白字体，视觉最清晰
                item->setForeground(QBrush(QColor("#FFFFFF")));
            } else {
                int lightness = bgColor.red() * 0.299 + bgColor.green() * 0.587 + bgColor.blue() * 0.114;
                item->setForeground(QBrush(QColor(lightness > 128 ? "#1E293B" : "#FFFFFF")));
            }
        }
        
        // 选中/当前时间段的重点标记
        if (matchWeek && c.day == currentDayOfWeek && 
            currentSection >= c.start && currentSection < c.start + c.duration) {
            // 当前正在上的课增加边框或明显颜色，目前直接用高亮色覆盖
            item->setBackground(QBrush(QColor(m_isDarkMode ? "#B87A3D" : "#FFC88A"))); 
            item->setForeground(QBrush(QColor(m_isDarkMode ? "#000000" : "#000000")));
        }
        
        item->setText(text);
        item->setTextAlignment(Qt::AlignCenter); // 居中更清晰
        
        // Tooltip
        QString tooltip = QString("<b>%1</b><br>地点：%2<br>教师：%3<br>最近任务：%4")
                          .arg(c.name).arg(c.location).arg(c.teacher).arg(nearestDdlText);
        item->setToolTip(tooltip);
        
        if (maxUrgency > globalMaxUrgency) {
            globalMaxUrgency = maxUrgency;
        }
    }
    
    // Update Mascot based on global max urgency
    if (globalMaxUrgency == 0.0) {
        m_mascotBtn->setText("(❁´◡`❁)"); // 0%
        m_mascotBtn->setToolTip("精力充沛！没有任何紧急任务哦~");
    } else if (globalMaxUrgency <= 0.33) {
        m_mascotBtn->setText("(。_。)"); // 1~33%
        m_mascotBtn->setToolTip("嗯...任务开始出现了，稍微有点担心");
    } else if (globalMaxUrgency <= 0.66) {
        m_mascotBtn->setText("(;;;*_*)"); // 34~66%
        m_mascotBtn->setToolTip("哇啊！任务堆积，有种不妙的预感！");
    } else {
        m_mascotBtn->setText("(×_×)"); // 67~100%
        m_mascotBtn->setToolTip("我不行了... DDL就在眼前...");
    }
}

void MainWindow::onCellDoubleClicked(int row, int column)
{
    int day = column + 1;
    int section = row + 1;
    
    for (int i = 0; i < m_courses.size(); ++i) {
        const Course &c = m_courses[i];
        if (c.day == day && section >= c.start && section < c.start + c.duration) {
             showCourseDetailsDialog(i);
             return;
        }
    }
    
    // 如果该时间段没有课程，提示是否新建课程
    QMessageBox::StandardButton reply = QMessageBox::question(this, "添加课程", "当前时间段没有课程，是否要在此处添加新课程？",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        Course newCourse{"新建课程", "地点", "待定", QDateTime(), "", day, section, 2, Course::All, "", {}};
        m_courses.append(newCourse);
        showCourseDetailsDialog(m_courses.size() - 1);
        refreshSchedule();
    }
}

void MainWindow::showCourseDetailsDialog(int courseIndex)
{
    if (courseIndex < 0 || courseIndex >= m_courses.size()) return;
    
    Course &c = m_courses[courseIndex];
    
    QDialog dialog(this);
    dialog.setWindowTitle(QString("课程详情 - %1").arg(c.name));
    dialog.resize(550, 450);
    
    // UI Style applying
    if (m_isDarkMode) {
        dialog.setStyleSheet("QDialog { background-color: #18181A; }"
                             "QLabel { color: #E4E4E7; font-size: 14px; }"
                             "QTabWidget::pane { border: 1px solid #3F3F46; border-radius: 8px; }"
                             "QTabBar::tab { background: #27272A; color: #A1A1AA; border: 1px solid #3F3F46; padding: 10px 20px; border-top-left-radius: 8px; border-top-right-radius: 8px; }"
                             "QTabBar::tab:selected { background: #3B82F6; color: white; }"
                             "QLineEdit, QDateTimeEdit { padding: 8px; border: 1px solid #52525B; border-radius: 6px; background-color: #27272A; color: #E4E4E7; font-size: 14px; }"
                             "QPushButton { padding: 10px; border-radius: 8px; background-color: #3B82F6; color: white; font-weight: bold; } "
                             "QPushButton:hover { background-color: #60A5FA; }");
    } else {
        dialog.setStyleSheet("QDialog { background-color: #F8FAFC; }"
                             "QLabel { color: #1E293B; font-size: 14px; }"
                             "QTabWidget::pane { border: 1px solid #E2E8F0; border-radius: 8px; }"
                             "QTabBar::tab { background: #E2E8F0; color: #64748B; border: 1px solid #CBD5E1; padding: 10px 20px; border-top-left-radius: 8px; border-top-right-radius: 8px; }"
                             "QTabBar::tab:selected { background: #2563EB; color: white; }"
                             "QLineEdit, QDateTimeEdit { padding: 8px; border: 1px solid #E2E8F0; border-radius: 6px; background-color: #FFFFFF; color: #1E293B; font-size: 14px; }"
                             "QPushButton { padding: 10px; border-radius: 8px; background-color: #2563EB; color: white; font-weight: bold; } "
                             "QPushButton:hover { background-color: #3B82F6; }");
    }
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    QTabWidget *tabWidget = new QTabWidget(&dialog);
    
    // ================= 1. 基本信息区 =================
    QWidget *infoTab = new QWidget();
    QVBoxLayout *infoMainLayout = new QVBoxLayout(infoTab);
    infoMainLayout->setContentsMargins(15, 20, 15, 15);
    QFormLayout *infoLayout = new QFormLayout();
    infoLayout->setSpacing(15);
    
    QLineEdit *nameEdit = new QLineEdit(c.name);
    QLineEdit *teacherEdit = new QLineEdit(c.teacher);
    QLineEdit *locationEdit = new QLineEdit(c.location);
    QLineEdit *folderEdit = new QLineEdit(c.folderPath);
    
    QComboBox *dayCombo = new QComboBox();
    dayCombo->addItems({"周一", "周二", "周三", "周四", "周五", "周六", "周日"});
    dayCombo->setCurrentIndex(c.day - 1);
    
    QComboBox *startCombo = new QComboBox();
    for (int i = 1; i <= 12; ++i) startCombo->addItem(QString("第 %1 节").arg(i));
    startCombo->setCurrentIndex(c.start - 1);
    
    QComboBox *durationCombo = new QComboBox();
    for (int i = 1; i <= 4; ++i) durationCombo->addItem(QString::number(i) + " 节课");
    durationCombo->setCurrentIndex(c.duration - 1);
    
    QComboBox *weekTypeCombo = new QComboBox();
    weekTypeCombo->addItems({"全周", "单周", "双周"});
    weekTypeCombo->setCurrentIndex(c.weekType);
    
    QDateTimeEdit *examEdit = new QDateTimeEdit(c.examTime.isValid() ? c.examTime : QDateTime::currentDateTime());
    examEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    examEdit->setCalendarPopup(true);
    
    infoLayout->addRow("课程名称:", nameEdit);
    infoLayout->addRow("授课教师:", teacherEdit);
    infoLayout->addRow("上课地点:", locationEdit);
    
    QHBoxLayout *timeLayout = new QHBoxLayout();
    timeLayout->addWidget(dayCombo);
    timeLayout->addWidget(startCombo);
    timeLayout->addWidget(durationCombo);
    infoLayout->addRow("上课时间:", timeLayout);
    infoLayout->addRow("奇偶周:", weekTypeCombo);
    
    infoLayout->addRow("考试时间:", examEdit);
    infoLayout->addRow("本地目录:", folderEdit);
    
    infoMainLayout->addLayout(infoLayout);
    infoMainLayout->addStretch();
    
    QHBoxLayout *saveLayout = new QHBoxLayout();
    QPushButton *saveInfoBtn = new QPushButton("保存基本信息");
    saveInfoBtn->setCursor(Qt::PointingHandCursor);
    
    QPushButton *deleteBtn = new QPushButton("删除本课程");
    deleteBtn->setCursor(Qt::PointingHandCursor);
    if (m_isDarkMode) {
        deleteBtn->setStyleSheet("background-color: #EF4444; color: white; border-radius: 8px; padding: 10px; font-weight: bold;");
    } else {
        deleteBtn->setStyleSheet("background-color: #EF4444; color: white; border-radius: 8px; padding: 10px; font-weight: bold;");
    }
    
    saveLayout->addWidget(saveInfoBtn);
    saveLayout->addWidget(deleteBtn);
    infoMainLayout->addLayout(saveLayout);
    
    connect(deleteBtn, &QPushButton::clicked, [&]() {
        QMessageBox::StandardButton rep = QMessageBox::warning(&dialog, "确认删除", "确定彻底删除这门课程及所有待办吗？", QMessageBox::Yes | QMessageBox::No);
        if (rep == QMessageBox::Yes) {
            m_courses.removeAt(courseIndex);
            refreshSchedule();
            refreshTodoList();
            dialog.accept();
        }
    });
    
    connect(saveInfoBtn, &QPushButton::clicked, [&]() {
        // 获取新的课程信息
        QString newName = nameEdit->text();
        QString newTeacher = teacherEdit->text();
        QString newLocation = locationEdit->text();
        int newDay = dayCombo->currentIndex() + 1;
        int newStart = startCombo->currentIndex() + 1;
        int newDuration = durationCombo->currentIndex() + 1;
        auto newWeekType = (Course::WeekType)weekTypeCombo->currentIndex();
        QDateTime newExamTime = examEdit->dateTime();
        QString newFolderPath = folderEdit->text();
        
        // 检测时间冲突
        bool hasConflict = false;
        for (int i = 0; i < m_courses.size(); ++i) {
            Course &other = m_courses[i];
            if (&other == &c) {
                continue; // 跳过当前正在编辑的课程
            }
            if (other.day != newDay) {
                continue; // 不是同一天，无冲突
            }
            // 计算当前课程的时间段 [newStart, newStart + newDuration - 1]
            int curEnd = newStart + newDuration - 1;
            // 计算其他课程的时间段 [other.start, other.start + other.duration - 1]
            int otherEnd = other.start + other.duration - 1;
            // 检查是否有重叠
            if (newStart <= otherEnd && other.start <= curEnd) {
                hasConflict = true;
                break;
            }
        }
        
        if (hasConflict) {
            QMessageBox::warning(&dialog, "时间冲突", "该课程时间与现有课程冲突，请调整上课时间！");
            return;
        }
        
        // 保存课程信息
        c.name = newName;
        c.teacher = newTeacher;
        c.location = newLocation;
        c.day = newDay;
        c.start = newStart;
        c.duration = newDuration;
        c.weekType = newWeekType;
        c.examTime = newExamTime;
        c.folderPath = newFolderPath;
        
        refreshSchedule();
        refreshTodoList();
        QMessageBox::information(&dialog, "成功", "基础信息已保存");
    });
    
    tabWidget->addTab(infoTab, "基本信息");
    
    // ================= 2. 待办事项 =================
    QWidget *todoTab = new QWidget();
    QVBoxLayout *todoLayout = new QVBoxLayout(todoTab);
    todoLayout->setContentsMargins(15, 15, 15, 15);
    todoLayout->setSpacing(12);
    
    QListWidget *courseTodoList = new QListWidget();
    if (m_isDarkMode) {
        courseTodoList->setStyleSheet("background-color: transparent; border: 1px solid #3F3F46; border-radius: 8px; color: #E4E4E7;");
    } else {
        courseTodoList->setStyleSheet("background-color: transparent; border: 1px solid #E2E8F0; border-radius: 8px; color: #1E293B;");
    }
    
    auto reloadTodos = [&]() {
        courseTodoList->clear();
        for (int i = 0; i < c.todos.size(); ++i) {
            TodoItem &t = c.todos[i];
            QString text = QString("[%1] %2 - %3")
                .arg(t.isCompleted ? "✔" : " ")
                .arg(t.text)
                .arg(t.deadline.toString("MM-dd HH:mm"));
            QListWidgetItem *item = new QListWidgetItem(text);
            item->setData(Qt::UserRole, i);
            if (t.isCompleted) {
                // Strike-through or greyed out
                QFont f = item->font();
                f.setStrikeOut(true);
                item->setFont(f);
                item->setForeground(QBrush(QColor(m_isDarkMode ? "#52525B" : "#94A3B8")));
            } else {
                item->setForeground(QBrush(QColor(m_isDarkMode ? "#E4E4E7" : "#1E293B")));
            }
            item->setSizeHint(QSize(0, 44)); // Make items taller for better tap targets and looks
            courseTodoList->addItem(item);
        }
    };
    reloadTodos();
    
    QHBoxLayout *todoOpLayout = new QHBoxLayout();
    todoOpLayout->setSpacing(12);
    QPushButton *toggleTodoBtn = new QPushButton("切换完成状态");
    QPushButton *delTodoBtn = new QPushButton("删除选中");
    toggleTodoBtn->setCursor(Qt::PointingHandCursor);
    delTodoBtn->setCursor(Qt::PointingHandCursor);
    
    if (m_isDarkMode) {
        delTodoBtn->setStyleSheet("background-color: #EF4444; color: white;");
    } else {
        delTodoBtn->setStyleSheet("background-color: #EF4444; color: white;");
    }
    
    todoOpLayout->addWidget(toggleTodoBtn);
    todoOpLayout->addWidget(delTodoBtn);
    
    connect(toggleTodoBtn, &QPushButton::clicked, [&]() {
        if (auto item = courseTodoList->currentItem()) {
            int idx = item->data(Qt::UserRole).toInt();
            c.todos[idx].isCompleted = !c.todos[idx].isCompleted;
            reloadTodos();
            refreshTodoList();
            refreshSchedule();
        }
    });
    connect(delTodoBtn, &QPushButton::clicked, [&]() {
        if (auto item = courseTodoList->currentItem()) {
            int idx = item->data(Qt::UserRole).toInt();
            c.todos.removeAt(idx);
            reloadTodos();
            refreshTodoList();
            refreshSchedule();
        }
    });
    
    todoLayout->addWidget(courseTodoList);
    todoLayout->addLayout(todoOpLayout);
    tabWidget->addTab(todoTab, "待办事项");
    
    // ================= 3. 课程资料 =================
    QWidget *materialTab = new QWidget();
    QVBoxLayout *materialLayout = new QVBoxLayout(materialTab);
    
    QPushButton *openFolderBtn = new QPushButton("📂 打开课程文件夹");
    QPushButton *createNoteBtn = new QPushButton("📄 新建笔记 (按时间戳)");
    
    connect(openFolderBtn, &QPushButton::clicked, [&]() {
        if (c.folderPath.isEmpty()) {
            QMessageBox::warning(&dialog, "未设置", "请先在基本信息中设置本地目录路径");
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(c.folderPath));
    });
    
    connect(createNoteBtn, &QPushButton::clicked, [&]() {
        if (c.folderPath.isEmpty()) {
            QMessageBox::warning(&dialog, "未设置", "请先在基本信息中设置本地目录路径");
            return;
        }
        QDir dir(c.folderPath);
        if (!dir.exists()) dir.mkpath(".");
        QString filename = "Note_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt";
        QFile file(dir.filePath(filename));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "Title: " << c.name << " Note\nDate: " << QDateTime::currentDateTime().toString() << "\n\n";
            file.close();
            QMessageBox::information(&dialog, "成功", "已创建笔记文件: " + filename);
            QDesktopServices::openUrl(QUrl::fromLocalFile(file.fileName()));
        }
    });
    
    materialLayout->addWidget(openFolderBtn);
    materialLayout->addWidget(createNoteBtn);
    materialLayout->addStretch();
    tabWidget->addTab(materialTab, "课程资料");
    
    // ================= 4. 统计卡片 =================
    QWidget *statTab = new QWidget();
    QVBoxLayout *statLayout = new QVBoxLayout(statTab);
    
    int total = c.todos.size();
    int completed = 0;
    for (const auto &t : c.todos) {
        if (t.isCompleted) completed++;
    }
    
    QLabel *statLabel = new QLabel(QString("📊 任务完成度: %1 / %2").arg(completed).arg(total));
    statLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setMaximum(total > 0 ? total : 1);
    progressBar->setValue(completed);
    
    statLayout->addWidget(statLabel);
    statLayout->addWidget(progressBar);
    statLayout->addStretch();
    tabWidget->addTab(statTab, "统计面板");
    
    mainLayout->addWidget(tabWidget);
    dialog.exec();
}

void MainWindow::showDashboardDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("全局学习数据看板");
    dialog.resize(500, 300);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    int totalTasks = 0;
    int completedTasks = 0;
    int overdueTasks = 0;
    QDateTime now = QDateTime::currentDateTime();
    
    for (const auto &c : m_courses) {
        for (const auto &t : c.todos) {
            totalTasks++;
            if (t.isCompleted) completedTasks++;
            else if (now.secsTo(t.deadline) < 0) overdueTasks++;
        }
    }
    
    QLabel *title = new QLabel("📈 你的本周/全局状态");
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    
    QLabel *tCount = new QLabel(QString("总任务数: %1").arg(totalTasks));
    QLabel *cCount = new QLabel(QString("已完成数: %1").arg(completedTasks));
    QLabel *oCount = new QLabel(QString("逾期任务: %1").arg(overdueTasks));
    if (overdueTasks > 0) oCount->setStyleSheet("color: red; font-weight: bold;");
    
    // Suggestion box
    QLabel *sugTitle = new QLabel("\n💡 智能任务建议:");
    sugTitle->setStyleSheet("font-weight: bold;");
    
    QString bestTask;
    qint64 minSecs = std::numeric_limits<qint64>::max();
    for (const auto &c : m_courses) {
        for (const auto &t : c.todos) {
            if (!t.isCompleted && now.secsTo(t.deadline) >= 0) {
                if (now.secsTo(t.deadline) < minSecs) {
                    minSecs = now.secsTo(t.deadline);
                    bestTask = c.name + " - " + t.text;
                }
            }
        }
    }
    
    QLabel *suggestion = new QLabel;
    if (minSecs != std::numeric_limits<qint64>::max()) {
        suggestion->setText(QString("建议你现在优先处理：<b>%1</b>，赶在 DDL 前完成！").arg(bestTask));
    } else {
        suggestion->setText("太棒了！当前没有任何未完成的任务。好好休息吧！");
    }
    suggestion->setWordWrap(true);
    
    layout->addWidget(title);
    layout->addWidget(tCount);
    layout->addWidget(cCount);
    layout->addWidget(oCount);
    layout->addWidget(sugTitle);
    layout->addWidget(suggestion);
    layout->addStretch();
    
    dialog.exec();
}

void MainWindow::onCellClicked(int row, int column)
{
    m_currentDay = column + 1;      // 记录点击的星期几
    m_currentSection = row + 1;     // 记录点击的第几节课

    int foundIndex = -1;            // 用来保存找到的课程在 m_courses 中的下标
    for (int i = 0; i < m_courses.size(); ++i) {
        const Course &c = m_courses[i];
        if (c.day == m_currentDay && m_currentSection >= c.start && m_currentSection < c.start + c.duration) {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex != -1) {         // 找到了课程
        m_todoInput->setEnabled(true);
        m_todoDateInput->setEnabled(true);
        m_addTodoBtn->setEnabled(true);
        // 使用刚才记录的索引来获取课程名
        m_todoInput->setPlaceholderText(
            QString("为 [%1] 填写待办任务...").arg(m_courses[foundIndex].name)
        );
        m_todoDateInput->setDateTime(QDateTime::currentDateTime().addDays(1));
    } else {                        // 没有找到课程
        m_todoInput->setEnabled(false);
        m_todoDateInput->setEnabled(false);
        m_addTodoBtn->setEnabled(false);
        m_todoInput->setPlaceholderText("请先点击左侧课表选择一门课程...");
    }

    refreshTodoList();
}

void MainWindow::refreshTodoList()
{
    m_todoTableWidget->setRowCount(0); // clear
    
    // Refresh course filter items without triggering changed signals
    m_todoCourseFilterCombo->blockSignals(true);
    QString currentText = m_todoCourseFilterCombo->currentText();
    QStringList courseNames;
    courseNames << "所有课程";
    for (const auto &c : m_courses) {
        if (!courseNames.contains(c.name)) {
            courseNames << c.name;
        }
    }
    m_todoCourseFilterCombo->clear();
    m_todoCourseFilterCombo->addItems(courseNames);
    int idx = m_todoCourseFilterCombo->findText(currentText);
    if (idx >= 0) m_todoCourseFilterCombo->setCurrentIndex(idx);
    m_todoCourseFilterCombo->blockSignals(false);

    struct FlatTodo {
        Course* course;
        int todoIndex;
    };
    QList<FlatTodo> allTodos;
    
    QString selectedCourseFilter = m_todoCourseFilterCombo->currentText();
    int timeFilterIndex = m_todoTimeFilterCombo->currentIndex(); // 0: 全部, 1: 今天, 2: 本周, 3: 已逾期
    QDateTime now = QDateTime::currentDateTime();
    QDate today = now.date();
    
    // Collect all todos applying filters
    for (int cIdx = 0; cIdx < m_courses.size(); ++cIdx) {
        Course &c = m_courses[cIdx];
        if (selectedCourseFilter != "所有课程" && selectedCourseFilter != c.name) continue;
        
        for (int tIdx = 0; tIdx < c.todos.size(); ++tIdx) {
            TodoItem &todo = c.todos[tIdx];
            
            // Time filters
            if (timeFilterIndex == 1) { // 今天
                if (todo.deadline.date() != today) continue;
            } else if (timeFilterIndex == 2) { // 本周
                if (today.daysTo(todo.deadline.date()) > 7 || today.daysTo(todo.deadline.date()) < 0) continue;
            } else if (timeFilterIndex == 3) { // 已逾期
                if (now.secsTo(todo.deadline) >= 0 || todo.isCompleted) continue;
            }
            
            allTodos.append({&c, tIdx});
        }
    }
    
    int sortType = m_todoSortCombo->currentIndex(); // 0:默认, 1:按课程, 2:按截止时间按早到晚, 3:按紧急度
    
    // Sort all todos
    std::sort(allTodos.begin(), allTodos.end(), [sortType, now](const FlatTodo &a, const FlatTodo &b) {
        const TodoItem &ta = a.course->todos[a.todoIndex];
        const TodoItem &tb = b.course->todos[b.todoIndex];
        
        if (sortType == 1) {
            if (a.course->name != b.course->name) return a.course->name < b.course->name;
            return ta.deadline < tb.deadline; // Same course, earlier deadline first
        } else if (sortType == 2) {
            if (ta.isCompleted != tb.isCompleted) return !ta.isCompleted;
            return ta.deadline < tb.deadline;
        } else if (sortType == 3) {
            if (ta.isCompleted != tb.isCompleted) return !ta.isCompleted;
            // Urgency: closer to now is more urgent. Overdue is maximum urgent.
            qint64 secsA = now.secsTo(ta.deadline);
            qint64 secsB = now.secsTo(tb.deadline);
            return secsA < secsB;
        } else {
            // Default
            if (ta.isCompleted != tb.isCompleted) return !ta.isCompleted;
            return ta.deadline < tb.deadline;
        }
    });
    
    int row = 0;
    for (const FlatTodo &ft : allTodos) {
        TodoItem &todo = ft.course->todos[ft.todoIndex];
        m_todoTableWidget->insertRow(row);
        
        QTableWidgetItem *taskItem = new QTableWidgetItem(todo.text);
        QTableWidgetItem *courseItem = new QTableWidgetItem(ft.course->name);
        QTableWidgetItem *ddlItem = new QTableWidgetItem(todo.deadline.toString("MM-dd HH:mm"));
        
        qint64 secsLeft = now.secsTo(todo.deadline);
        QString timeLeftStr;
        if (todo.isCompleted) {
            timeLeftStr = "-";
        } else if (secsLeft < 0) {
            timeLeftStr = "已逾期";
        } else if (secsLeft < 3600) {
            timeLeftStr = QString("%1分钟").arg(secsLeft / 60);
        } else if (secsLeft < 86400) {
            timeLeftStr = QString("%1小时").arg(secsLeft / 3600);
        } else {
            timeLeftStr = QString("%1天").arg(secsLeft / 86400);
        }
        QTableWidgetItem *leftItem = new QTableWidgetItem(timeLeftStr);
        
        QTableWidgetItem *statusItem = new QTableWidgetItem(todo.isCompleted ? "☑ 已完成" : "☐ 未完成");
        
        // Store indices 
        statusItem->setData(Qt::UserRole, ft.course->name);
        statusItem->setData(Qt::UserRole + 1, ft.todoIndex);
        
        // Colors
        if (todo.isCompleted) {
            QColor g = Qt::gray;
            taskItem->setForeground(g); courseItem->setForeground(g);
            ddlItem->setForeground(g); leftItem->setForeground(g); statusItem->setForeground(g);
            QFont f = taskItem->font(); f.setStrikeOut(true);
            taskItem->setFont(f);
        } else if (secsLeft < 0) {
            QColor r("#D32F2F"); // overtime red
            leftItem->setForeground(r); statusItem->setForeground(r);
            taskItem->setForeground(r);
        } else if (secsLeft < 86400 * 3) {
            QColor o("#E65100"); // orange
            leftItem->setForeground(o); taskItem->setForeground(o);
        }
        
        m_todoTableWidget->setItem(row, 0, taskItem);
        m_todoTableWidget->setItem(row, 1, courseItem);
        m_todoTableWidget->setItem(row, 2, ddlItem);
        m_todoTableWidget->setItem(row, 3, leftItem);
        m_todoTableWidget->setItem(row, 4, statusItem);
        row++;
    }
}

void MainWindow::onTodoItemClicked(QTableWidgetItem *item)
{
    // We bind cellClicked in constructor instead to handle state
}

void MainWindow::onAddTodoClicked()
{
    if (m_currentDay == -1 || m_currentSection == -1) return;
    
    QString text = m_todoInput->text().trimmed();
    if (text.isEmpty()) return;
    
    for (Course &c : m_courses) {
        if (c.day == m_currentDay && m_currentSection >= c.start && m_currentSection < c.start + c.duration) {
            // 获取选择的 DDL 时间
            QDateTime customDeadline = m_todoDateInput->dateTime();
            c.todos.append({text, customDeadline, false, "", ""});
            m_todoInput->clear();
            m_todoDateInput->setDateTime(QDateTime::currentDateTime().addDays(1)); // Reset logic
            refreshTodoList();
            refreshSchedule(); // 此时更新一下最高紧急度和提示
            break;
        }
    }
}

void MainWindow::updateTime()
{
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("yyyy-MM-dd HH:mm:ss dddd");
    m_timeLabel->setText(QString("🕒 系统时间: %1").arg(timeStr));
}

void MainWindow::onToggleThemeClicked()
{
    m_isDarkMode = !m_isDarkMode;
    applyTheme();
    refreshSchedule(); // redraw schedule with new colors
}

void MainWindow::applyTheme()
{
    QString qss;
    QString fontSet = "system-ui, -apple-system, 'Segoe UI', 'PingFang SC', 'Microsoft YaHei', sans-serif";

    if (m_isDarkMode) {
        qss = QString(R"(
            QMainWindow {
                background-color: #18181A;
            }
            QWidget {
                font-family: %1;
            }
            QTableWidget {
                background-color: transparent;
                border: none;
                gridline-color: #52525B;
                outline: none;
                font-size: 14px;
            }
            QTableWidget::item {
                padding: 10px;
                border: 6px solid #18181A;
                border-radius: 16px;
            }
            QTableWidget::item:selected {
                background-color: rgba(255, 255, 255, 0.15); 
                color: #FFFFFF;
                border: 2px solid #5C9DFF;
                border-radius: 12px;
            }
            QHeaderView::section {
                background-color: transparent;
                color: #A1A1AA;
                font-weight: 600;
                font-size: 14px;
                border: none;
                padding: 6px;
            }
            QHeaderView {
                background-color: transparent;
            }
            QListWidget {
                background-color: #27272A;
                border: none;
                border-radius: 10px;
                font-size: 14px;
                color: #E4E4E7;
            }
            QListWidget::item {
                padding: 14px;
                border-bottom: 1px solid #3F3F46;
            }
            QListWidget::item:hover {
                background-color: #3F3F46;
                border-radius: 8px;
            }
            QLineEdit, QDateTimeEdit {
                border: 1px solid #52525B;
                border-radius: 8px;
                padding: 8px;
                font-size: 14px;
                background-color: #27272A;
                color: #E4E4E7;
            }
            QPushButton#addBtn {
                background-color: #3B82F6;
                color: white;
                border-radius: 8px;
                padding: 8px 16px;
                font-weight: 600;
            }
            QPushButton#addBtn:hover {
                background-color: #60A5FA;
            }
            QToolBar {
                background: transparent;
                border: none;
                spacing: 15px;
                padding: 10px;
            }
            QDockWidget {
                font-weight: 600;
                font-size: 15px;
                color: #E4E4E7;
            }
            QDockWidget::title {
                text-align: left;
                background: transparent;
                padding: 10px;
            }
            QLabel#weekInfoLabel {
                font-size: 20px;
                font-weight: bold;
                color: #E4E4E7;
                padding: 5px 5px;
            }
            QLabel#weekStateLabel {
                font-size: 20px;
                font-weight: bold;
                color: #F87171;
                padding: 0px 5px;
            }
            QLabel#timeLabel {
                font-size: 15px;
                font-weight: 600;
                color: #A1A1AA;
                padding: 5px 15px;
            }
        )").arg(fontSet);
    } else {
        qss = QString(R"(
            QMainWindow {
                background-color: #F8FAFC;
            }
            QWidget {
                font-family: %1;
            }
            QTableWidget {
                background-color: transparent;
                border: none;
                gridline-color: #CBD5E1;
                outline: none;
                font-size: 14px;
            }
            QTableWidget::item {
                padding: 10px;
                border: 6px solid #F8FAFC;
                border-radius: 16px;
            }
            QTableWidget::item:selected {
                background-color: rgba(0, 0, 0, 0.05); 
                color: #1E293B;
                border: 2px solid #3B82F6;
                border-radius: 12px;
            }
            QHeaderView::section {
                background-color: transparent;
                color: #64748B;
                font-weight: 600;
                font-size: 14px;
                border: none;
                padding: 6px;
            }
            QHeaderView {
                background-color: transparent;
            }
            QListWidget {
                background-color: #FFFFFF;
                border: none;
                border-radius: 10px;
                font-size: 14px;
                color: #1E293B;
            }
            QListWidget::item {
                padding: 14px;
                border-bottom: 1px solid #F1F5F9;
            }
            QListWidget::item:hover {
                background-color: #F8FAFC;
                border-radius: 8px;
            }
            QLineEdit, QDateTimeEdit {
                border: 1px solid #E2E8F0;
                border-radius: 8px;
                padding: 8px;
                font-size: 14px;
                background-color: #FFFFFF;
                color: #1E293B;
            }
            QPushButton#addBtn {
                background-color: #2563EB;
                color: white;
                border-radius: 8px;
                padding: 8px 16px;
                font-weight: 600;
            }
            QPushButton#addBtn:hover {
                background-color: #3B82F6;
            }
            QToolBar {
                background: transparent;
                border: none;
                spacing: 15px;
                padding: 10px;
            }
            QDockWidget {
                font-weight: 600;
                font-size: 15px;
                color: #1E293B;
            }
            QDockWidget::title {
                text-align: left;
                background: transparent;
                padding: 10px;
            }
            QLabel#weekInfoLabel {
                font-size: 20px;
                font-weight: bold;
                color: #1E293B;
                padding: 5px 5px;
            }
            QLabel#weekStateLabel {
                font-size: 20px;
                font-weight: bold;
                color: #EF4444;
                padding: 0px 5px;
            }
            QLabel#timeLabel {
                font-size: 15px;
                font-weight: 600;
                color: #64748B;
                padding: 5px 15px;
            }
        )").arg(fontSet);
    }
    
    qApp->setStyleSheet(qss);
}
