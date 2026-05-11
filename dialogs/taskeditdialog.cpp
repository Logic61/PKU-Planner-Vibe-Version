#include "taskeditdialog.h"
#include "../models/datamanager.h"
#include "../ui/theme.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFrame>
#include <QCalendarWidget>
#include <QCheckBox>

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
        calendar->setStyleSheet(QString(R"(
            QCalendarWidget QWidget {
                color: %1;
                background: white;
            }
            QCalendarWidget QToolButton {
                color: %1;
                background: transparent;
                border: none;
                font-weight: 600;
            }
            QCalendarWidget QToolButton#qt_calendar_prevmonth {
                color: transparent;
                background: transparent;
                border: none;
            }
            QCalendarWidget QToolButton#qt_calendar_nextmonth {
                color: transparent;
                background: transparent;
                border: none;
            }
            QCalendarWidget QMenu {
                background: white;
                color: %1;
            }
            QCalendarWidget QAbstractItemView {
                selection-background-color: %2;
                selection-color: white;
                background: white;
                color: %1;
            }
        )").arg(Theme::TEXT_PRIMARY).arg(Theme::PRIMARY));
    }
    priorityCombo = new QComboBox();
    priorityCombo->addItems({"低", "中", "高"});
    hoursSpin = new QSpinBox();
    hoursSpin->setRange(1, 100);
    hoursSpin->setValue(2);
    completedCheck = new QCheckBox("标记为已完成");
    courseCombo = new QComboBox();
    courseCombo->addItem("请选择课程");

    for (const Course &course : DataManager::instance().courses()) {
        if (!course.name.isEmpty()) {
            courseCombo->addItem(course.name);
        }
    }

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
    formLayout->addRow("完成状态:", completedCheck);
    formLayout->addRow("所属课程:", courseCombo);

    mainLayout->addLayout(formLayout);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttons);

    setStyleSheet(QString(R"(
        QDialog {
            background: transparent;
        }
        QLineEdit {
            border: 1px solid #E8DADA;
            border-radius: 10px;
            padding: 10px 12px;
            background: #FEFEFE;
            color: #333;
            font-size: 13px;
        }
        QLineEdit:focus {
            border: 2px solid %1;
            background: white;
        }
        QDateTimeEdit {
            border: 1px solid #E8DADA;
            border-radius: 10px;
            padding: 10px 12px;
            background: #FEFEFE;
            color: #333;
            font-size: 13px;
        }
        QDateTimeEdit:focus {
            border: 2px solid %1;
        }
        QComboBox {
            border: 1px solid #E8DADA;
            border-radius: 10px;
            padding: 10px 12px;
            background: #FEFEFE;
            color: #333;
            font-size: 13px;
        }
        QComboBox:focus {
            border: 2px solid %1;
        }
        QSpinBox {
            border: 1px solid #E8DADA;
            border-radius: 10px;
            padding: 10px 12px;
            background: #FEFEFE;
            color: #333;
            font-size: 13px;
        }
        QSpinBox:focus {
            border: 2px solid %1;
        }
        QCheckBox {
            color: #444;
            padding: 4px 0;
            font-size: 13px;
        }
        QCheckBox::indicator {
            width: 20px;
            height: 20px;
            border-radius: 4px;
            border: 2px solid #E8DADA;
            background: white;
        }
        QCheckBox::indicator:checked {
            background: %1;
            border: 2px solid %1;
        }
        QPushButton {
            background: %1;
            color: white;
            border-radius: 10px;
            padding: 10px 20px;
            font-weight: 600;
            font-size: 13px;
            border: none;
        }
        QPushButton:hover {
            background: %2;
        }
        QPushButton:pressed {
            background: %3;
        }
        QDialogButtonBox QPushButton[role='RejectRole'] {
            background: #F5F5F5;
            color: #666;
        }
        QDialogButtonBox QPushButton[role='RejectRole']:hover {
            background: #E8E8E8;
            color: #333;
        }
        QLabel {
            color: #444;
            font-size: 13px;
            font-weight: 500;
        }
    )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK).arg("#6A1520"));
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

bool TaskEditDialog::getCompleted() const {
    bool checked = completedCheck->isChecked();
    qDebug() << "[TaskEditDialog] getCompleted() returning:" << checked;
    return checked;
}

QString TaskEditDialog::getCourseName() const {
    return courseCombo->currentText();
}

void TaskEditDialog::setTaskData(const Task &task) {
    titleEdit->setText(task.title);
    deadlineEdit->setDateTime(task.deadline);
    priorityCombo->setCurrentIndex(task.priority);
    completedCheck->setChecked(task.completed);

    const int index = courseCombo->findText(task.course);
    if (index != -1) {
        courseCombo->setCurrentIndex(index);
    }
}
