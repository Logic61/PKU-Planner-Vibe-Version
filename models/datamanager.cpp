#include "datamanager.h"

#include <QJsonParseError>
#include <QDateTime>

DataManager& DataManager::instance()
{
    static DataManager instance;
    return instance;
}

DataManager::DataManager() : QObject(nullptr)
{
    // 在构造函数中加载数据
    qDebug() << "[DataManager] Constructor called, loading data";
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
    Course newCourse = c;
    const int existingIndex = courseIndexByName(c.name);
    if (existingIndex >= 0) {
        const Course &existing = m_courses[existingIndex];
        if (!existing.teacher.isEmpty()) {
            newCourse.teacher = existing.teacher;
        }
        if (!existing.location.isEmpty()) {
            newCourse.location = existing.location;
        }
        if (!existing.examTime.isEmpty()) {
            newCourse.examTime = existing.examTime;
        }
    }
    m_courses.append(newCourse);
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
        m_tasks[index] = t;
        emit tasksChanged();
        saveTasks();
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
        m_tasks[index].completed = completed;
        emit tasksChanged();
        saveTasks();
    }
}

bool DataManager::load()
{
    bool coursesLoaded = loadCourses();
    bool tasksLoaded = loadTasks();
    return coursesLoaded && tasksLoaded;
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
        // 尝试从备份加载
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
        // 尝试从备份加载
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
            // 假设Task有fromJson方法，如果没有需要添加
            Task task;
            QJsonObject obj = item.toObject();
            task.course = obj["course"].toString();
            task.title = obj["title"].toString();
            task.deadline = QDateTime::fromString(obj["deadline"].toString(), Qt::ISODate);
            task.priority = obj["priority"].toInt();
            task.completed = obj["completed"].toBool();
            m_tasks.append(task);
        }
    }
    
    qDebug() << "成功加载" << m_tasks.size() << "个任务";
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
        arr.append(obj);
    }
    
    QJsonDocument doc(arr);
    return saveToFile("tasks.json", doc);
}

bool DataManager::loadFromFile(const QString& filename, QJsonDocument& doc)
{
    QFile file(filename);
    
    if (!file.exists()) {
        qWarning() << "文件不存在:" << filename;
        return false;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件:" << filename;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError || doc.isNull()) {
        qWarning() << "JSON解析错误:" << error.errorString() << "在文件:" << filename;
        return false;
    }
    
    return true;
}

bool DataManager::saveToFile(const QString& filename, const QJsonDocument& doc)
{
    // 先保存到临时文件，然后重命名，避免写入失败导致数据丢失
    QString tempFile = filename + ".tmp";
    QString backupFile = filename + ".bak";
    
    // 1. 如果存在原文件，先备份
    if (QFile::exists(filename)) {
        if (QFile::exists(backupFile)) {
            QFile::remove(backupFile);
        }
        QFile::copy(filename, backupFile);
    }
    
    // 2. 写入临时文件
    QFile file(tempFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法创建临时文件:" << tempFile;
        return false;
    }
    
    qint64 bytesWritten = file.write(doc.toJson());
    if (!file.flush()) {
        qWarning() << "刷新文件失败:" << tempFile;
        file.close();
        QFile::remove(tempFile);
        return false;
    }
    file.close();
    
    if (bytesWritten == -1) {
        qWarning() << "写入文件失败:" << tempFile;
        QFile::remove(tempFile);
        return false;
    }
    
    // 3. 将临时文件重命名为正式文件
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    
    if (!QFile::rename(tempFile, filename)) {
        qWarning() << "重命名文件失败:" << tempFile << "->" << filename;
        return false;
    }
    
    qDebug() << "成功保存文件:" << filename;
    return true;
}

int DataManager::courseIndexByName(const QString& name) const
{
    for (int i = 0; i < m_courses.size(); ++i) {
        if (m_courses[i].name == name) {
            return i;
        }
    }
    return -1;
}