#include "sidebarwidget.h"
#include "theme.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPixmap>
#include <QTimer>
#include <QMouseEvent>
#include <QDebug>
#include "../widgets/mascot/mascotwidget.h"
#include "../services/mascotstateservice.h"

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(QString(R"(
        QWidget {
            background: %1;
            color: white;
            font-family: 'Microsoft YaHei', 'Segoe UI', Arial;
            font-weight: 500;
        }
    )").arg(Theme::PRIMARY));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    QWidget *headerWidget = new QWidget;
    headerWidget->setFixedHeight(100);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 20, 20, 20);
    headerLayout->setSpacing(0);

    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->setSpacing(2);
    textLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *title = new QLabel("PKU Planner+");
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #FFF;");

    QLabel *subtitle = new QLabel("北京大学课程管理系统");
    subtitle->setStyleSheet("font-size: 12px; color: rgba(255,255,255,0.6);");

    textLayout->addWidget(title);
    textLayout->addWidget(subtitle);
    textLayout->addStretch();

    headerLayout->addLayout(textLayout);

    layout->addWidget(headerWidget);

    btnDashboard = new QPushButton("课程表");
    btnTodo = new QPushButton("待办");
    btnStats = new QPushButton("统计");
    btnSettings = new QPushButton("设置");

    QString buttonStyle = QString(R"(
        QPushButton {
            background: #F8EDEE;
            border: none;
            border-radius: 18px;
            padding: 18px;
            min-height: 70px;
            text-align: left;
            font-size: 16px;
            font-weight: 600;
            color: #8C1D2C;
        }
        QPushButton:hover {
            background: #F3DCDC;
        }
        QPushButton:pressed {
            background: #EED0D2;
        }
        QPushButton:checked {
            background: #8C1D2C;
            color: white;
        }
    )");
    btnDashboard->setStyleSheet(buttonStyle);
    btnTodo->setStyleSheet(buttonStyle);
    btnStats->setStyleSheet(buttonStyle);
    btnSettings->setStyleSheet(buttonStyle);
    btnDashboard->setCheckable(true);
    btnTodo->setCheckable(true);
    btnStats->setCheckable(true);
    btnSettings->setCheckable(true);
    btnDashboard->setChecked(true);
    btnDashboard->setCursor(Qt::PointingHandCursor);
    btnTodo->setCursor(Qt::PointingHandCursor);
    btnStats->setCursor(Qt::PointingHandCursor);
    btnSettings->setCursor(Qt::PointingHandCursor);

    layout->addWidget(btnDashboard);
    layout->addWidget(btnTodo);
    layout->addWidget(btnStats);
    layout->addWidget(btnSettings);
    layout->addStretch();

    QFrame *mascotContainer = new QFrame(this);
    mascotContainer->setFixedSize(200, 160);
    mascotContainer->setStyleSheet("background:transparent;");
    QVBoxLayout *mascotLayout = new QVBoxLayout(mascotContainer);
    mascotLayout->setContentsMargins(0, 10, 0, 10);

    mascotLabel = new QLabel(mascotContainer);
    mascotLabel->setFixedSize(200, 160);
    mascotLabel->setScaledContents(true);
    mascotLabel->setCursor(Qt::PointingHandCursor);
    mascotLayout->addWidget(mascotLabel);

    mascotLabel->installEventFilter(this);
    mascotLabel->setMouseTracking(true);

    auto loadMascotImage = [&](MascotState state) {
        QString imagePath = QString("C:/Users/32372/Desktop/Course-Helper/image/%1.png").arg((int)state);
        qDebug() << "[Sidebar] Loading mascot image:" << imagePath;
        QPixmap pix(imagePath);
        if (!pix.isNull()) {
            mascotLabel->setPixmap(pix);
            qDebug() << "[Sidebar] Mascot image loaded successfully";
        } else {
            qDebug() << "[Sidebar] Mascot image failed to load";
        }
    };

    loadMascotImage(MascotStateService::instance().currentState());

    connect(&MascotStateService::instance(), &MascotStateService::stateChanged, this, [loadMascotImage](MascotState state) {
        loadMascotImage(state);
    });

layout->addWidget(mascotContainer, 0, Qt::AlignCenter);

    connect(btnDashboard, &QPushButton::clicked, [=](){
        btnDashboard->setChecked(true);
        btnTodo->setChecked(false);
        btnStats->setChecked(false);
        btnSettings->setChecked(false);
        emit pageChanged(0);
    });

    connect(btnTodo, &QPushButton::clicked, [=](){
        btnTodo->setChecked(true);
        btnDashboard->setChecked(false);
        btnStats->setChecked(false);
        btnSettings->setChecked(false);
        emit pageChanged(1);
    });

    connect(btnStats, &QPushButton::clicked, [=](){
        btnStats->setChecked(true);
        btnDashboard->setChecked(false);
        btnTodo->setChecked(false);
        btnSettings->setChecked(false);
        emit pageChanged(2);
    });

    connect(btnSettings, &QPushButton::clicked, [=](){
        btnSettings->setChecked(true);
        btnDashboard->setChecked(false);
        btnTodo->setChecked(false);
        btnStats->setChecked(false);
        emit pageChanged(3);
    });

    connect(this, &SidebarWidget::pageChanged, [=](int page){
        btnDashboard->setChecked(page == 0);
        btnTodo->setChecked(page == 1);
        btnStats->setChecked(page == 2);
        btnSettings->setChecked(page == 3);
    });
}

bool SidebarWidget::eventFilter(QObject *obj, QEvent *event)
{
    qDebug() << "[Sidebar] eventFilter called, type:" << event->type() << "obj:" << obj;
    if (obj == mascotLabel && event->type() == QEvent::MouseButtonPress) {
        qDebug() << "[Sidebar] Mouse press on mascot label";
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            qDebug() << "[Sidebar] Emitting mascotClicked";
            emit mascotClicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void SidebarWidget::mousePressEvent(QMouseEvent *event)
{
    if (mascotLabel && mascotLabel->geometry().contains(event->pos())) {
        emit mascotClicked();
        return;
    }
    QWidget::mousePressEvent(event);
}

void SidebarWidget::setActivePage(int index)
{
    btnDashboard->setChecked(index == 0);
    btnTodo->setChecked(index == 1);
    btnStats->setChecked(index == 2);
    btnSettings->setChecked(index == 3);
}