#include "taskeditdialog.h"
#include "../models/datamanager.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFrame>
#include <QCalendarWidget>

TaskEditDialog::TaskEditDialog(QWidget *parent, const QString &defaultCourse)
    : QDialog(parent)
{
    setWindowTitle("添加DDL任务");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QFrame *container = new QFrame(this);
    container->setStyleSheet(R"(
        QFrame{
            background:white;
            border-radius:20px;
        }
    )");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->addWidget(container);

    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    QFormLayout *formLayout = new QFormLayout();

    titleEdit = new QLineEdit();
    deadlineEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    deadlineEdit->setCalendarPopup(true);
    if (QCalendarWidget *calendar = deadlineEdit->calendarWidget()) {
        calendar->setStyleSheet(R"(
            QCalendarWidget QWidget {
                color: #222;
                background: white;
            }
            QCalendarWidget QToolButton {
                color: #222;
                background: transparent;
                border: none;
                font-weight: 600;
            }
            QCalendarWidget QMenu {
                background: white;
                color: #222;
            }
            QCalendarWidget QAbstractItemView {
                selection-background-color: #8B1E2D;
                selection-color: white;
                background: white;
                color: #222;
            }
        )");
    }
    priorityCombo = new QComboBox();
    priorityCombo->addItems({"低", "中", "高"});
    hoursSpin = new QSpinBox();
    hoursSpin->setRange(1, 100);
    hoursSpin->setValue(2);
    courseCombo = new QComboBox();
    courseCombo->addItem("请选择课程");

    for (const Course &course : DataManager::instance().courses()) {
        if (!course.name.isEmpty()) {
            courseCombo->addItem(course.name);
        }
    }

    // 设置默认课程
    if (!defaultCourse.isEmpty()) {
        int index = courseCombo->findText(defaultCourse);
        if (index != -1) {
            courseCombo->setCurrentIndex(index);
        }
    }

    formLayout->addRow("任务标题:", titleEdit);
    formLayout->addRow("截止时间:", deadlineEdit);
    formLayout->addRow("优先级:", priorityCombo);
    formLayout->addRow("预计小时:", hoursSpin);
    formLayout->addRow("所属课程:", courseCombo);

    mainLayout->addLayout(formLayout);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttons);

    setStyleSheet(R"(
        QDialog {
            background: transparent;
        }
        QLineEdit {
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 8px;
            color: #222;
        }
        QDateTimeEdit {
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 8px;
            color: #222;
        }
        QComboBox {
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 8px;
            color: #222;
        }
        QSpinBox {
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 8px;
            color: #222;
        }
        QPushButton {
            background: #8B1E2D;
            color: white;
            border-radius: 8px;
            padding: 8px 16px;
        }
        QPushButton:hover {
            background: #7A1C2C;
        }
    )");
}

QString TaskEditDialog::getTitle() const {
    return titleEdit->text();
}

QDateTime TaskEditDialog::getDeadline() const {
    return deadlineEdit->dateTime();
}

int TaskEditDialog::getPriority() const {
    return priorityCombo->currentIndex();
}

int TaskEditDialog::getEstimatedHours() const {
    return hoursSpin->value();
}

QString TaskEditDialog::getCourseName() const {
    return courseCombo->currentText();
}

void TaskEditDialog::setTaskData(const Task &task) {
    titleEdit->setText(task.title);
    deadlineEdit->setDateTime(task.deadline);
    priorityCombo->setCurrentIndex(task.priority);

    const int index = courseCombo->findText(task.course);
    if (index != -1) {
        courseCombo->setCurrentIndex(index);
    }
}