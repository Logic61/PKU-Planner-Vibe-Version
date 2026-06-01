#include "topbarwidget.h"
#include "../ui/theme.h"
#include "../models/datamanager.h"
#include "../models/course.h"
#include "../models/task.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDate>
#include <QDebug>
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QDir>

TopbarWidget::TopbarWidget(QWidget *parent)
    : QWidget(parent)
    , searchPopup(nullptr)
    , searchTimer(new QTimer(this))
{
    setFixedHeight(50);
    setStyleSheet("background:#F7F3EF;");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);

    QLabel *title = new QLabel("PKU Planner+");
    title->setStyleSheet("font-weight:700; font-size:16px; color:#222;");
    layout->addWidget(title);

    layout->addStretch();

    QDate today = QDate::currentDate();
    int month = today.month();
    QString season;
    if (month >= 3 && month <= 5) season = "Spring";
    else if (month >= 6 && month <= 8) season = "Summer";
    else if (month >= 9 && month <= 11) season = "Fall";
    else season = "Winter";

    QString dateStr = QString("%1 %2 Semester | %3, %4")
        .arg(today.year())
        .arg(season)
        .arg(today.toString("dddd"))
        .arg(today.toString("MMM d"));

    QLabel *dateLabel = new QLabel(dateStr);
    dateLabel->setStyleSheet("font-size:12px; color:#888;");
    layout->addWidget(dateLabel);

    layout->addStretch();

    searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText("搜索课程、任务、文件...");
    searchEdit->setFixedWidth(260);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background: white;
            border: 1px solid #E0D5D5;
            border-radius: 16px;
            padding: 0 14px;
            font-size: 13px;
            color: #222;
        }
        QLineEdit:focus {
            border: 1px solid %1;
            background: white;
        }
        QLineEdit::placeholder {
            color: #BDBDBD;
        }
    )").arg(Theme::PRIMARY));
    layout->addWidget(searchEdit);

    searchPopup = new SearchPopup(this);

    searchTimer->setSingleShot(true);
    searchTimer->setInterval(150);

    // 安装 eventFilter 以检测点击外部的行为
    qApp->installEventFilter(this);

    connect(searchEdit, &QLineEdit::textChanged, this, &TopbarWidget::onSearchTextChanged);
    connect(searchEdit, &QLineEdit::returnPressed, this, &TopbarWidget::onSearchReturned);
    connect(searchPopup, &SearchPopup::courseSelected, this, &TopbarWidget::onCourseSelected);
    connect(searchPopup, &SearchPopup::taskSelectedByCourseAndTitle, this, &TopbarWidget::onTaskSelected);
    connect(searchPopup, &SearchPopup::fileSelected, this, &TopbarWidget::onFileSelected);
    connect(searchTimer, &QTimer::timeout, this, &TopbarWidget::doSearch);
}

bool TopbarWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (searchPopup && searchPopup->isVisible()) {
            // 检查点击位置是否在 searchEdit 或 searchPopup 内部
            bool inSearchEdit = searchEdit->geometry().contains(searchEdit->mapFromGlobal(mouseEvent->globalPos()));
            bool inPopup = searchPopup->geometry().contains(searchPopup->mapFromGlobal(mouseEvent->globalPos()));

            if (!inSearchEdit && !inPopup) {
                searchPopup->hide();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void TopbarWidget::onSearchTextChanged(const QString& text)
{
    currentSearchText = text;
    searchTimer->stop();
    searchTimer->start(150);
}

void TopbarWidget::onSearchReturned()
{
    searchTimer->stop();
    searchPopup->hide();
    QString text = searchEdit->text().trimmed();
    if (text.isEmpty()) {
        searchPopup->hide();
    } else {
        doSearch();
    }
}

void TopbarWidget::doSearch()
{
    QString text = searchEdit->text().trimmed();
    if (text.isEmpty()) {
        searchPopup->hide();
        return;
    }

    QVector<SearchResult> results = search(text);
    searchPopup->showResults(results, text);
    
    positionPopup();
    searchPopup->show();
}

void TopbarWidget::positionPopup()
{
    QPoint globalPos = searchEdit->mapToGlobal(QPoint(0, searchEdit->height() + 4));
    int popupWidth = searchEdit->width();
    searchPopup->setFixedWidth(popupWidth);
    searchPopup->move(globalPos);
}

QLineEdit* TopbarWidget::getSearchEdit() const
{
    return searchEdit;
}

TopbarWidget::~TopbarWidget()
{
}

void TopbarWidget::onCourseSelected(const QString& courseName)
{
    qDebug() << "[Topbar] Course selected:" << courseName;
    searchEdit->clear();
    searchPopup->hide();
    emit searchCourseRequested(courseName);
}

void TopbarWidget::onTaskSelected(const QString& courseAndTitle)
{
    qDebug() << "[Topbar] Task selected:" << courseAndTitle;
    searchEdit->clear();
    searchPopup->hide();
    emit searchTaskRequested(courseAndTitle);
}

void TopbarWidget::onFileSelected(const QString& filePath)
{
    qDebug() << "[Topbar] File selected:" << filePath;
    searchEdit->clear();
    searchPopup->hide();
    emit fileSelected(filePath);
}

QVector<SearchResult> TopbarWidget::search(const QString& keyword)
{
    if (keyword.trimmed().isEmpty()) {
        return {};
    }
    QVector<SearchResult> results;
    results.append(searchCourses(keyword));
    results.append(searchTasks(keyword));
    results.append(searchFiles(keyword));
    return results;
}

QVector<SearchResult> TopbarWidget::searchCourses(const QString& keyword)
{
    QVector<SearchResult> results;
    const auto courses = DataManager::instance().courses();
    const QString lower = keyword.toLower();

    for (const Course& c : courses) {
        if (c.name.toLower().contains(lower) ||
            c.teacher.toLower().contains(lower) ||
            c.location.toLower().contains(lower)) {
            SearchResult r;
            r.type = SearchResult::Course;
            r.title = c.name;
            QString timeInfo = QString("周%1 %2-%3节").arg(c.day).arg(c.startPeriod).arg(c.endPeriod);
            if (!c.location.isEmpty()) {
                r.subtitle = timeInfo + " | " + c.location;
            } else {
                r.subtitle = timeInfo;
            }
            r.id = c.name;
            r.icon = "📚";
            results.append(r);
        }
    }
    return results;
}

QVector<SearchResult> TopbarWidget::searchTasks(const QString& keyword)
{
    QVector<SearchResult> results;
    const auto tasks = DataManager::instance().tasks();
    const QString lower = keyword.toLower();

    for (int i = 0; i < tasks.size(); ++i) {
        const Task& t = tasks[i];
        if (t.title.toLower().contains(lower) ||
            t.course.toLower().contains(lower)) {
            SearchResult r;
            r.type = SearchResult::Task;
            r.title = t.title;
            r.subtitle = QString("课程: %1 | DDL: %2").arg(t.course).arg(t.deadline.toString("yyyy-MM-dd hh:mm"));
            r.id = QString::number(i);
            r.icon = t.completed ? "✅" : "📝";
            results.append(r);
        }
    }
    return results;
}

QVector<SearchResult> TopbarWidget::searchFiles(const QString& keyword)
{
    QVector<SearchResult> results;
    const auto courses = DataManager::instance().courses();
    const QString lower = keyword.toLower();

    for (const Course& c : courses) {
        const QString folderPath = c.folderPath.trimmed();
        if (folderPath.isEmpty()) continue;

        QDir dir(folderPath);
        if (!dir.exists()) continue;

        QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
        for (const QFileInfo& info : files) {
            if (info.isDir()) continue;
            if (info.fileName().toLower().contains(lower)) {
                SearchResult r;
                r.type = SearchResult::File;
                r.title = info.fileName();
                r.subtitle = QString("课程: %1 | %2").arg(c.name).arg(info.lastModified().toString("yyyy-MM-dd"));
                r.id = info.absoluteFilePath();
                r.icon = "📄";
                results.append(r);
            }
        }
    }
    return results;
}