#include "courseinfopage.h"
#include "../../ui/theme.h"
#include "../../utils/datetimeutils.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QStringList>
#include <QSet>
#include <QInputDialog>
#include <QEvent>
#include <QMouseEvent>
#include <algorithm>

#include "../../models/datamanager.h"

namespace {
using DateTimeUtils::dayText;
using DateTimeUtils::weekTypeText;
using DateTimeUtils::safeText;
using DateTimeUtils::scheduleLine;
}

CourseInfoPage::CourseInfoPage(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet("background:#F8F6F4;");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    QWidget* pageHeader = new QWidget(this);
    QHBoxLayout* headerLayout = new QHBoxLayout(pageHeader);
    headerLayout->setContentsMargins(4, 0, 4, 0);
    headerLayout->setSpacing(10);

    QLabel* titleLabel = new QLabel("课程信息", pageHeader);
    titleLabel->setStyleSheet("font-size:18px;font-weight:700;color:#222;");

    editBtn = new QPushButton("编辑课程", pageHeader);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet(
        QString("QPushButton{background:white;border:1px solid %1;border-radius:10px;padding:6px 12px;color:#4B3A35;}"
        "QPushButton:hover{border:1px solid %2;}").arg(Theme::BORDER).arg(Theme::PRIMARY)
    );

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(editBtn);

    root->addWidget(pageHeader);
    root->addWidget(createProgressCard());

    QWidget* middleRow = new QWidget(this);
    QHBoxLayout* middleLayout = new QHBoxLayout(middleRow);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    middleLayout->setSpacing(12);
    middleLayout->addWidget(createScheduleCard(), 1);
    middleLayout->addWidget(createExamCard(), 1);
    root->addWidget(middleRow);

    root->addWidget(createTeacherCard());
    root->addWidget(createNoteCard(), 1);

    connect(editBtn, &QPushButton::clicked, this, [this]() {
        emit editRequested(currentCourse);
    });
}

QWidget* CourseInfoPage::createProgressCard()
{
    QFrame* card = new QFrame(this);
    card->setStyleSheet(
        "QFrame{background:white;border-radius:20px;}"
    );

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);

    QLabel* title = new QLabel("学习进度", card);
    title->setStyleSheet("font-size:15px;font-weight:700;color:#222;");

    progressLabel = new QLabel("0%", card);
    progressLabel->setStyleSheet(QString("font-size:28px;font-weight:800;color:%1;").arg(Theme::PRIMARY));

    progressBar = new QProgressBar(card);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(false);
    progressBar->setFixedHeight(12);
    progressBar->setStyleSheet(QString(R"(
        QProgressBar {
            border: none;
            background: #F5F5F5;
            border-radius: 6px;
        }
        QProgressBar::chunk {
            background: %1;
            border-radius: 6px;
        }
    )").arg(Theme::PRIMARY));

    QLabel* detailLabel = new QLabel("0 / 0 tasks completed", card);
    detailLabel->setObjectName("progressDetailLabel");
    detailLabel->setStyleSheet("color:#7A746E;font-size:12px;");

    layout->addWidget(title);
    layout->addWidget(progressLabel);
    layout->addWidget(progressBar);
    layout->addWidget(detailLabel);

    return card;
}

QWidget* CourseInfoPage::createScheduleCard()
{
    QFrame* card = new QFrame(this);
    card->setStyleSheet("QFrame{background:white;border-radius:20px;}");

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    QLabel* title = new QLabel("上课安排", card);
    title->setStyleSheet("font-size:15px;font-weight:700;color:#222;");

    scheduleLabel = new QLabel("暂未设置", card);
    scheduleLabel->setWordWrap(true);
    scheduleLabel->setStyleSheet("font-size:18px;font-weight:700;color:#4B3A35;");

    locationLabel = new QLabel("", card);
    locationLabel->setStyleSheet("color:#7A746E;font-size:13px;");

    layout->addWidget(title);
    layout->addWidget(scheduleLabel);
    layout->addWidget(locationLabel);
    layout->addStretch();

    return card;
}

QWidget* CourseInfoPage::createExamCard()
{
    QFrame* card = new QFrame(this);
    card->setStyleSheet("QFrame{background:white;border-radius:20px;}");

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    QLabel* title = new QLabel("考试信息", card);
    title->setStyleSheet("font-size:15px;font-weight:700;color:#222;");

    examLabel = new QLabel("暂未设置", card);
    examLabel->setWordWrap(true);
    examLabel->setStyleSheet("font-size:18px;font-weight:700;color:#4B3A35;");

    layout->addWidget(title);
    layout->addWidget(examLabel);
    layout->addStretch();

    return card;
}

QWidget* CourseInfoPage::createTeacherCard()
{
    QFrame* card = new QFrame(this);
    card->setStyleSheet("QFrame{background:white;border-radius:20px;}");

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(6);

    QLabel* title = new QLabel("教师信息", card);
    title->setStyleSheet("font-size:15px;font-weight:700;color:#222;");

    teacherLabel = new QLabel("未填写", card);
    teacherLabel->setStyleSheet("font-size:18px;font-weight:700;color:#4B3A35;");

    contactLabel = new QLabel("未填写联系方式", card);
    contactLabel->setStyleSheet("color:#7A746E;font-size:13px;");
    contactLabel->setCursor(Qt::PointingHandCursor);
    contactLabel->setToolTip("点击编辑联系方式");

    layout->addWidget(title);
    layout->addWidget(teacherLabel);
    layout->addWidget(contactLabel);

    connect(contactLabel, &QLabel::linkActivated, this, &CourseInfoPage::editContact);

    // linkActivated only fires on HTML links; install event filter for plain-text click
    contactLabel->installEventFilter(this);

    return card;
}

QWidget* CourseInfoPage::createNoteCard()
{
    QFrame* card = new QFrame(this);
    card->setStyleSheet("QFrame{background:white;border-radius:20px;}");

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    QLabel* title = new QLabel("课程备注", card);
    title->setStyleSheet("font-size:15px;font-weight:700;color:#222;");

    noteEdit = new QTextEdit(card);
    noteEdit->setPlaceholderText("记录课堂重点、考试提醒、老师要求...");
    noteEdit->setAcceptRichText(true);
    noteEdit->setMinimumHeight(150);
    noteEdit->setStyleSheet(QString(R"(
        QTextEdit {
            border: 1px solid %1;
            border-radius: 14px;
            background: #FCFBFA;
            padding: 10px;
            color: #2F2926;
            font-size: 13px;
            selection-background-color: %2;
        }
        QTextEdit:focus {
            border: 1px solid %2;
            background: white;
        }
    )").arg(Theme::BORDER).arg(Theme::PRIMARY));

    layout->addWidget(title);
    layout->addWidget(noteEdit, 1);

    connect(noteEdit, &QTextEdit::textChanged, this, [this]() {
        currentCourse.note = noteEdit->toPlainText();
        emit courseUpdated(currentCourse);
    });

    return card;
}

void CourseInfoPage::loadCourse(const Course& course)
{
    currentCourse = course;

    const QString scheduleSummary = scheduleSummaryForCourse(course.name);
    const QString locationSummary = locationSummaryForCourse(course.name);

    scheduleLabel->setText(scheduleSummary.isEmpty() ? QString("暂未设置") : scheduleSummary);
    locationLabel->setText(locationSummary.isEmpty() ? QString("暂无上课地点") : locationSummary);

    examLabel->setText(safeText(course.examTime, "暂未设置"));
    teacherLabel->setText(safeText(course.teacher, "未填写"));
    contactLabel->setText(course.contact.trimmed().isEmpty() ? "未填写联系方式" : course.contact);

    {
        QSignalBlocker blocker(noteEdit);
        noteEdit->setPlainText(course.note);
    }

    const QList<Task> tasks = DataManager::instance().tasks();
    int totalTasks = 0;
    int completedTasks = 0;
    for (const Task& task : tasks) {
        if (task.course != course.name) {
            continue;
        }
        ++totalTasks;
        if (task.completed) {
            ++completedTasks;
        }
    }
    refreshProgress(completedTasks, totalTasks);
}

QString CourseInfoPage::scheduleSummaryForCourse(const QString& courseName) const
{
    const QList<Course> courses = DataManager::instance().courses();
    QStringList lines;
    QSet<QString> seen;

    for (const Course& item : courses) {
        if (item.name != courseName) {
            continue;
        }
        const QString line = scheduleLine(item);
        if (!line.isEmpty() && !seen.contains(line)) {
            seen.insert(line);
            lines.append(line);
        }
    }

    return lines.join("\n");
}

QString CourseInfoPage::locationSummaryForCourse(const QString& courseName) const
{
    const QList<Course> courses = DataManager::instance().courses();
    QStringList locations;
    QSet<QString> seen;

    for (const Course& item : courses) {
        if (item.name != courseName) {
            continue;
        }
        const QString location = item.location.trimmed();
        if (!location.isEmpty() && !seen.contains(location)) {
            seen.insert(location);
            locations.append(location);
        }
    }

    return locations.join(" · ");
}

void CourseInfoPage::refreshProgress(int completedTasks, int totalTasks)
{
    const int safeTotal = std::max(totalTasks, 0);
    const int safeCompleted = std::max(0, std::min(completedTasks, safeTotal));
    const int progress = safeTotal > 0 ? (safeCompleted * 100) / safeTotal : 0;

    progressBar->setValue(progress);
    progressLabel->setText(QString::number(progress) + "%");

    QLabel* detailLabel = findChild<QLabel*>("progressDetailLabel");
    if (detailLabel) {
        if (safeTotal > 0) {
            detailLabel->setText(QString("%1 / %2 tasks completed").arg(safeCompleted).arg(safeTotal));
        } else {
            detailLabel->setText("暂未关联任务");
        }
    }
}

void CourseInfoPage::editContact()
{
    bool ok = false;
    QString text = QInputDialog::getText(this, "编辑联系方式",
        "请输入教师联系方式（邮箱/电话/微信等）:",
        QLineEdit::Normal, currentCourse.contact, &ok);

    if (ok && text != currentCourse.contact) {
        currentCourse.contact = text;
        contactLabel->setText(text.trimmed().isEmpty() ? "未填写联系方式" : text);
        emit courseUpdated(currentCourse);
    }
}

bool CourseInfoPage::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == contactLabel && event->type() == QEvent::MouseButtonPress) {
        editContact();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}