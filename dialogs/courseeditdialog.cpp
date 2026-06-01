#include "courseeditdialog.h"
#include "../models/datamanager.h"
#include "../ui/theme.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QMessageBox>

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
    
    setStyleSheet(QString(R"(
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
        QLineEdit:disabled {
            background: #F5F5F5;
            color: #999;
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
    weekTypeCombo->setCurrentIndex(qBound(0, weekType, 2));
}

void CourseEditDialog::onAccepted() {
    if (nameEdit->text().trimmed().isEmpty()) {
        nameEdit->setStyleSheet("border: 2px solid red; border-radius: 8px; padding: 8px;");
        nameEdit->setPlaceholderText("课程名称不能为空！");
        nameEdit->setFocus();
        return;
    }

    int start = startCombo->currentIndex() + 1;
    int end = endCombo->currentIndex() + 1;
    if (start > end) {
        QMessageBox::warning(this, "输入错误", "开始节次不能大于结束节次");
        startCombo->setFocus();
        return;
    }
    // 验证通过，接受对话框
    accept();
}
