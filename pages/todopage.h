#ifndef TODOPAGE_H
#define TODOPAGE_H

#include <QWidget>

class TodoPage : public QWidget
{
    Q_OBJECT

public:
    explicit TodoPage(QWidget *parent = nullptr);
};

#endif