#include "weeklysummaryservice.h"
#include "../models/datamanager.h"
#include "../models/task.h"
#include "../services/configservice.h"
#include <QMap>
#include <QMapIterator>

QDate WeeklySummaryService::getWeekStart(const QDate& date)
{
    int daysToMonday = (date.dayOfWeek() - 1) % 7;
    return date.addDays(-daysToMonday);
}

bool WeeklySummaryService::isThisWeek(const QDateTime& deadline)
{
    QDate today = QDate::currentDate();
    QDate weekStart = getWeekStart(today);
    QDate weekEnd = weekStart.addDays(7);
    QDate deadlineDate = deadline.date();
    return deadlineDate >= weekStart && deadlineDate < weekEnd;
}

QString WeeklySummaryService::generateSuggestion(int overdue, double rate, const QString& busiest)
{
    if (overdue > 3) {
        return "建议提升DDL规划能力，逾期任务较多需要重点关注";
    }
    if (!busiest.isEmpty() && rate < 60) {
        return QString("建议优先处理 %1 课程的任务，该课程任务较多").arg(busiest);
    }
    if (rate >= 90) {
        return "你本周状态非常棒！继续保持高效学习";
    }
    if (rate >= 70) {
        return "本周表现不错，建议保持当前节奏";
    }
    if (overdue > 0) {
        return "建议关注逾期任务，及时补上进度";
    }
    return "本周任务较少，建议做好下周规划";
}

QString WeeklySummaryService::generateMascotMessage(int completed, int total, int overdue)
{
    if (total == 0) {
        return "这周没什么任务呢，好好休息一下吧～";
    }
    if (overdue > 3) {
        return QString("哇，有 %1 个任务逾期了！加油加油，一个一个来！").arg(overdue);
    }
    if (completed == total && total > 0) {
        return "太厉害了！所有任务都完成了，给你点赞！";
    }
    if (completed > 0) {
        return QString("已经完成了 %1/%2 个任务，继续保持哦！").arg(completed).arg(total);
    }
    return QString("这周有 %1 个任务等着你呢，开始行动吧！").arg(total);
}

WeeklySummary WeeklySummaryService::generate()
{
    QDate today = QDate::currentDate();
    QDate weekStart = getWeekStart(today);
    QDate weekEnd = weekStart.addDays(7);

    const QList<Task>& tasks = DataManager::instance().tasks();

    WeeklySummary summary = {0, 0, 0, 0, 0.0, "", 0, 0, 0.0, "", ""};
    QMap<QString, int> courseTaskCount;
    QMap<QString, int> courseCompletedCount;

    for (int i = 0; i < tasks.size(); ++i) {
        const Task& task = tasks[i];
        QDate d = task.deadline.date();
        QDate completedDate = task.completedAt.date();

        if (d >= weekStart && d < weekEnd) {
            summary.totalTasks++;
            if (task.completed) {
                summary.completedTasks++;
            } else if (d < today) {
                summary.overdueTasks++;
            } else {
                summary.upcomingTasks++;
            }
            courseTaskCount[task.course]++;
            if (task.completed) {
                courseCompletedCount[task.course]++;
            }
        } else if (task.completed && completedDate >= weekStart && completedDate < weekEnd && d >= weekEnd) {
            summary.totalTasks++;
            summary.completedTasks++;
            courseTaskCount[task.course]++;
            courseCompletedCount[task.course]++;
        }
    }

    for (auto it = courseTaskCount.constBegin(); it != courseTaskCount.constEnd(); ++it) {
        if (it.value() > summary.busiestCourseTasks) {
            summary.busiestCourse = it.key();
            summary.busiestCourseTasks = it.value();
            summary.busiestCourseCompletedTasks = courseCompletedCount.value(it.key(), 0);
        }
    }

    if (summary.totalTasks > 0) {
        summary.completionRate = (double)summary.completedTasks / summary.totalTasks * 100;
    }

    summary.suggestion = generateSuggestion(summary.overdueTasks, summary.completionRate, summary.busiestCourse);
    summary.mascotMessage = generateMascotMessage(summary.completedTasks, summary.totalTasks, summary.overdueTasks);

    return summary;
}

bool WeeklySummaryService::shouldShowOnStartup()
{
    ConfigService& config = ConfigService::instance();
    QDate lastShown = config.getLastSummaryDate();
    QDate today = QDate::currentDate();
    
    int todayWeek = getWeekStart(today).toJulianDay();
    int lastWeek = lastShown.isValid() ? getWeekStart(lastShown).toJulianDay() : -1;
    
    if (todayWeek > lastWeek) {
        return true;
    }
    return false;
}

void WeeklySummaryService::markSummaryShown()
{
    ConfigService& config = ConfigService::instance();
    config.setLastSummaryDate(QDate::currentDate());
}