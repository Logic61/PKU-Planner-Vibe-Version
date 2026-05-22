#include "todopage.h"
#include "../models/datamanager.h"
#include "../components/taskcardwidget.h"
#include "../components/toastwidget.h"
#include "../components/emptystatewidget.h"
#include "../dialogs/taskeditdialog.h"
#include "../dialogs/confirmdialog.h"
#include "../ui/theme.h"
#include "../services/teachingplatformservice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QApplication>
#include <QScrollArea>
#include <QToolButton>
#include <QTimer>
#include <QDate>
#include <QDialog>
#include <algorithm>

namespace {

bool matchesFilters(const Task &task,
                    const QString &courseName,
                    const QString &timeFilter,
                    const QString &statusFilter,
                    const QString &keyword,
                    bool hideCompleted)
{
    if (hideCompleted && task.completed) {
        return false;
    }

    if (!courseName.isEmpty() && courseName != "全部课程" && task.course != courseName) {
        return false;
    }

    if (!statusFilter.isEmpty() && statusFilter != "全部状态") {
        if (statusFilter == "未完成" && task.completed) {
            return false;
        }
        if (statusFilter == "已完成" && !task.completed) {
            return false;
        }
    }

    if (!timeFilter.isEmpty() && timeFilter != "全部时间") {
        if (timeFilter == "今天" && task.deadline.date() != QDate::currentDate()) {
            return false;
        }

        if (timeFilter == "本周") {
            int currentYear = 0;
            int deadlineYear = 0;
            const int currentWeek = QDate::currentDate().weekNumber(&currentYear);
            const int deadlineWeek = task.deadline.date().weekNumber(&deadlineYear);
            if (currentWeek != deadlineWeek || currentYear != deadlineYear) {
                return false;
            }
        }

        if (timeFilter == "逾期" && !task.isOverdue()) {
            return false;
        }
    }

    const QString lowered = keyword.trimmed().toLower();
    if (!lowered.isEmpty()) {
        if (!task.course.toLower().contains(lowered) && !task.title.toLower().contains(lowered)) {
            return false;
        }
    }

    return true;
}

QFrame *makeSectionHeader(const QString &title, int count, const QString &accent)
{
    QFrame *header = new QFrame;
    header->setObjectName("sectionHeader");
    header->setStyleSheet(QString(R"(
        QFrame#sectionHeader {
            background: white;
            border: 1px solid #F0E6E6;
            border-radius: 14px;
        }
        QLabel#sectionTitle {
            color: %1;
            font-size: 15px;
            font-weight: 700;
        }
        QLabel#sectionCount {
            color: #777;
            font-size: 12px;
            font-weight: 600;
        }
    )").arg(accent));

    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(10);

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setObjectName("sectionTitle");
    QLabel *countLabel = new QLabel(QString::number(count));
    countLabel->setObjectName("sectionCount");

    layout->addWidget(titleLabel);
    layout->addWidget(countLabel);
    layout->addStretch();
    return header;
}

QList<TaskViewModel> toViewItems(const QList<Task> &tasks)
{
    QList<TaskViewModel> items;
    items.reserve(tasks.size());
    for (int i = 0; i < tasks.size(); ++i) {
        items.append({tasks[i], i});
    }
    return items;
}

}

TodoPage::TodoPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(QString("background:%1; font-family: 'Microsoft YaHei','Segoe UI', Arial; color: %2; font-weight:500;")
    .arg(Theme::BACKGROUND).arg(Theme::TEXT_PRIMARY));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    QFrame *hero = new QFrame;
    hero->setStyleSheet(QString(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, stop:0 %1, stop:1 %2);
            border-radius: %3px;
        }
    )").arg(Theme::PRIMARY).arg("#B44B5D").arg(Theme::CARD_RADIUS));
    QVBoxLayout *heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(20, 18, 20, 18);

    QLabel *heroTitle = new QLabel("待办任务");
    heroTitle->setStyleSheet("color:white; font-size:22px; font-weight:700;");
    QLabel *heroSubtitle = new QLabel("卡片流展示、分组管理、快速完成与编辑");
    heroSubtitle->setStyleSheet("color:rgba(255,255,255,0.82); font-size:12px;");
    heroLayout->addWidget(heroTitle);
    heroLayout->addWidget(heroSubtitle);
    mainLayout->addWidget(hero);

    QFrame *filterCard = new QFrame;
    filterCard->setStyleSheet(QString("background:%1; border-radius:%2px;")
        .arg(Theme::CARD_BG).arg(Theme::CARD_RADIUS));
    QVBoxLayout *filterLayout = new QVBoxLayout(filterCard);
    filterLayout->setContentsMargins(12, 12, 12, 12);
    filterLayout->setSpacing(10);
    filterLayout->addWidget(createFilterBar());

    summaryLabel = new QLabel;
    summaryLabel->setStyleSheet(QString("color:%1; font-size:12px; font-weight:600;").arg(Theme::TEXT_SECONDARY));
    filterLayout->addWidget(summaryLabel);
    mainLayout->addWidget(filterCard);

    scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background:transparent;");

    boardWidget = new QWidget;
    boardWidget->setStyleSheet("background:transparent;");
    boardLayout = new QVBoxLayout(boardWidget);
    boardLayout->setContentsMargins(0, 0, 0, 0);
    boardLayout->setSpacing(12);
    scrollArea->setWidget(boardWidget);
    mainLayout->addWidget(scrollArea, 1);

    setupUI();

    emptyStateWidget = new EmptyStateWidget;
    emptyStateWidget->setContent("🎉", "今日没有待办", "去未名湖散步吧");
    emptyStateWidget->hide();
    connect(emptyStateWidget, &EmptyStateWidget::buttonClicked, this, [this](){
        TaskEditDialog dialog(this, "");
        if (dialog.exec() == QDialog::Accepted) {
            Task t;
            t.course = dialog.getCourseName();
            t.title = dialog.getTitle();
            t.deadline = dialog.getDeadline();
            t.priority = dialog.getPriority();
            t.completed = false;
            DataManager::instance().addTask(t);
        }
    });
    mainLayout->addWidget(emptyStateWidget, 1);

    // 先连接信号
    connect(&DataManager::instance(), &DataManager::coursesChanged, this, &TodoPage::refreshCourseFilter);
    connect(&DataManager::instance(), &DataManager::tasksChanged, this, &TodoPage::refreshTasks);

    // 然后延迟刷新，确保 DataManager 数据已加载
    QTimer::singleShot(50, this, [this]() {
        refreshCourseFilter();
        refreshTasks();
    });
}

QWidget* TodoPage::createFilterBar()
{
    QWidget *bar = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText("搜索任务...");
    searchEdit->setClearButtonEnabled(true);

    courseFilter = new QComboBox;
    courseFilter->addItems({"全部课程"});

    timeFilter = new QComboBox;
    timeFilter->addItems({"全部时间", "今天", "本周", "逾期"});

    statusFilter = new QComboBox;
    statusFilter->addItems({"全部状态", "未完成", "已完成"});

    sortCombo = new QComboBox;
    sortCombo->addItems({"按截止时间", "按优先级"});
    sortCombo->setToolTip("排序方式");

    hideCompletedCheck = new QCheckBox("隐藏已完成");
    hideCompletedCheck->setToolTip("隐藏已完成的任务");

    QPushButton *refreshButton = new QPushButton("🔄 刷新");
    refreshButton->setStyleSheet(QString(R"(
        QPushButton {
            background: linear-gradient(135deg, %1, #B44B5D);
            color: white;
            border: none;
            border-radius: 12px;
            padding: 10px 20px;
            font-weight: 700;
            font-size: 13px;
        }
        QPushButton:hover {
            background: linear-gradient(135deg, %2, #A63D4F);
            box-shadow: 0 4px 12px rgba(139, 30, 45, 0.3);
        }
        QPushButton:pressed {
            background: linear-gradient(135deg, %3, #963547);
        }
    )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK).arg("#6A1620"));

    for (QComboBox *box : {courseFilter, timeFilter, statusFilter}) {
        box->setStyleSheet(QString(R"(
            QComboBox {
                border: 2px solid #E8D9DB;
                border-radius: 12px;
                padding: 8px 12px 8px 14px;
                background: white;
                color: #222;
                font-size: 13px;
                font-weight: 600;
                selection-background-color: %1;
            }
            QComboBox:hover {
                border: 2px solid %1;
                background: #FAFAFA;
            }
            QComboBox:focus {
                border: 2px solid %1;
                background: white;
                outline: none;
            }
            QComboBox::drop-down {
                border: none;
                width: 24px;
                subcontrol-position: right;
                subcontrol-origin: padding;
            }
            QComboBox::down-arrow {
                image: url(:/icons/dropdown_arrow.png);
                width: 10px;
                height: 10px;
            }
            QComboBox QAbstractItemView {
                border: 1px solid #E8D9DB;
                background-color: white;
                color: #222;
                selection-background-color: %1;
                selection-color: white;
                border-radius: 8px;
                outline: none;
            }
            QComboBox QAbstractItemView::item {
                padding: 6px 12px;
                height: 30px;
            }
            QComboBox QAbstractItemView::item:hover {
                background-color: %2;
            }
            QComboBox QAbstractItemView::item:selected {
                background-color: %1;
                color: white;
            }
        )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_LIGHT));
    }

    searchEdit->setStyleSheet(QString(R"(
        QLineEdit {
            border: 2px solid #E8D9DB;
            border-radius: 12px;
            padding: 10px 14px;
            background: white;
            color: #222;
            font-size: 13px;
            font-weight: 600;
            selection-background-color: %1;
        }
        QLineEdit:hover {
            border: 2px solid %1;
            background: #FAFAFA;
        }
        QLineEdit:focus {
            border: 2px solid %1;
            background: white;
            outline: none;
        }
    )").arg(Theme::PRIMARY));

    layout->addWidget(searchEdit, 2);
    layout->addWidget(courseFilter, 1);
    layout->addWidget(timeFilter, 1);
    layout->addWidget(statusFilter, 1);
    layout->addWidget(sortCombo, 1);
    layout->addWidget(hideCompletedCheck);
    layout->addWidget(refreshButton);

    connect(searchEdit, &QLineEdit::textChanged, this, &TodoPage::applyFilter);
    connect(courseFilter, &QComboBox::currentTextChanged, this, &TodoPage::applyFilter);
    connect(timeFilter, &QComboBox::currentTextChanged, this, &TodoPage::applyFilter);
    connect(statusFilter, &QComboBox::currentTextChanged, this, &TodoPage::applyFilter);
    connect(sortCombo, &QComboBox::currentTextChanged, this, &TodoPage::applyFilter);
    connect(hideCompletedCheck, &QCheckBox::toggled, this, &TodoPage::applyFilter);
    connect(refreshButton, &QPushButton::clicked, this, &TodoPage::refreshTasks);

    return bar;
}

QWidget* TodoPage::createSectionHeader(const QString &title, int count, const QString &accent)
{
    return makeSectionHeader(title, count, accent);
}

void TodoPage::refreshCourseFilter()
{
    const QString current = courseFilter ? courseFilter->currentText() : QString();
    if (!courseFilter) {
        return;
    }

    courseFilter->blockSignals(true);
    courseFilter->clear();
    courseFilter->addItem("全部课程");

    for (const Course &course : DataManager::instance().courses()) {
        if (!course.name.isEmpty()) {
            courseFilter->addItem(course.name);
        }
    }

    const int index = courseFilter->findText(current);
    courseFilter->setCurrentIndex(index >= 0 ? index : 0);
    courseFilter->blockSignals(false);
    applyFilter();
}

void TodoPage::refreshTasks()
{
    applyFilter();
}

void TodoPage::reloadTasks()
{
    refreshCourseFilter();
    refreshTasks();
}

void TodoPage::applyFilter()
{
    while (QLayoutItem *item = boardLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    const QList<Task> tasks = DataManager::instance().tasks();
    QList<TaskViewModel> visible = toViewItems(tasks);

    visible.erase(std::remove_if(visible.begin(), visible.end(), [this](const TaskViewModel &item) {
        return !matchesFilters(item.task,
                               courseFilter ? courseFilter->currentText() : QString(),
                               timeFilter ? timeFilter->currentText() : QString(),
                               statusFilter ? statusFilter->currentText() : QString(),
                               searchEdit ? searchEdit->text() : QString(),
                               hideCompletedCheck ? hideCompletedCheck->isChecked() : false);
    }), visible.end());

    std::sort(visible.begin(), visible.end(), [this](const TaskViewModel &left, const TaskViewModel &right) {
        if (left.task.completed != right.task.completed) {
            return !left.task.completed && right.task.completed;
        }

        QString sortType = sortCombo ? sortCombo->currentText() : QString("按截止时间");

        if (sortType == "按截止时间") {
            if (left.task.deadline != right.task.deadline) {
                return left.task.deadline < right.task.deadline;
            }
        } else if (sortType == "按优先级") {
            if (left.task.priority != right.task.priority) {
                return left.task.priority > right.task.priority;
            }
        }

        return left.task.title < right.task.title;
    });

    QList<TaskViewModel> overdue;
    QList<TaskViewModel> today;
    QList<TaskViewModel> week;
    QList<TaskViewModel> later;
    QList<TaskViewModel> completed;

    const QDate todayDate = QDate::currentDate();
    for (const TaskViewModel &item : visible) {
        if (item.task.completed) {
            completed.append(item);
        } else if (item.task.isOverdue()) {
            overdue.append(item);
        } else if (item.task.deadline.date() == todayDate) {
            today.append(item);
        } else {
            int currentYear = 0;
            int deadlineYear = 0;
            const int currentWeek = QDate::currentDate().weekNumber(&currentYear);
            const int deadlineWeek = item.task.deadline.date().weekNumber(&deadlineYear);
            if (currentWeek == deadlineWeek && currentYear == deadlineYear) {
                week.append(item);
            } else {
                later.append(item);
            }
        }
    }

    if (summaryLabel) {
        summaryLabel->setText(QString("今日:%1  本周:%2  已完成:%3")
                                  .arg(today.size() + overdue.size())
                                  .arg(week.size())
                                  .arg(completed.size()));
    }

    auto appendSection = [this](const QString &title, const QString &accent, const QList<TaskViewModel> &items) {
        if (items.isEmpty()) {
            return;
        }

        boardLayout->addWidget(makeSectionHeader(title, items.size(), accent));

        QWidget *sectionBody = new QWidget(boardWidget);
        QVBoxLayout *sectionLayout = new QVBoxLayout(sectionBody);
        sectionLayout->setContentsMargins(0, 0, 0, 0);
        sectionLayout->setSpacing(10);

        for (const TaskViewModel &item : items) {
            TaskCardWidget *card = new TaskCardWidget(item.task, sectionBody);
            sectionLayout->addWidget(card);

            connect(card, &TaskCardWidget::completed, this, [sourceIndex = item.sourceIndex](const Task &task) {
                QTimer::singleShot(0, qApp, [sourceIndex, task]() {
                    DataManager::instance().updateTask(sourceIndex, task);
                });
            });

            connect(card, &TaskCardWidget::edited, this, [this, sourceIndex = item.sourceIndex](const Task &) {
                editTaskByIndex(sourceIndex);
            });

            connect(card, &TaskCardWidget::deleted, this, [this, sourceIndex = item.sourceIndex](const Task &) {
                if (!ConfirmDialog::confirm(
                    this,
                    "删除任务",
                    "删除后无法恢复，是否继续？",
                    "删除",
                    true
                )) {
                    return;
                }
                DataManager::instance().deleteTask(sourceIndex);
                ToastWidget::showToast(this, "任务已删除", 3000);
            });
        }

        boardLayout->addWidget(sectionBody);
    };

    appendSection("今日截止", Theme::PRIMARY, overdue + today);
    appendSection("本周 DDL", "#7B5E53", week);
    appendSection("后续任务", "#607D8B", later);

    if (!completed.isEmpty()) {
        QWidget *completedBody = new QWidget(boardWidget);
        completedBody->setVisible(false);
        QVBoxLayout *completedLayout = new QVBoxLayout(completedBody);
        completedLayout->setContentsMargins(0, 0, 0, 0);
        completedLayout->setSpacing(10);

        QFrame *header = new QFrame;
        header->setObjectName("sectionHeader");
        header->setStyleSheet(R"(
            QFrame#sectionHeader {
                background: white;
                border: 1px solid #F0E6E6;
                border-radius: 14px;
            }
        )");
        QHBoxLayout *headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(14, 12, 14, 12);
        QLabel *titleLabel = new QLabel("已完成任务");
        titleLabel->setStyleSheet("color:#7A7A7A; font-size:15px; font-weight:700;");
        QLabel *countLabel = new QLabel(QString::number(completed.size()));
        countLabel->setStyleSheet("color:#777; font-size:12px; font-weight:600;");
        QToolButton *toggle = new QToolButton;
        toggle->setCheckable(true);
        toggle->setText("展开");
        toggle->setStyleSheet(QString(R"(
            QToolButton {
                background: #FAF7F7;
                color: %1;
                border: 1px solid #E8D9DB;
                border-radius: 10px;
                padding: 6px 10px;
                font-weight: 600;
            }
        )").arg(Theme::PRIMARY));
        connect(toggle, &QToolButton::toggled, this, [completedBody, toggle](bool checked) {
            completedBody->setVisible(checked);
            toggle->setText(checked ? "收起" : "展开");
        });
        headerLayout->addWidget(titleLabel);
        headerLayout->addWidget(countLabel);
        headerLayout->addStretch();
        headerLayout->addWidget(toggle);
        boardLayout->addWidget(header);

        for (const TaskViewModel &item : completed) {
            TaskCardWidget *card = new TaskCardWidget(item.task, completedBody);
            completedLayout->addWidget(card);

            connect(card, &TaskCardWidget::completed, this, [sourceIndex = item.sourceIndex](const Task &task) {
                QTimer::singleShot(0, qApp, [sourceIndex, task]() {
                    DataManager::instance().updateTask(sourceIndex, task);
                });
            });
            connect(card, &TaskCardWidget::edited, this, [this, sourceIndex = item.sourceIndex](const Task &) {
                editTaskByIndex(sourceIndex);
            });
            connect(card, &TaskCardWidget::deleted, this, [this, sourceIndex = item.sourceIndex](const Task &) {
                if (!ConfirmDialog::confirm(
                    this,
                    "删除任务",
                    "删除后无法恢复，是否继续？",
                    "删除",
                    true
                )) {
                    return;
                }
                DataManager::instance().deleteTask(sourceIndex);
                ToastWidget::showToast(this, "任务已删除", 3000);
            });
        }

        boardLayout->addWidget(completedBody);
    }

    const bool hasAnyTask = !overdue.isEmpty() || !today.isEmpty() || !week.isEmpty() || !later.isEmpty() || !completed.isEmpty();
    if (emptyStateWidget) {
        if (hasAnyTask) {
            emptyStateWidget->hide();
            scrollArea->show();
        } else {
            emptyStateWidget->show();
            scrollArea->hide();
        }
    }

    boardLayout->addStretch();
}

void TodoPage::editTaskByIndex(int sourceIndex)
{
    const QList<Task> tasks = DataManager::instance().tasks();
    if (sourceIndex < 0 || sourceIndex >= tasks.size()) {
        return;
    }

    const Task task = tasks[sourceIndex];
    TaskEditDialog dialog(this, task.course);
    dialog.setWindowTitle("编辑DDL任务");
    dialog.setTaskData(task);

    bool completedStatus = task.completed;
    connect(&dialog, &QDialog::accepted, [&]() {
        completedStatus = dialog.getCompleted();
    });

    if (dialog.exec() == QDialog::Accepted) {
        Task updated = task;
        updated.course = dialog.getCourseName();
        updated.title = dialog.getTitle();
        updated.deadline = dialog.getDeadline();
        updated.priority = dialog.getPriority();
        updated.completed = completedStatus;
        DataManager::instance().updateTask(sourceIndex, updated);
        refreshTasks();
    }
}

void TodoPage::highlightTask(int taskIndex)
{
    refreshTasks();
}

void TodoPage::setupUI() {
    QPushButton *syncButton = new QPushButton("Sync Tasks from Teaching Platform");
    connect(syncButton, &QPushButton::clicked, this, [this]() {
        TeachingPlatformService *service = new TeachingPlatformService(this);
        connect(service, &TeachingPlatformService::tasksFetched, this, [this](const QList<QJsonObject> &tasks) {
            DataManager::instance().updateTasksFromPlatform(tasks);
        });
        service->fetchTodoTasks();
    });
    boardLayout->addWidget(syncButton);
}