#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QDateTime>

struct Task
{
    QString course;
    QString title;
    QDateTime deadline;
    int priority; // 0=低 1=中 2=高
    bool completed = false;
    QDateTime completedAt;

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