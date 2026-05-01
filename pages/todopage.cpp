#include "todopage.h"
#include <QVBoxLayout>
#include <QLabel>

TodoPage::TodoPage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel("这里是待办页面（M2）");
    layout->addWidget(label);
}