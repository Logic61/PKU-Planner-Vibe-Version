#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include <QList>
#include <QFile>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QString>
#include "course.h"
#include "task.h"

class DataManager : public QObject
{
    Q_OBJECT
public:
    static DataManager& instance();
    static QString dataDirectory();

    // 课程管理
    QList<Course> courses() const;
    void addCourse(const Course& c);
    void updateCourse(int index, const Course& c);
    void deleteCourse(int index);

    // 任务管理
    QList<Task> tasks() const;
    void addTask(const Task& t);
    void updateTask(int index, const Task& t);
    void deleteTask(int index);
    void markTaskCompleted(int index, bool completed);

    // 持久化
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
    
    bool loadCourses();
    bool loadTasks();
    bool saveCourses();
    bool saveTasks();
    bool loadFromFile(const QString& filename, QJsonDocument& doc);
    bool saveToFile(const QString& filename, const QJsonDocument& doc);
    int courseIndexByName(const QString& name) const;

    QList<Course> m_courses;
    QList<Task> m_tasks;
    QString m_storageDir;
};

#endif // DATAMANAGER_H