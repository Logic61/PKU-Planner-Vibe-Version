#ifndef WEEKLYSUMMARYSERVICE_H
#define WEEKLYSUMMARYSERVICE_H

#include <QString>
#include <QDateTime>
#include <QMap>

struct WeeklySummary {
    int totalTasks;
    int completedTasks;
    int overdueTasks;
    int upcomingTasks;
    double completionRate;
    QString busiestCourse;
    int busiestCourseTasks;
    int busiestCourseCompletedTasks;
    double avgEarlyDays;
    QString suggestion;
    QString mascotMessage;
};

class WeeklySummaryService {
public:
    static WeeklySummary generate();
    static bool shouldShowOnStartup();
    static void markSummaryShown();

private:
    static QDate getWeekStart(const QDate& date);
    static bool isThisWeek(const QDateTime& deadline);
    static QString generateSuggestion(int overdue, double rate, const QString& busiest);
    static QString generateMascotMessage(int completed, int total, int overdue);
};

#endif