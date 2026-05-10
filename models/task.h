#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>

struct Task
{
    QString course;
    QString title;
    QDateTime deadline;
    int priority; // 0=低 1=中 2=高
    bool completed = false;
    QDateTime completedAt;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["course"] = course;
        obj["title"] = title;
        obj["deadline"] = deadline.toString(Qt::ISODate);
        obj["priority"] = priority;
        obj["completed"] = completed;
        obj["completedAt"] = completedAt.toString(Qt::ISODate);
        return obj;
    }

    static Task fromJson(QJsonObject obj) {
        Task t;
        t.course = obj["course"].toString();
        t.title = obj["title"].toString();
        t.deadline = QDateTime::fromString(obj["deadline"].toString(), Qt::ISODate);
        t.priority = obj["priority"].toInt();
        t.completed = obj["completed"].toBool();
        t.completedAt = QDateTime::fromString(obj["completedAt"].toString(), Qt::ISODate);
        return t;
    }

    int daysLeft() const
    {
        return QDateTime::currentDateTime().daysTo(deadline);
    }

    bool isOverdue() const
    {
        return deadline < QDateTime::currentDateTime() && !completed;
    }

    bool hasCompletionTime() const
    {
        return completed && completedAt.isValid();
    }
};

#endif