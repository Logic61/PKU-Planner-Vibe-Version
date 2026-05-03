#include "courseeditdialog.h"
#include "../models/datamanager.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QFrame>

CourseEditDialog::CourseEditDialog(int defaultStart, int defaultEnd, QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    QFrame *container=new QFrame;
    container->setStyleSheet(R"(
    QFrame{
        background:white;
        border-radius:20px;
    }
    )");
    
    QVBoxLayout *root=new QVBoxLayout(this);
    root->setContentsMargins(20,20,20,20);
    root->addWidget(container);
    
    setStyleSheet(R"(
        QLineEdit {
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 8px;
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

    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    QFormLayout *formLayout = new QFormLayout();

    nameEdit = new QLineEdit();
    teacherEdit = new QLineEdit();
    locationEdit = new QLineEdit();
    examEdit = new QLineEdit();
    
    startCombo = new QComboBox();
    endCombo = new QComboBox();
    weekTypeCombo = new QComboBox();
    
    for (int i = 1; i <= 12; ++i) {
        startCombo->addItem(QString("第 %1 节").arg(i));
        endCombo->addItem(QString("第 %1 节").arg(i));
    }
    
    weekTypeCombo->addItem("每周");
    weekTypeCombo->addItem("单周");
    weekTypeCombo->addItem("双周");
    
    startCombo->setCurrentIndex(defaultStart - 1);
    endCombo->setCurrentIndex(defaultEnd - 1);
    weekTypeCombo->setCurrentIndex(0);

    formLayout->addRow("课程名称:", nameEdit);
    formLayout->addRow("教师:", teacherEdit);
    formLayout->addRow("地点:", locationEdit);
    formLayout->addRow("考试时间:", examEdit);
    formLayout->addRow("开始节次:", startCombo);
    formLayout->addRow("结束节次:", endCombo);
    formLayout->addRow("周数类型:", weekTypeCombo);

    // 当课程名称文本变化时，恢复样式
    connect(nameEdit, &QLineEdit::textChanged, this, [this]() {
        if (!nameEdit->text().trimmed().isEmpty()) {
            nameEdit->setStyleSheet("");
            nameEdit->setPlaceholderText("");
        }

        const QString courseName = nameEdit->text().trimmed();
        if (courseName.isEmpty()) {
            return;
        }

        for (const Course &course : DataManager::instance().courses()) {
            if (course.name == courseName) {
                teacherEdit->setText(course.teacher);
                locationEdit->setText(course.location);
                examEdit->setText(course.examTime);
                break;
            }
        }
    });

    mainLayout->addLayout(formLayout);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    
    connect(buttons, &QDialogButtonBox::accepted, this, &CourseEditDialog::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttons);
}

QString CourseEditDialog::getName() const {
    return nameEdit->text();
}

QString CourseEditDialog::getTeacher() const {
    return teacherEdit->text();
}

QString CourseEditDialog::getLocation() const {
    return locationEdit->text();
}

QString CourseEditDialog::getExamTime() const {
    return examEdit->text();
}

int CourseEditDialog::getStart() const {
    return startCombo->currentIndex() + 1;
}

int CourseEditDialog::getEnd() const {
    return endCombo->currentIndex() + 1;
}

int CourseEditDialog::getWeekType() const {
    return weekTypeCombo->currentIndex();
}

void CourseEditDialog::setCourseData(const QString &name, const QString &teacher,
                                   const QString &location, const QString &examTime,
                                   int start, int end, int weekType) {
    nameEdit->setText(name);
    teacherEdit->setText(teacher);
    locationEdit->setText(location);
    examEdit->setText(examTime);
    startCombo->setCurrentIndex(start - 1);
    endCombo->setCurrentIndex(end - 1);
    weekTypeCombo->setCurrentIndex(weekType);
}

void CourseEditDialog::onAccepted() {
    if (nameEdit->text().trimmed().isEmpty()) {
        // 课程名称为空，显示错误提示
        nameEdit->setStyleSheet("border: 2px solid red; border-radius: 8px; padding: 8px;");
        nameEdit->setPlaceholderText("课程名称不能为空！");
        nameEdit->setFocus();
        return;
    }
    // 验证通过，接受对话框
    accept();
}
