#include "sidebarwidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(R"(
        QWidget {
            background: #7E1D2D;
            color: white;
            font-family: 'Microsoft YaHei', 'Segoe UI', Arial;
            font-weight: 500;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 18, 16, 16);
    layout->setSpacing(10);

    QLabel *title = new QLabel("Course Helper");
    title->setStyleSheet("font-size:18px; font-weight:700; padding: 6px 4px; color:#FFF;");
    layout->addWidget(title);

    QLabel *subtitle = new QLabel("管理课程与 DDL");
    subtitle->setStyleSheet("color: rgba(255,255,255,0.85); font-size: 12px; padding-left: 4px;");
    layout->addWidget(subtitle);

    QPushButton *btnDashboard = new QPushButton("课程表");
    QPushButton *btnTodo = new QPushButton("待办");

    const QString buttonStyle = R"(
        QPushButton {
            /* 默认使用半透明白背景，避免在深色侧栏上显示为纯白 */
            background: rgba(94, 64, 64, 0.12);
            color: white;
            border: 1px solid rgba(255,255,255,0.14);
            border-radius: 14px;
            padding: 12px 14px;
            text-align: left;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: rgba(190, 116, 116, 0.2);
            color: white;
        }
        QPushButton:pressed {
            background: rgba(255,255,255,0.28);
        }
        /* 更明显的选中态，便于与背景区分 */
        QPushButton:checked {
            background: #FFD7DA;
            color: #7E1D2D;
            border: 1px solid #FFD7DA;
        }
    )";
    btnDashboard->setStyleSheet(buttonStyle);
    btnTodo->setStyleSheet(buttonStyle);
    btnDashboard->setCheckable(true);
    btnTodo->setCheckable(true);
    btnDashboard->setChecked(true);
    btnDashboard->setCursor(Qt::PointingHandCursor);
    btnTodo->setCursor(Qt::PointingHandCursor);

    layout->addWidget(btnDashboard);
    layout->addWidget(btnTodo);
    layout->addStretch();

    connect(btnDashboard, &QPushButton::clicked, [=](){
        btnDashboard->setChecked(true);
        btnTodo->setChecked(false);
        emit pageChanged(0);
    });

    connect(btnTodo, &QPushButton::clicked, [=](){
        btnTodo->setChecked(true);
        btnDashboard->setChecked(false);
        emit pageChanged(1);
    });
}