#ifndef COURSEINFOPAGE_H
#define COURSEINFOPAGE_H

#include <QWidget>
#include "../../models/course.h"

class QProgressBar;
class QLabel;
class QTextEdit;
class QPushButton;

class CourseInfoPage : public QWidget
{
    Q_OBJECT

public:
    explicit CourseInfoPage(QWidget* parent=nullptr);

    void loadCourse(const Course& course);
    void refreshProgress(int completedTasks,int totalTasks);

signals:
    void courseUpdated(Course updatedCourse);
    void editRequested(Course course);

private slots:
    void editContact();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    Course currentCourse;

    QProgressBar* progressBar;
    QLabel* progressLabel;

    QLabel* scheduleLabel;
    QLabel* locationLabel;

    QLabel* examLabel;

    QLabel* teacherLabel;
    QLabel* contactLabel;

    QTextEdit* noteEdit;

    QPushButton* editBtn;

    QWidget* createProgressCard();
    QWidget* createScheduleCard();
    QWidget* createExamCard();
    QWidget* createTeacherCard();
    QWidget* createNoteCard();
    QString scheduleSummaryForCourse(const QString& courseName) const;
    QString locationSummaryForCourse(const QString& courseName) const;
};

#endif // COURSEINFOPAGE_H