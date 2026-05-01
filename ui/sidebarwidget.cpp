#include "sidebarwidget.h"
#include <QVBoxLayout>
#include <QPushButton>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background:#8B1E2D; color:white;");

    QVBoxLayout *layout = new QVBoxLayout(this);

    QPushButton *btnDashboard = new QPushButton("课程表");
    QPushButton *btnTodo = new QPushButton("待办");

    btnDashboard->setStyleSheet("background:white; color:#8B1E2D;");
    btnTodo->setStyleSheet("background:white; color:#8B1E2D;");

    layout->addWidget(btnDashboard);
    layout->addWidget(btnTodo);
    layout->addStretch();

    connect(btnDashboard, &QPushButton::clicked, [=](){
        emit pageChanged(0);
    });

    connect(btnTodo, &QPushButton::clicked, [=](){
        emit pageChanged(1);
    });
}