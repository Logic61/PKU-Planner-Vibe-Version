#ifndef COURSECELLWIDGET_H
#define COURSECELLWIDGET_H

#include <QFrame>
#include "../models/course.h"

class QLabel;
class QMouseEvent;
class QTimer;
class DDLPreviewWidget;

class CourseCellWidget : public QFrame
{
    Q_OBJECT

public:
    explicit CourseCellWidget(int row = 0, int col = 0, QWidget *parent = nullptr);

    void setCourse(QString name, QString location, QString teacher, int index = -1, int daysLeft = -999, QString scheduleSummary = QString());

signals:
    void createCourseRequested(int row, int col);
    void editCourseRequested(int index);
    void editCourseDirectlyRequested(int index);
    void deleteCourseRequested(int index);
    void addDDLRequested(const QString &courseName);
    void navigateToTodoPageRequested();
    void courseDoubleClicked(const Course& course);
    void courseClicked(const Course& course);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QLabel *title;
    QLabel *info;
    int m_row;
    int m_col;
    int m_index = -1;
    DDLPreviewWidget *m_preview = nullptr;
    QTimer *m_showTimer = nullptr;
    QTimer *m_hideTimer = nullptr;
    int m_hoverSerial = 0;
    int m_showToken = 0;
    QString m_courseName;
    QTimer *m_clickTimer = nullptr;
    bool m_clicked = false;
};

#endif
