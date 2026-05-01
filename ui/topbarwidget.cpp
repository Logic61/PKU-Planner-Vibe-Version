#include "topbarwidget.h"
#include <QHBoxLayout>
#include <QLabel>

TopbarWidget::TopbarWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(50);
    setStyleSheet("background:#F7F3EF;");

    QHBoxLayout *layout = new QHBoxLayout(this);

    QLabel *title = new QLabel("PKU Planner+");
    title->setStyleSheet("font-weight:bold; font-size:16px;");

    layout->addWidget(title);
    layout->addStretch();
}