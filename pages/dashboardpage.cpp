#include "dashboardpage.h"
#include "../components/coursecellwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>

DashboardPage::DashboardPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background:#F7F3EF;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16,16,16,16);
    mainLayout->setSpacing(12);

    // ===== 1. 顶部统计卡片 =====
    QHBoxLayout *statsLayout = new QHBoxLayout;

    for(int i=0;i<4;i++)
    {
        QFrame *card = new QFrame;
        card->setFixedHeight(80);

        card->setStyleSheet(R"(
            QFrame {
                background:white;
                border-radius:12px;
            }
        )");

        QVBoxLayout *cl = new QVBoxLayout(card);

        QLabel *num = new QLabel("3");
        num->setStyleSheet("font-size:18px; font-weight:bold;");

        QLabel *text = new QLabel("今日课程");

        cl->addWidget(num);
        cl->addWidget(text);

        statsLayout->addWidget(card);
    }

    mainLayout->addLayout(statsLayout);

    // ===== 2. 主体区域 =====
    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(12);

    // 左：课程表
    QFrame *courseCard = new QFrame;
    courseCard->setStyleSheet("background:white; border-radius:16px;");
    QVBoxLayout *courseLayout = new QVBoxLayout(courseCard);

    QLabel *courseTitle = new QLabel("课程表");
    courseTitle->setStyleSheet("font-weight:bold; font-size:16px;");
    courseLayout->addWidget(courseTitle);

    QWidget *gridContainer = new QWidget;
    grid = new QGridLayout(gridContainer);
    grid->setSpacing(6);

    courseLayout->addWidget(gridContainer);

    contentLayout->addWidget(courseCard, 7); // ⭐ 70%

    // 右：侧栏
    QVBoxLayout *rightLayout = new QVBoxLayout;

    // 今日课程卡
    QFrame *todayCard = createCard("今日课程");
    rightLayout->addWidget(todayCard);

    // DDL卡
    QFrame *ddlCard = createCard("DDL提醒");
    rightLayout->addWidget(ddlCard);

    rightLayout->addStretch();

    contentLayout->addLayout(rightLayout, 3); // ⭐ 30%

    mainLayout->addLayout(contentLayout);

    initGrid();
}

QFrame* DashboardPage::createCard(QString title)
{
    QFrame *card = new QFrame;
    card->setMinimumHeight(120);

    card->setStyleSheet(R"(
        QFrame {
            background:white;
            border-radius:16px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(card);

    QLabel *t = new QLabel(title);
    t->setStyleSheet("font-weight:bold;");

    layout->addWidget(t);

    // 示例内容
    QLabel *dummy = new QLabel("暂无内容");
    dummy->setStyleSheet("color:#999;");

    layout->addWidget(dummy);

    return card;
}

void DashboardPage::initGrid()
{
    QStringList days = {"一","二","三","四","五","六","日"};

    // 星期
    for(int col=0; col<7; col++)
    {
        QLabel *label = new QLabel("周" + days[col]);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-weight:bold; color:#666;");
        grid->addWidget(label, 0, col+1);
    }

    // 时间（减少视觉压力）
    for(int row=0; row<10; row++)
    {
        QLabel *time = new QLabel(QString("%1").arg(row+1));
        time->setAlignment(Qt::AlignCenter);
        time->setStyleSheet("color:#aaa; font-size:11px;");
        grid->addWidget(time, row+1, 0);
    }

    // 单元格（更小更紧凑）
    for(int row=0; row<10; row++)
    {
        for(int col=0; col<7; col++)
        {
            CourseCellWidget *cell = new CourseCellWidget;
            cell->setFixedHeight(50);   // ⭐ 关键：压缩高度
            grid->addWidget(cell, row+1, col+1);
        }
    }

    // 示例课程
    CourseCellWidget *c = new CourseCellWidget;
    c->setCourse("数据结构", "理教208");
    c->setFixedHeight(100); // ⭐ 跨节效果

    grid->addWidget(c, 1, 1, 2, 1); // 跨2行
}