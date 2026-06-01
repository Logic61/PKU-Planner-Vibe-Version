#include "searchservice.h"
#include "../models/datamanager.h"
#include "../models/course.h"
#include "../models/task.h"
#include <QDir>

QVector<SearchResult> SearchService::search(const QString& keyword)
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

QVector<SearchResult> SearchService::searchCourses(const QString& keyword)
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

QVector<SearchResult> SearchService::searchTasks(const QString& keyword)
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
            r.id = QString("%1::%2").arg(t.course).arg(t.title);
            r.icon = t.completed ? "✅" : "📝";
            results.append(r);
        }
    }
    return results;
}

QVector<SearchResult> SearchService::searchFiles(const QString& keyword)
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