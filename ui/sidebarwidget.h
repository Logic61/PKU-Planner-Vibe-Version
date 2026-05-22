#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>
#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include "../models/mascotstate.h"

class SidebarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);

signals:
    void pageChanged(int index);
    void mascotClicked();
    void connectTeachingPlatformRequested();

public slots:
    void onMascotClicked() { emit mascotClicked(); }
    void setActivePage(int index);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *mascotLabel = nullptr;
    QPushButton *btnDashboard = nullptr;
    QPushButton *btnTodo = nullptr;
    QPushButton *btnStats = nullptr;
    QPushButton *btnSettings = nullptr;
    QPushButton *btnConnect = nullptr;
};

#endif