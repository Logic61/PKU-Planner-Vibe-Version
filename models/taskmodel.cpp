#include "taskmodel.h"

#include <QDate>
#include <QColor>
#include <QBrush> // 如果你要返回复杂的背景刷
#include <QDateTime>
#include <QFont>
#include <algorithm>

namespace {

bool matchesTimeFilter(const Task &task, const QString &timeFilter)
{
    if (timeFilter == "今天") {
        return task.deadline.date() == QDate::currentDate();
    }

    if (timeFilter == "本周") {
        int currentYear = 0;
        int deadlineYear = 0;
        const int currentWeek = QDate::currentDate().weekNumber(&currentYear);
        const int deadlineWeek = task.deadline.date().weekNumber(&deadlineYear);
        return currentWeek == deadlineWeek && currentYear == deadlineYear;
    }

    if (timeFilter == "逾期") {
        return task.isOverdue();
    }

    return true;
}

}

TaskModel::TaskModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int TaskModel::rowCount(const QModelIndex &) const
{
    return m_visibleTasks.size();
}

int TaskModel::columnCount(const QModelIndex &) const
{
    return 5;
}

QVariant TaskModel::headerData(int section, Qt::Orientation o, int role) const
{
    if(role != Qt::DisplayRole || o != Qt::Horizontal) return {};

    switch(section)
    {
    case 0: return "课程";
    case 1: return "任务";
    case 2: return "DDL";
    case 3: return "剩余时间";
    case 4: return "优先级";
    }
    return {};
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleTasks.size()) {
        return {};
    }

    const Task &t = m_visibleTasks[index.row()];

    // 显示内容
    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case 0: return t.course;
        case 1: return t.title;
        case 2: return t.deadline.toString("MM-dd hh:mm");
        case 3:
            if(t.isOverdue()) return "已逾期";
            return QString("%1 天").arg(t.daysLeft());
        case 4:
            return t.priority == 2 ? "高" : (t.priority == 1 ? "中" : "低");
        }
    }

    if (role == Qt::ForegroundRole && t.completed) {
        return QBrush(QColor("#9E9E9E"));
    }

    if (role == Qt::FontRole && t.completed) {
        QFont font;
        font.setStrikeOut(true);
        return font;
    }

    // 行背景（核心视觉）
    if(role == Qt::BackgroundRole)
    {
        if (t.completed) return QColor("#F3F3F3");
        if(t.isOverdue()) return QColor("#FFEAEA");
        if(t.daysLeft() <= 1) return QColor("#FFF3E0");
    }

    return {};
}

void TaskModel::setTasks(const QList<Task> &tasks)
{
    beginResetModel();
    m_allTasks = tasks;
    rebuildVisibleTasks();
    endResetModel();
}

void TaskModel::setFilter(const QString &courseName,
                          const QString &timeFilter,
                          const QString &statusFilter,
                          const QString &keyword)
{
    m_courseName = courseName;
    m_timeFilter = timeFilter;
    m_statusFilter = statusFilter;
    m_keyword = keyword;
    beginResetModel();
    rebuildVisibleTasks();
    endResetModel();
}

void TaskModel::rebuildVisibleTasks()
{
    m_visibleSourceIndices.clear();

    QList<int> order;

    for (int i = 0; i < m_allTasks.size(); ++i) {
        const Task &task = m_allTasks[i];
        if (!m_courseName.isEmpty() && m_courseName != "全部课程" && task.course != m_courseName) {
            continue;
        }

        if (!m_statusFilter.isEmpty() && m_statusFilter != "全部状态") {
            if (m_statusFilter == "未完成" && task.completed) {
                continue;
            }
            if (m_statusFilter == "已完成" && !task.completed) {
                continue;
            }
        }

        if (!m_timeFilter.isEmpty() && m_timeFilter != "全部时间" && !matchesTimeFilter(task, m_timeFilter)) {
            continue;
        }

        if (!m_keyword.isEmpty()) {
            const QString lowered = m_keyword.trimmed().toLower();
            if (!task.course.toLower().contains(lowered) && !task.title.toLower().contains(lowered)) {
                continue;
            }
        }

        order.append(i);
    }

    std::sort(order.begin(), order.end(), [this](int left, int right) {
        const Task &a = m_allTasks[left];
        const Task &b = m_allTasks[right];

        if (a.completed != b.completed) {
            return !a.completed && b.completed;
        }

        if (a.deadline != b.deadline) {
            return a.deadline < b.deadline;
        }

        if (a.priority != b.priority) {
            return a.priority > b.priority;
        }

        return a.title < b.title;
    });

    m_visibleTasks.clear();
    for (int sourceIndex : order) {
        m_visibleTasks.append(m_allTasks[sourceIndex]);
        m_visibleSourceIndices.append(sourceIndex);
    }
}

int TaskModel::sourceIndexAt(int row) const
{
    if (row < 0 || row >= m_visibleSourceIndices.size()) {
        return -1;
    }
    return m_visibleSourceIndices[row];
}

Task TaskModel::taskAt(int row) const
{
    if (row < 0 || row >= m_visibleTasks.size()) {
        return Task{};
    }
    return m_visibleTasks[row];
}