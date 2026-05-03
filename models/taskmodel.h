#ifndef TASKMODEL_H
#define TASKMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QString>
#include "task.h"

class TaskModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TaskModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override;

    QVariant data(const QModelIndex &, int role) const override;
    QVariant headerData(int, Qt::Orientation, int) const override;

    void setTasks(const QList<Task> &tasks);
    void setFilter(const QString &courseName,
                   const QString &timeFilter,
                   const QString &statusFilter,
                   const QString &keyword);
    int sourceIndexAt(int row) const;
    Task taskAt(int row) const;

private:
    void rebuildVisibleTasks();

    QList<Task> m_allTasks;
    QList<Task> m_visibleTasks;
    QList<int> m_visibleSourceIndices;
    QString m_courseName;
    QString m_timeFilter;
    QString m_statusFilter;
    QString m_keyword;
};

#endif