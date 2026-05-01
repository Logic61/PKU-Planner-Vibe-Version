#ifndef COURSECELLWIDGET_H
#define COURSECELLWIDGET_H

#include <QFrame>

class QLabel;

class CourseCellWidget : public QFrame
{
    Q_OBJECT

public:
    explicit CourseCellWidget(QWidget *parent = nullptr);

    void setCourse(QString name, QString location);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QLabel *title;
    QLabel *info;
};

#endif