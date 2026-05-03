#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QVBoxLayout>

#include "../models/course.h"
#include "../models/task.h"
#include <vector>

class QGridLayout;
class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;
class QHBoxLayout;

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);

public slots:
    void createCourse(int row, int col);
    void editCourse(int index); // for editing/deleting

private:
    // std::vector<Course> courses; Removed and handled by DataManager

    void renderCourses();
    
    int getNearestDDL(const QString& courseName);
    
    QGridLayout *grid;

    QLabel *weekLabel;
    QLabel *timeLabel;
    QProgressBar *semesterProgress;

    int currentWeek = 9;
    int realWeek = 9;

    void initGrid();
    QWidget* createTopBar();
    QWidget* createBottomStats();
    QWidget* createRightPanel();
    
    QVBoxLayout *ddlLayout;
    
    void updateDDLWidget();
    void updateWeekInfo();
};

#endif