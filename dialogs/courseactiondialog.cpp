#include "courseactiondialog.h"
#include "../ui/theme.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

CourseActionDialog::CourseActionDialog(QWidget *parent)
    :QDialog(parent)
{
    setFixedSize(320,220);
    setWindowTitle("课程操作");

    setStyleSheet(QString(R"(
        QDialog{
            background:#FAFAFA;
            border-radius:16px;
        }

        QLabel{
            font-size:18px;
            font-weight:bold;
            color:#333;
        }

        QPushButton{
            background:%1;
            color:white;
            border:none;
            border-radius:10px;
            padding:10px;
            font-size:14px;
        }

        QPushButton:hover{
            background:%2;
        }
    )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));

    QVBoxLayout *layout=new QVBoxLayout(this);

    QLabel *title=new QLabel("请选择操作");
    title->setAlignment(Qt::AlignCenter);

    QPushButton *editBtn=new QPushButton("编辑课程");
    QPushButton *deleteBtn=new QPushButton("删除课程");
    QPushButton *ddlBtn=new QPushButton("添加DDL");
    QPushButton *cancelBtn=new QPushButton("取消");

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(editBtn);
    layout->addWidget(deleteBtn);
    layout->addWidget(ddlBtn);
    layout->addWidget(cancelBtn);

    connect(editBtn,&QPushButton::clicked,[this](){
        editChoice=true;
        accept();
    });

    connect(deleteBtn,&QPushButton::clicked,[this](){
        deleteChoice=true;
        accept();
    });

    connect(ddlBtn,&QPushButton::clicked,[this](){
        ddlChoice=true;
        accept();
    });
}

bool CourseActionDialog::editSelected() const{
    return editChoice;
}

bool CourseActionDialog::deleteSelected() const{
    return deleteChoice;
}

bool CourseActionDialog::ddlSelected() const{
    return ddlChoice;
}