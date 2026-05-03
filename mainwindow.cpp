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

    // Add placeholder widgets
    stack->addWidget(new QWidget()); // index 0 - will be replaced by DashboardPage
    stack->addWidget(new QWidget()); // index 1 - will be replaced by TodoPage

    rightLayout->addWidget(topbar);
    rightLayout->addWidget(stack);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(right);

    // 页面切换
    connect(sidebar, &SidebarWidget::pageChanged,
            stack, &QStackedWidget::setCurrentIndex);
    
    // Defer page initialization
    QMetaObject::invokeMethod(this, "initPages", Qt::QueuedConnection);
}

void MainWindow::initPages()
{
    if (pagesInitialized) return;
    pagesInitialized = true;
    // Remove placeholder widgets and add real pages
    QWidget *oldPage0 = stack->widget(0);
    QWidget *oldPage1 = stack->widget(1);
    stack->removeWidget(oldPage0);
    stack->removeWidget(oldPage1);
    delete oldPage0;
    delete oldPage1;
    stack->insertWidget(0, new DashboardPage);
    stack->insertWidget(1, new TodoPage);
}