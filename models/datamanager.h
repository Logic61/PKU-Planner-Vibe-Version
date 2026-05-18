#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include <QList>
#include <QString>
#include "course.h"
#include "task.h"
#include "datastore.h"
#include "datarepository.h"

class DataManager : public QObject
{
    Q_OBJECT
public:
    static DataManager& instance();
    static QString dataDirectory();

    QList<Course> courses() const;
    void addCourse(const Course& c);
    void updateCourse(int index, const Course& c);
    void deleteCourse(int index);

    QList<Task> tasks() const;
    void addTask(const Task& t);
    void updateTask(int index, const Task& t);
    void deleteTask(int index);
    void markTaskCompleted(int index, bool completed);
    void updateTasksFromPlatform(const QList<QJsonObject> &tasks);

    bool load();
    bool save();
    void clearAll();
    QString storageDir() const;

signals:
    void coursesChanged();
    void tasksChanged();

private:
    DataManager();
    ~DataManager();

    DataStore m_store;
    DataRepository m_repository;
};

#endif // DATAMANAGER_H