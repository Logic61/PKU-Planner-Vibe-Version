#include "datamanager.h"

#include <QJsonParseError>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QSaveFile>
#include <QStandardPaths>

QString DataManager::dataDirectory()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataPath.isEmpty()) {
        dataPath = QCoreApplication::applicationDirPath();
    }
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dataPath;
}

DataManager& DataManager::instance()
{
    static DataManager instance;
    return instance;
}

DataManager::DataManager() : QObject(nullptr)
{
    qDebug() << "[DataManager] Constructor called";
    m_storageDir = dataDirectory();
    load();
    qDebug() << "[DataManager] Data loaded, courses:" << m_courses.size() << "tasks:" << m_tasks.size();
}

DataManager::~DataManager()
{
    // 在析构函数中保存数据
    save();
}

QList<Course> DataManager::courses() const
{
    return m_courses;
}

void DataManager::addCourse(const Course& c)
{
    m_courses.append(c);
    emit coursesChanged();
    saveCourses();
}

void DataManager::updateCourse(int index, const Course& c)
{
    if (index >= 0 && index < m_courses.size()) {
        m_courses[index] = c;
        emit coursesChanged();
        saveCourses();
    }
}

void DataManager::deleteCourse(int index)
{
    if (index >= 0 && index < m_courses.size()) {
        m_courses.removeAt(index);
        emit coursesChanged();
        saveCourses();
    }
}

QList<Task> DataManager::tasks() const
{
    return m_tasks;
}

void DataManager::addTask(const Task& t)
{
    m_tasks.append(t);
    emit tasksChanged();
    saveTasks();
}

void DataManager::updateTask(int index, const Task& t)
{
    if (index >= 0 && index < m_tasks.size()) {
        Task updatedTask = t;
        const Task &oldTask = m_tasks[index];
        
        qDebug() << "[DataManager] UpdateTask - Index:" << index << ", Old Completed:" << oldTask.completed << ", New Completed:" << updatedTask.completed;

        // If becoming completed, set completedAt
        if (updatedTask.completed && !oldTask.completed) {
            if (!updatedTask.completedAt.isValid()) {
                updatedTask.completedAt = QDateTime::currentDateTime();
            }
        }
        // If becoming incomplete, clear completedAt
        else if (!updatedTask.completed && oldTask.completed) {
            updatedTask.completedAt = QDateTime();
        }
        // If was already completed but doesn't have a time, set it
        else if (updatedTask.completed && !updatedTask.completedAt.isValid()) {
            updatedTask.completedAt = QDateTime::currentDateTime();
        }
        
        m_tasks[index] = updatedTask;
        emit tasksChanged();
        saveTasks();
        qDebug() << "[DataManager] Task updated and saved. Current task completed state:" << m_tasks[index].completed;
    }
}

void DataManager::deleteTask(int index)
{
    if (index >= 0 && index < m_tasks.size()) {
        m_tasks.removeAt(index);
        emit tasksChanged();
        saveTasks();
    }
}

void DataManager::markTaskCompleted(int index, bool completed)
{
    if (index >= 0 && index < m_tasks.size()) {
        Task task = m_tasks[index];
        if (task.completed != completed) {
            task.completed = completed;
            if (completed) {
                task.completedAt = QDateTime::currentDateTime();
            } else {
                task.completedAt = QDateTime();
            }
            m_tasks[index] = task;
            emit tasksChanged();
            saveTasks();
        }
    }
}

void DataManager::clearAll()
{
    m_courses.clear();
    m_tasks.clear();
    
    QString dir = dataDirectory();
    QDir(dir).remove("courses.json");
    QDir(dir).remove("courses.bak");
    QDir(dir).remove("tasks.json");
    QDir(dir).remove("tasks.bak");
    
    emit coursesChanged();
    emit tasksChanged();
    
    qDebug() << "[DataManager] All data cleared from memory and disk.";
}

QString DataManager::storageDir() const
{
    return m_storageDir;
}

bool DataManager::load()
{
    bool coursesLoaded = loadCourses();
    bool tasksLoaded = loadTasks();
    return coursesLoaded || tasksLoaded;
}

bool DataManager::save()
{
    bool coursesSaved = saveCourses();
    bool tasksSaved = saveTasks();
    return coursesSaved && tasksSaved;
}

bool DataManager::loadCourses()
{
    qDebug() << "[DataManager] loadCourses() starting";
    QJsonDocument doc;
    if (!loadFromFile("courses.json", doc)) {
        if (!loadFromFile("courses.bak", doc)) {
            qWarning() << "[DataManager] 无法加载课程数据，使用空数据初始化";
            m_courses.clear();
            return false;
        }
    }
    
    if (!doc.isArray()) {
        qWarning() << "[DataManager] 课程数据格式错误，不是JSON数组";
        m_courses.clear();
        return false;
    }
    
    QJsonArray arr = doc.array();
    m_courses.clear();
    
    for (const auto& item : arr) {
        if (item.isObject()) {
            m_courses.append(Course::fromJson(item.toObject()));
        }
    }
    
    qDebug() << "[DataManager] 成功加载" << m_courses.size() << "门课程，发射信号";
    emit coursesChanged();
    qDebug() << "[DataManager] coursesChanged 信号已发射";
    return true;
}

bool DataManager::loadTasks()
{
    QJsonDocument doc;
    if (!loadFromFile("tasks.json", doc)) {
        if (!loadFromFile("tasks.bak", doc)) {
            qWarning() << "无法加载任务数据，使用空数据初始化";
            m_tasks.clear();
            return false;
        }
    }
    
    if (!doc.isArray()) {
        qWarning() << "任务数据格式错误，不是JSON数组";
        m_tasks.clear();
        return false;
    }
    
    QJsonArray arr = doc.array();
    m_tasks.clear();
    
    for (const auto& item : arr) {
        if (item.isObject()) {
            m_tasks.append(Task::fromJson(item.toObject()));
        }
    }
    
    qDebug() << "[DataManager] 成功加载" << m_tasks.size() << "个任务，发射信号";
    emit tasksChanged();
    return true;
}

bool DataManager::saveCourses()
{
    QJsonArray arr;
    for (const auto& course : m_courses) {
        arr.append(course.toJson());
    }
    
    QJsonDocument doc(arr);
    return saveToFile("courses.json", doc);
}

bool DataManager::saveTasks()
{
    QJsonArray arr;
    for (const auto& task : m_tasks) {
        QJsonObject obj;
        obj["course"] = task.course;
        obj["title"] = task.title;
        obj["deadline"] = task.deadline.toString(Qt::ISODate);
        obj["priority"] = task.priority;
        obj["completed"] = task.completed;
        if (task.completed) {
            if (task.completedAt.isValid()) {
                obj["completedAt"] = task.completedAt.toString(Qt::ISODate);
            } else {
                obj["completedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            }
        }
        arr.append(obj);
    }
    
    QJsonDocument doc(arr);
    return saveToFile("tasks.json", doc);
}

bool DataManager::loadFromFile(const QString& filename, QJsonDocument& doc)
{
    QString dataDir = m_storageDir.isEmpty() ? dataDirectory() : m_storageDir;
    QString fullPath = QDir(dataDir).absoluteFilePath(filename);

    QFile file(fullPath);

    if (!file.exists()) {
        qWarning() << "[DataManager] File does not exist:" << fullPath;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[DataManager] Cannot open file:" << fullPath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || doc.isNull()) {
        qWarning() << "[DataManager] JSON parse error:" << error.errorString() << "in file:" << fullPath;
        
        QString backupPath = fullPath + "." + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".corrupted";
        QFile::copy(fullPath, backupPath);
        qDebug() << "[DataManager] Backed up corrupted file to:" << backupPath;
        qWarning() << "[DataManager] JSON parse error, file corrupted";
        
        return false;
    }

    qDebug() << "[DataManager] Successfully loaded from:" << fullPath;
    return true;
}

bool DataManager::saveToFile(const QString& filename, const QJsonDocument& doc)
{
    QString dataDir = m_storageDir.isEmpty()
        ? QCoreApplication::applicationDirPath()
        : m_storageDir;
    QString target = QDir(dataDir).absoluteFilePath(filename);

    QSaveFile file(target);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[DataManager] Cannot open file for writing:" << target;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));

    if (!file.commit()) {
        qWarning() << "[DataManager] Failed to commit file:" << target;
        return false;
    }

    qDebug() << "[DataManager] Successfully saved:" << target;
    return true;
}