#ifndef REMINDERSERVICE_H
#define REMINDERSERVICE_H

#include <QObject>
#include <QTimer>
#include <QDateTime>

class ReminderService : public QObject
{
    Q_OBJECT
public:
    explicit ReminderService(QObject *parent = nullptr);
    ~ReminderService();

    void start();
    void stop();

    void checkUpcomingTasks();

signals:
    void reminderTriggered(const QString &course, const QString &title, const QDateTime &deadline);

private:
    QTimer *m_timer;
    bool m_running;
    QSet<QString> m_notifiedTasks; // track "course::title" to avoid duplicate reminders
};

#endif