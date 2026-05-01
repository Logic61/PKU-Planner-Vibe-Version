#include "mainwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QWidget>

#include "ui/sidebarwidget.h"
#include "ui/topbarwidget.h"

#include "pages/dashboardpage.h"
#include "pages/todopage.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget;
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);

    // 左侧导航
    sidebar = new SidebarWidget;
    sidebar->setFixedWidth(200);

    // 右侧区域
    QWidget *right = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0,0,0,0);

    // 顶栏
    topbar = new TopbarWidget;

    // 页面栈
    stack = new QStackedWidget;

    stack->addWidget(new DashboardPage); // index 0
    stack->addWidget(new TodoPage);      // index 1

    rightLayout->addWidget(topbar);
    rightLayout->addWidget(stack);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(right);

    // 页面切换
    connect(sidebar, &SidebarWidget::pageChanged,
            stack, &QStackedWidget::setCurrentIndex);
}