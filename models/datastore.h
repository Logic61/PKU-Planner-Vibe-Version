#pragma once

#include <QList>
#include "course.h"
#include "task.h"

// High‑cohesion interface for the in‑memory store
class IDataStore {
public:
    virtual ~IDataStore() = default;
    virtual QList<Course> courses() const = 0;
    virtual void addCourse(const Course& c) = 0;
    virtual void updateCourse(int index, const Course& c) = 0;
    virtual void deleteCourse(int index) = 0;

    virtual QList<Task> tasks() const = 0;
    virtual void addTask(const Task& t) = 0;
    virtual void updateTask(int index, const Task& t) = 0;
    virtual void deleteTask(int index) = 0;
    virtual void markTaskCompleted(int index, bool completed) = 0;
};

class DataStore : public IDataStore {
public:
    DataStore() = default;

    // IDataStore implementation
    QList<Course> courses() const { return m_courses; }
    void addCourse(const Course& c) { m_courses.append(c); }
    void updateCourse(int index, const Course& c) {
        if (index >= 0 && index < m_courses.size()) {
            m_courses[index] = c;
        }
    }
    void deleteCourse(int index) {
        if (index >= 0 && index < m_courses.size()) {
            m_courses.removeAt(index);
        }
    }

    QList<Task> tasks() const { return m_tasks; }
    void addTask(const Task& t) { m_tasks.append(t); }
    void updateTask(int index, const Task& t) {
        if (index >= 0 && index < m_tasks.size()) { m_tasks[index] = t; }
    }
    void deleteTask(int index) {
        if (index >= 0 && index < m_tasks.size()) { m_tasks.removeAt(index); }
    }
    void markTaskCompleted(int index, bool completed) {
        if (index >= 0 && index < m_tasks.size()) { m_tasks[index].completed = completed; }
    }

    // Additional methods used by DataManager
    void clear() { m_courses.clear(); m_tasks.clear(); }
    void setCourses(const QList<Course>& c) { m_courses = c; }
    void setTasks(const QList<Task>& t) { m_tasks = t; }

private:
    QList<Course> m_courses;
    QList<Task> m_tasks;
};
