#include "todopage.h"
#include "../models/taskmodel.h"
#include "../models/datamanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QFrame>
#include <QPushButton>

#include "../dialogs/taskeditdialog.h"

TodoPage::TodoPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background:#F7F3EF; font-family: 'Microsoft YaHei','Segoe UI', Arial; color: #222; font-weight:500;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16,16,16,16);
    mainLayout->setSpacing(14);

    QFrame *hero = new QFrame;
    hero->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #8B1E2D, stop:1 #B44B5D);
            border-radius: 20px;
        }
    )");
    QVBoxLayout *heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(20,18,20,18);

    QLabel *heroTitle = new QLabel("待办任务");
    heroTitle->setStyleSheet("color:white; font-size:22px; font-weight:700;");
    QLabel *heroSubtitle = new QLabel("按课程、时间、状态筛选，快速处理 DDL");
    heroSubtitle->setStyleSheet("color:rgba(255,255,255,0.82); font-size:12px;");
    heroLayout->addWidget(heroTitle);
    heroLayout->addWidget(heroSubtitle);
    mainLayout->addWidget(hero);

    // ===== 筛选栏 =====
    QFrame *filterCard = new QFrame;
    filterCard->setStyleSheet("background:white; border-radius:16px; border:1px solid #ECECEC;");
    QVBoxLayout *filterLayout = new QVBoxLayout(filterCard);
    filterLayout->setContentsMargins(14,14,14,14);
    filterLayout->addWidget(createFilterBar());
    mainLayout->addWidget(filterCard);

    // ===== 表格卡片 =====
    QFrame *card = new QFrame;
    card->setStyleSheet("background:white; border-radius:16px; border:1px solid #ECECEC;");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14,14,14,14);
    cardLayout->setSpacing(10);

    QHBoxLayout *cardBar = new QHBoxLayout;
    QLabel *listTitle = new QLabel("任务列表");
    listTitle->setStyleSheet("font-size:16px; font-weight:700; color:#222;");
    QLabel *listHint = new QLabel("双击可编辑，完成的任务会自动灰显并下沉");
    listHint->setStyleSheet("color:#888; font-size:12px;");

    QWidget *hintBox = new QWidget;
    QVBoxLayout *hintLayout = new QVBoxLayout(hintBox);
    hintLayout->setContentsMargins(0,0,0,0);
    hintLayout->setSpacing(2);
    hintLayout->addWidget(listTitle);
    hintLayout->addWidget(listHint);

    QPushButton *editBtn = new QPushButton("编辑");
    QPushButton *doneBtn = new QPushButton("完成/未完成");
    QPushButton *deleteBtn = new QPushButton("删除");
    for (QPushButton *btn : {editBtn, doneBtn, deleteBtn}) {
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(R"(
            QPushButton {
                background: transparent;
                color: #8B1E2D;
                border: 1px solid #E2C9CD;
                border-radius: 10px;
                padding: 8px 14px;
                font-weight: 600;
            }
            QPushButton:hover {
                background: #8B1E2D;
                color: white;
            }
            QPushButton:pressed {
                background: rgba(139,30,45,0.18);
                color: white;
            }
        )");
    }

    cardBar->addWidget(hintBox, 1);
    cardBar->addWidget(editBtn);
    cardBar->addWidget(doneBtn);
    cardBar->addWidget(deleteBtn);
    cardLayout->addLayout(cardBar);

    table = new QTableView;
    model = new TaskModel(this);

    table->setModel(model);

    // UI美化
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setShowGrid(false);
    table->setWordWrap(false);
    table->setCornerButtonEnabled(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(46);
    table->horizontalHeader()->setFixedHeight(40);

    table->setStyleSheet(R"(
        QTableView {
            border: none;
            background: transparent;
            selection-background-color: rgba(139,30,45,0.10);
            selection-color: #222;
        }
        QHeaderView::section {
            background: #F5F1EF;
            border: none;
            border-bottom: 1px solid #E7DDD8;
            padding: 10px 8px;
            font-weight: 700;
            color: #444;
        }
    )");

    cardLayout->addWidget(table);
    
    QWidget *actionBar = new QWidget;
    QHBoxLayout *actionLayout = new QHBoxLayout(actionBar);
    actionLayout->setContentsMargins(0,0,0,0);
    actionLayout->addStretch();
    actionLayout->addWidget(new QLabel("提示：完成任务后会自动沉到底部"));
    actionLayout->itemAt(1)->widget()->setStyleSheet("color:#888; font-size:12px;");
    cardLayout->addWidget(actionBar);

    connect(table, &QTableView::doubleClicked, this, [this](const QModelIndex &) {
        editSelectedTask();
    });

    connect(editBtn, &QPushButton::clicked, this, [this]() {
        editSelectedTask();
    });

    connect(doneBtn, &QPushButton::clicked, this, [this]() {
        if (!model || !table) return;
        const QModelIndex current = table->currentIndex();
        if (!current.isValid()) return;
        const int sourceIndex = model->sourceIndexAt(current.row());
        if (sourceIndex < 0) return;
        const QList<Task> tasks = DataManager::instance().tasks();
        if (sourceIndex >= tasks.size()) return;
        DataManager::instance().markTaskCompleted(sourceIndex, !tasks[sourceIndex].completed);
    });

    connect(deleteBtn, &QPushButton::clicked, this, [this]() {
        if (!model || !table) return;
        const QModelIndex current = table->currentIndex();
        if (!current.isValid()) return;
        const int sourceIndex = model->sourceIndexAt(current.row());
        if (sourceIndex < 0) return;
        DataManager::instance().deleteTask(sourceIndex);
    });

    mainLayout->addWidget(card);

    refreshCourseFilter();
    refreshTasks();

    connect(&DataManager::instance(), &DataManager::coursesChanged, this, &TodoPage::refreshCourseFilter);
    connect(&DataManager::instance(), &DataManager::tasksChanged, this, &TodoPage::refreshTasks);
}

QWidget* TodoPage::createFilterBar()
{
    QWidget *bar = new QWidget;

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0,0,0,0);

    searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText("搜索任务...");
    searchEdit->setClearButtonEnabled(true);

    courseFilter = new QComboBox;
    courseFilter->addItems({"全部课程"});
    timeFilter = new QComboBox;
    timeFilter->addItems({"全部时间", "今天", "本周", "逾期"});

    statusFilter = new QComboBox;
    statusFilter->addItems({"全部状态", "未完成", "已完成"});

    QPushButton *editButton = new QPushButton("编辑DDL");
    editButton->setStyleSheet(R"(
        QPushButton {
            background:#8B1E2D;
            color:white;
            border:none;
            border-radius:8px;
            padding:8px 14px;
        }
        QPushButton:hover {
            background:#7A1C2C;
        }
    )");

    layout->addWidget(searchEdit);
    layout->addWidget(courseFilter);
    layout->addWidget(timeFilter);
    layout->addWidget(statusFilter);
    layout->addWidget(editButton);

    searchEdit->setStyleSheet(R"(
        QLineEdit {
            border: 1px solid #E2D7D1;
            border-radius: 10px;
            padding: 10px 12px;
            background: #FAFAFA;
            color: #222;
        }
    )");
    for (QComboBox *box : {courseFilter, timeFilter, statusFilter}) {
        box->setStyleSheet(R"(
            QComboBox {
                border: 1px solid #E2D7D1;
                border-radius: 10px;
                padding: 8px 12px;
                background: #FAFAFA;
                color: #222;
            }
            QComboBox::drop-down {
                border: none;
                width: 20px;
            }
        )");
    }

    connect(searchEdit, &QLineEdit::textChanged, this, &TodoPage::applyFilter);
    connect(courseFilter, &QComboBox::currentTextChanged, this, &TodoPage::applyFilter);
    connect(timeFilter, &QComboBox::currentTextChanged, this, &TodoPage::applyFilter);
    connect(statusFilter, &QComboBox::currentTextChanged, this, &TodoPage::applyFilter);
    connect(editButton, &QPushButton::clicked, this, &TodoPage::editSelectedTask);

    return bar;
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
    model->setTasks(DataManager::instance().tasks());
    applyFilter();
}

void TodoPage::applyFilter()
{
    if (!model) {
        return;
    }

    model->setFilter(
        courseFilter ? courseFilter->currentText() : QString(),
        timeFilter ? timeFilter->currentText() : QString(),
        statusFilter ? statusFilter->currentText() : QString(),
        searchEdit ? searchEdit->text() : QString()
    );
}

void TodoPage::editSelectedTask()
{
    if (!model || !table) {
        return;
    }

    const QModelIndex current = table->currentIndex();
    if (!current.isValid()) {
        return;
    }

    const int sourceIndex = model->sourceIndexAt(current.row());
    if (sourceIndex < 0) {
        return;
    }

    const QList<Task> tasks = DataManager::instance().tasks();
    if (sourceIndex >= tasks.size()) {
        return;
    }

    const Task task = tasks[sourceIndex];
    TaskEditDialog dialog(this, task.course);
    dialog.setWindowTitle("编辑DDL任务");
    dialog.setTaskData(task);

    if (dialog.exec() == QDialog::Accepted) {
        Task updated = task;
        updated.course = dialog.getCourseName();
        updated.title = dialog.getTitle();
        updated.deadline = dialog.getDeadline();
        updated.priority = dialog.getPriority();

        DataManager::instance().updateTask(sourceIndex, updated);
    }
}