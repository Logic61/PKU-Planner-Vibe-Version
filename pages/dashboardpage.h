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
class EmptyStateWidget;

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);

public slots:
    void createCourse(int row, int col);
    void editCourse(int index); // for editing/deleting
    void editCourseDirect(int index); // Direct edit without ActionDialog
    void applyCourseUpdate(const Course& updatedCourse);
    void refreshCourseUrgency();
    void importSchedule();
    void parseTimeString(const QString& timeStr, Course& c);

signals:
    void navigateToTodoPageRequested();
    void openCourseDetail(const Course& course);

private:
    // std::vector<Course> courses; Removed and handled by DataManager

    void renderCourses();
    
    int getNearestDDL(const QString& courseName);
    
    QGridLayout *grid;
    QWidget *gridContainer;

    QLabel *weekLabel;
    QLabel *timeLabel;
    QProgressBar *semesterProgress;

    int currentWeek = 9;
    int realWeek = 9;

    void initGrid();
    QWidget* createTopBar();
    QWidget* createBottomStats();
    QWidget* createRightPanel();
    void updateBottomStats();
    
    QVBoxLayout *ddlLayout;
    QVBoxLayout *todayCourseLayout = nullptr;
    QLabel *todayCourseValue = nullptr;
    QLabel *todayDdlValue = nullptr;
    QLabel *weekDdlValue = nullptr;

    void updateDDLWidget();
    void updateTodayCourses();
    void updateWeekInfo();
    QWidget* createSuggestionCard();
};

#endif