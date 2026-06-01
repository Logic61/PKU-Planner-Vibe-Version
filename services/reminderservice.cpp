#include "reminderservice.h"
#include "../models/datamanager.h"
#include "../models/task.h"
#include <QDebug>
#include <QSet>

ReminderService::ReminderService(QObject *parent)
    : QObject(parent)
    , m_running(false)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ReminderService::checkUpcomingTasks);
}

ReminderService::~ReminderService()
{
    stop();
}

void ReminderService::start()
{
    if (m_running) return;
    m_running = true;
    m_timer->start(30 * 60 * 1000);
    qDebug() << "[ReminderService] Started - will check every 30 minutes";
    checkUpcomingTasks();
}

void ReminderService::stop()
{
    m_running = false;
    m_timer->stop();
    qDebug() << "[ReminderService] Stopped";
}

void ReminderService::checkUpcomingTasks()
{
    const QList<Task> &tasks = DataManager::instance().tasks();
    QDateTime now = QDateTime::currentDateTime();

    // Clear notifications for tasks that are no longer in the reminder window
    // or have been completed/removed
    QSet<QString> activeKeys;
    for (const Task &task : tasks) {
        if (task.completed || !task.deadline.isValid()) continue;
        qint64 hoursUntilDeadline = now.secsTo(task.deadline) / 3600;
        if (hoursUntilDeadline > 0 && hoursUntilDeadline <= 24) {
            QString key = QString("%1::%2").arg(task.course).arg(task.title);
            activeKeys.insert(key);
            if (!m_notifiedTasks.contains(key)) {
                m_notifiedTasks.insert(key);
                qDebug() << "[ReminderService] Upcoming deadline:" << task.course << task.title
                         << "due in" << hoursUntilDeadline << "hours";
                emit reminderTriggered(task.course, task.title, task.deadline);
            }
        }
    }
    // Remove stale entries
    m_notifiedTasks = activeKeys;
}