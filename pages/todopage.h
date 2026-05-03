#ifndef TODOPAGE_H
#define TODOPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>

class QTableView;
class TaskModel;

class TodoPage : public QWidget
{
    Q_OBJECT

public:
    explicit TodoPage(QWidget *parent = nullptr);

private:
    TaskModel *model;
    QTableView *table;
    QComboBox *courseFilter;
    QComboBox *timeFilter;
    QComboBox *statusFilter;
    QLineEdit *searchEdit;

    QWidget* createFilterBar();
    void refreshCourseFilter();
    void refreshTasks();
    void applyFilter();
    void editSelectedTask();
};

#endif