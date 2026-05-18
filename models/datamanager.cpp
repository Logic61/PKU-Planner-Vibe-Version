#include "datamanager.h"

#include <QDateTime>
#include <QDir>
#include <QDebug>

QString DataManager::dataDirectory()
{
    return DataRepository::dataDirectory();
}

DataManager& DataManager::instance()
{
    static DataManager instance;
    return instance;
}

DataManager::DataManager()
    : QObject(nullptr)
    , m_repository(dataDirectory())
{
    qDebug() << "[DataManager] Constructor called";
    load();
    qDebug() << "[DataManager] Data loaded, courses:" << m_store.courses().size() << "tasks:" << m_store.tasks().size();
}

DataManager::~DataManager()
{
    save();
}

QList<Course> DataManager::courses() const
{
    return m_store.courses();
}

void DataManager::addCourse(const Course& c)
{
    m_store.addCourse(c);
    emit coursesChanged();
    m_repository.saveCourses(m_store.courses());
}

void DataManager::updateCourse(int index, const Course& c)
{
    if (index >= 0 && index < m_store.courses().size()) {
        m_store.updateCourse(index, c);
        emit coursesChanged();
        m_repository.saveCourses(m_store.courses());
    }
}

void DataManager::deleteCourse(int index)
{
    if (index >= 0 && index < m_store.courses().size()) {
        m_store.deleteCourse(index);
        emit coursesChanged();
        m_repository.saveCourses(m_store.courses());
    }
}

QList<Task> DataManager::tasks() const
{
    return m_store.tasks();
}

void DataManager::addTask(const Task& t)
{
    m_store.addTask(t);
    emit tasksChanged();
    m_repository.saveTasks(m_store.tasks());
}

void DataManager::updateTask(int index, const Task& t)
{
    if (index >= 0 && index < m_store.tasks().size()) {
        Task updatedTask = t;
        const Task& oldTask = m_store.tasks().at(index);

        qDebug() << "[DataManager] UpdateTask - Index:" << index << ", Old Completed:" << oldTask.completed << ", New Completed:" << updatedTask.completed;

        if (updatedTask.completed && !oldTask.completed) {
            if (!updatedTask.completedAt.isValid()) {
                updatedTask.completedAt = QDateTime::currentDateTime();
            }
        }
        else if (!updatedTask.completed && oldTask.completed) {
            updatedTask.completedAt = QDateTime();
        }
        else if (updatedTask.completed && !updatedTask.completedAt.isValid()) {
            updatedTask.completedAt = QDateTime::currentDateTime();
        }

        m_store.updateTask(index, updatedTask);
        emit tasksChanged();
        m_repository.saveTasks(m_store.tasks());
        qDebug() << "[DataManager] Task updated and saved. Current task completed state:" << m_store.tasks().at(index).completed;
    }
}

void DataManager::deleteTask(int index)
{
    if (index >= 0 && index < m_store.tasks().size()) {
        m_store.deleteTask(index);
        emit tasksChanged();
        m_repository.saveTasks(m_store.tasks());
    }
}

void DataManager::markTaskCompleted(int index, bool completed)
{
    if (index >= 0 && index < m_store.tasks().size()) {
        QList<Task> taskList = m_store.tasks();
        Task task = taskList.at(index);
        if (task.completed != completed) {
            task.completed = completed;
            if (completed) {
                task.completedAt = QDateTime::currentDateTime();
            } else {
                task.completedAt = QDateTime();
            }
            m_store.updateTask(index, task);
            emit tasksChanged();
            m_repository.saveTasks(m_store.tasks());
        }
    }
}


void DataManager::updateTasksFromPlatform(const QList<QJsonObject> &tasks) {
    for (const QJsonObject &taskJson : tasks) {
        Task newTask;

        // Title: try common keys
        QString title = taskJson.value("title").toString();
        if (title.isEmpty()) title = taskJson.value("name").toString();
        if (title.isEmpty()) title = taskJson.value("summary").toString();
        newTask.title = title;

        // Course: try multiple possible keys, provide fallback
        QString course = taskJson.value("course").toString();
        if (course.isEmpty()) course = taskJson.value("courseName").toString();
        if (course.isEmpty()) course = taskJson.value("class").toString();
        if (course.isEmpty()) course = taskJson.value("course_title").toString();
        if (course.isEmpty()) course = QStringLiteral("未知课程");
        newTask.course = course;

        // Deadline: try common keys and parse ISO date; leave invalid if not provided
        QString dl = taskJson.value("deadline").toString();
        if (dl.isEmpty()) dl = taskJson.value("dueDate").toString();
        if (dl.isEmpty()) dl = taskJson.value("endTime").toString();
        if (!dl.isEmpty()) {
            QDateTime dt = QDateTime::fromString(dl, Qt::ISODate);
            if (!dt.isValid()) {
                // Try common alternative formats
                dt = QDateTime::fromString(dl, Qt::TextDate);
            }
            newTask.deadline = dt;
        } else {
            newTask.deadline = QDateTime();
        }

        // Priority and completion
        newTask.priority = taskJson.value("priority").toInt(0);
        newTask.completed = taskJson.value("completed").toBool(false);
        if (newTask.completed) {
            QString cat = taskJson.value("completedAt").toString();
            newTask.completedAt = QDateTime::fromString(cat, Qt::ISODate);
            if (!newTask.completedAt.isValid()) newTask.completedAt = QDateTime::currentDateTime();
        } else {
            newTask.completedAt = QDateTime();
        }

        if (newTask.title.isEmpty()) {
            qWarning() << "[DataManager] Task missing title, skipping object:" << taskJson;
            continue;
        }

        m_store.addTask(newTask);
    }
    emit tasksChanged();
    m_repository.saveTasks(m_store.tasks());
}


void DataManager::clearAll()
{
    m_store.clear();
    m_repository.clearDisk();

    emit coursesChanged();
    emit tasksChanged();

    qDebug() << "[DataManager] All data cleared from memory and disk.";
}

QString DataManager::storageDir() const
{
    return m_repository.storageDir();
}

bool DataManager::load()
{
    QList<Course> courses;
    QList<Task> tasks;
    bool coursesLoaded = m_repository.loadCourses(courses);
    bool tasksLoaded = m_repository.loadTasks(tasks);

    m_store.setCourses(courses);
    m_store.setTasks(tasks);

    if (coursesLoaded) emit coursesChanged();
    if (tasksLoaded) emit tasksChanged();

    return coursesLoaded || tasksLoaded;
}

bool DataManager::save()
{
    bool coursesSaved = m_repository.saveCourses(m_store.courses());
    bool tasksSaved = m_repository.saveTasks(m_store.tasks());
    return coursesSaved && tasksSaved;
}