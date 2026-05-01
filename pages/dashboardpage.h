#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QFrame>

class QGridLayout;

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);

private:
    QGridLayout *grid;
    void initGrid();
    QFrame* createCard(QString title);
};

#endif