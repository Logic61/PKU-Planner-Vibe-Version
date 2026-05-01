#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QStackedWidget;
class SidebarWidget;
class TopbarWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    QStackedWidget *stack;
    SidebarWidget *sidebar;
    TopbarWidget *topbar;
};

#endif