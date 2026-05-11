#include "coursetaskpage.h"
#include "../../ui/theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

#include "../../components/taskcardwidget.h"
#include "../../dialogs/taskeditdialog.h"
#include "../../dialogs/confirmdialog.h"
#include "../../components/toastwidget.h"
#include "../../models/datamanager.h"

namespace {
struct TaskItem
{
    Task task;
    int sourceIndex = -1;
};

bool isCourseTask(const Task& task, const QString& courseName)
{
    return !courseName.trimmed().isEmpty() && task.course == courseName;
}

QString priorityText(int priority)
{
    switch (priority) {
    case 2: return "高优先级";
    case 1: return "中优先级";
    default: return "低优先级";
    }
}
}

CourseTaskPage::CourseTaskPage(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet("background:#F8F6F4;");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    QFrame* titleCard = new QFrame(this);
    titleCard->setStyleSheet("QFrame{background:white;border-radius:20px;}");
    QVBoxLayout* titleLayout = new QVBoxLayout(titleCard);
    titleLayout->setContentsMargins(18, 16, 18, 16);
    titleLayout->setSpacing(4);

    QLabel* title = new QLabel("课程任务", titleCard);
    title->setStyleSheet("font-size:18px;font-weight:700;color:#222;");
    QLabel* subtitle = new QLabel("搜索、筛选、排序并与 TodoPage 保持同步", titleCard);
    subtitle->setStyleSheet("font-size:12px;color:#7A746E;");

    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);
    root->addWidget(titleCard);

    QFrame* filterCard = new QFrame(this);
    filterCard->setStyleSheet("QFrame{background:white;border-radius:20px;}");
    QHBoxLayout* filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(14, 12, 14, 12);
    filterLayout->setSpacing(8);

    searchEdit = new QLineEdit(filterCard);
    searchEdit->setPlaceholderText("搜索任务...");
    searchEdit->setClearButtonEnabled(true);

    sortBox = new QComboBox(filterCard);
    sortBox->addItems({"DDL优先", "优先级优先"});

    hideCompletedBox = new QCheckBox("隐藏已完成", filterCard);
    hideCompletedBox->setChecked(false);

    QPushButton* addBtn = new QPushButton("+ 添加任务", filterCard);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(
        QString("QPushButton{background:%1;color:white;border:none;border-radius:12px;padding:8px 14px;font-weight:700;}"
        "QPushButton:hover{background:%2;}").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK)
    );

    for (QWidget* widget : {static_cast<QWidget*>(searchEdit), static_cast<QWidget*>(sortBox)}) {
        widget->setStyleSheet(
            "QLineEdit, QComboBox{border:1px solid #E8D9DB;border-radius:12px;padding:8px 12px;background:white;color:#222;}"
        );
    }

    filterLayout->addWidget(searchEdit, 2);
    filterLayout->addWidget(sortBox, 1);
    filterLayout->addWidget(hideCompletedBox, 0);
    filterLayout->addWidget(addBtn, 0);
    root->addWidget(filterCard);

    summaryLabel = new QLabel(this);
    summaryLabel->setStyleSheet("color:#7A746E;font-size:12px;font-weight:600;");
    root->addWidget(summaryLabel);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background:transparent;");

    QWidget* listWidget = new QWidget(scrollArea);
    listWidget->setStyleSheet("background:transparent;");
    taskListLayout = new QVBoxLayout(listWidget);
    taskListLayout->setContentsMargins(0, 0, 0, 0);
    taskListLayout->setSpacing(10);
    scrollArea->setWidget(listWidget);
    root->addWidget(scrollArea, 1);

    connect(searchEdit, &QLineEdit::textChanged, this, &CourseTaskPage::renderTasks);
    connect(sortBox, &QComboBox::currentIndexChanged, this, &CourseTaskPage::renderTasks);
    connect(hideCompletedBox, &QCheckBox::toggled, this, &CourseTaskPage::renderTasks);
    connect(addBtn, &QPushButton::clicked, this, &CourseTaskPage::openAddTaskDialog);
    connect(&DataManager::instance(), &DataManager::tasksChanged, this, [this]() {
        if (!currentCourseName.isEmpty()) {
            loadTasksFromJson();
            renderTasks();
        }
    });
}

void CourseTaskPage::loadCourseTasks(const QString& courseName)
{
    currentCourseName = courseName;
    loadTasksFromJson();
    renderTasks();
}

void CourseTaskPage::loadTasksFromJson()
{
    currentTasks.clear();
    sourceIndices.clear();

    const QList<Task> tasks = DataManager::instance().tasks();
    for (int i = 0; i < tasks.size(); ++i) {
        if (isCourseTask(tasks[i], currentCourseName)) {
            currentTasks.push_back(tasks[i]);
            sourceIndices.push_back(i);
        }
    }
}

void CourseTaskPage::renderTasks()
{
    if (!taskListLayout) {
        return;
    }

    while (QLayoutItem* item = taskListLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    struct TaskItemWithSource {
        Task task;
        int sourceIndex;
    };

    QList<TaskItemWithSource> items;
    const QString keyword = searchEdit ? searchEdit->text().trimmed().toLower() : QString();
    for (int i = 0; i < static_cast<int>(currentTasks.size()); ++i) {
        const Task& task = currentTasks[static_cast<size_t>(i)];
        if (hideCompletedBox && hideCompletedBox->isChecked() && task.completed) {
            continue;
        }
        if (!keyword.isEmpty() && !task.title.toLower().contains(keyword) && !task.course.toLower().contains(keyword)) {
            continue;
        }
        items.append({task, sourceIndices[static_cast<size_t>(i)]});
    }

    std::sort(items.begin(), items.end(), [this](const TaskItemWithSource& left, const TaskItemWithSource& right) {
        if (left.task.completed != right.task.completed) {
            return !left.task.completed && right.task.completed;
        }

        if (sortBox && sortBox->currentIndex() == 1) {
            if (left.task.priority != right.task.priority) {
                return left.task.priority > right.task.priority;
            }
            if (left.task.deadline != right.task.deadline) {
                return left.task.deadline < right.task.deadline;
            }
        } else {
            if (left.task.deadline != right.task.deadline) {
                return left.task.deadline < right.task.deadline;
            }
            if (left.task.priority != right.task.priority) {
                return left.task.priority > right.task.priority;
            }
        }

        return left.task.title < right.task.title;
    });

    int activeCount = 0;
    int completedCount = 0;
    for (const auto& item : items) {
        if (item.task.completed) {
            ++completedCount;
        } else {
            ++activeCount;
        }
    }
    if (summaryLabel) {
        summaryLabel->setText(QString("当前课程：%1 | 未完成 %2 | 已完成 %3").arg(currentCourseName).arg(activeCount).arg(completedCount));
    }

    if (items.isEmpty()) {
        QLabel* empty = new QLabel("暂无任务", taskListLayout->parentWidget());
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("color:#888;font-size:14px;padding:24px;");
        taskListLayout->addWidget(empty);
        taskListLayout->addStretch();
        return;
    }

    for (const auto& item : items) {
        TaskCardWidget* card = new TaskCardWidget(item.task, taskListLayout->parentWidget());
        taskListLayout->addWidget(card);

        connect(card, &TaskCardWidget::completed, this, [this, sourceIndex = item.sourceIndex](const Task& task) {
            DataManager::instance().updateTask(sourceIndex, task);
            emit taskUpdated();
        });
        connect(card, &TaskCardWidget::edited, this, [this, sourceIndex = item.sourceIndex](const Task&) {
            openEditTaskDialog(sourceIndex);
        });
        connect(card, &TaskCardWidget::deleted, this, [this, sourceIndex = item.sourceIndex](const Task&) {
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
            emit taskUpdated();
        });
    }

    taskListLayout->addStretch();
}

void CourseTaskPage::openAddTaskDialog()
{
    if (currentCourseName.trimmed().isEmpty()) {
        return;
    }

    emit addTaskRequested(currentCourseName);

    TaskEditDialog dialog(this, currentCourseName);
    dialog.setWindowTitle("添加任务");
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Task task;
    task.course = currentCourseName;
    task.title = dialog.getTitle();
    task.deadline = dialog.getDeadline();
    task.priority = dialog.getPriority();
    task.completed = dialog.getCompleted();
    DataManager::instance().addTask(task);
    emit taskUpdated();
    loadTasksFromJson();
    renderTasks();
}

void CourseTaskPage::openEditTaskDialog(int sourceIndex)
{
    const QList<Task> tasks = DataManager::instance().tasks();
    if (sourceIndex < 0 || sourceIndex >= tasks.size()) {
        return;
    }

    Task task = tasks[sourceIndex];
    TaskEditDialog dialog(this, task.course);
    dialog.setWindowTitle("编辑任务");
    dialog.setTaskData(task);

    bool completedStatus = task.completed;
    connect(&dialog, &QDialog::accepted, [&]() {
        completedStatus = dialog.getCompleted();
    });

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    task.course = dialog.getCourseName();
    task.title = dialog.getTitle();
    task.deadline = dialog.getDeadline();
    task.priority = dialog.getPriority();
    task.completed = completedStatus;
    DataManager::instance().updateTask(sourceIndex, task);
    emit taskUpdated();
    loadTasksFromJson();
renderTasks();
}