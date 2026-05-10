#ifndef COURSEDETAILDRAWER_H
#define COURSEDETAILDRAWER_H

#include <QFrame>
#include <QPropertyAnimation>
#include <QEvent>
#include <QEasingCurve>
#include "../../models/course.h"

class CourseHeaderWidget;
class CourseTabBar;
class CourseInfoPage;
class CourseTaskPage;
class CourseFilePage;
class QStackedWidget;
class CourseStatsWidget;

class CourseDetailDrawer : public QFrame
{
    Q_OBJECT

public:
    explicit CourseDetailDrawer(QWidget *parent=nullptr);

    void loadCourse(const Course& course);

    void openDrawer();
    void closeDrawer();

    bool isDrawerOpen() const { return isOpen; }

signals:
    void editCourseRequested(Course course);
    void deleteCourseRequested(Course course);
    void addTaskRequested(Course course);
    void courseUpdated(Course updatedCourse);
    void taskUpdated();

private:
    Course currentCourse;

    CourseHeaderWidget* headerWidget;
    CourseTabBar* tabBar;
    QStackedWidget* stackedWidget;

    CourseInfoPage* basicInfoPage;
    CourseTaskPage* taskPage;
    CourseFilePage* filePage;
    CourseStatsWidget* statsWidget;

    void setupUI();
    void setupAnimation();
    void updateDockGeometry();
    int effectiveDrawerWidth() const;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEventHandler(QMouseEvent *event);
    QPropertyAnimation* anim = nullptr;
    int drawerWidth = 480;
    bool isOpen = false;
    bool isDragging = false;
    int dragStartX = 0;
    int dragStartWidth = 0;
    const int MIN_DRAWER_WIDTH = 300;
    const int MAX_DRAWER_WIDTH = 700;
    const int DRAG_EDGE_WIDTH = 10;
    
private slots:
    void onAnimationFinished();
};

#endif
