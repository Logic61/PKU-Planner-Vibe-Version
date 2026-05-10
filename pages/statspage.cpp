#include "statspage.h"
#include "../models/datamanager.h"
#include "../models/task.h"
#include "../models/course.h"
#include "../ui/theme.h"
#include "../components/emptystatewidget.h"
#include "../widgets/dialogs/weeklysummarydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QProgressBar>
#include <QGridLayout>
#include <QDate>
#include <QList>
#include <QMap>
#include <QVector>
#include <QScrollArea>
#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QEvent>
#include <algorithm>
#include <QDebug>

namespace {
QFrame* makeCard(const QString &icon, const QString &title, const QString &value) {
    QFrame *card = new QFrame;
    card->setMinimumHeight(120);
    card->setStyleSheet("QFrame{background:white;border-radius:20px;}");
    
    QVBoxLayout *l = new QVBoxLayout(card);
    l->setContentsMargins(10, 10, 10, 10);
    l->setSpacing(4);
    l->addStretch();

    QLabel *i = new QLabel(icon, card);
    i->setStyleSheet("font-size:24px; color:#222; background:transparent;");
    i->setAlignment(Qt::AlignCenter);
    l->addWidget(i);

    QLabel *v = new QLabel(value, card);
    v->setObjectName("value");
    v->setStyleSheet("font-size:32px; font-weight:700; color:#222; background:transparent;");
    v->setAlignment(Qt::AlignCenter);
    l->addWidget(v);

    QLabel *t = new QLabel(title, card);
    t->setObjectName("title");
    t->setStyleSheet("font-size:14px; color:#888; background:transparent;");
    t->setAlignment(Qt::AlignCenter);
    l->addWidget(t);

    l->addStretch();
    return card;
}

QString dayText(int day) {
    switch (day) {
    case 1: return "周一";
    case 2: return "周二";
    case 3: return "周三";
    case 4: return "周四";
    case 5: return "周五";
    case 6: return "周六";
    case 7: return "周日";
    default: return "";
    }
}
}

StatsPage::StatsPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background:#F7F6F4;");

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background:transparent;");
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget *contentWidget = new QWidget;
    contentWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    contentWidget->setMinimumWidth(400);

    QVBoxLayout *root = new QVBoxLayout(contentWidget);
    root->setContentsMargins(16,16,16,16);
    root->setSpacing(12);

    QLabel *title = new QLabel("学习分析");
    title->setStyleSheet("font-size:18px;font-weight:700;color:#222;");
    root->addWidget(title);

    QHBoxLayout *titleRow = new QHBoxLayout;
    titleRow->setSpacing(8);
    QLabel *subtitle = new QLabel("本学期效率概览");
    subtitle->setStyleSheet("color:#888;font-size:12px;margin-bottom:8px;");
    titleRow->addWidget(subtitle);
    titleRow->addStretch();

    QPushButton *summaryBtn = new QPushButton("📊 周总结");
    summaryBtn->setCursor(Qt::PointingHandCursor);
    summaryBtn->setFixedHeight(28);
    summaryBtn->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            border: none;
            border-radius: 14px;
            padding: 0 12px;
            color: white;
            font-size: 12px;
            font-weight: 500;
        }
        QPushButton:hover {
            background: %2;
        }
    )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));
    connect(summaryBtn, &QPushButton::clicked, this, &StatsPage::showWeeklySummary);
    titleRow->addWidget(summaryBtn);

    root->addLayout(titleRow);

    // 四个统计卡片
    QHBoxLayout *cards = new QHBoxLayout;
    cards->setSpacing(12);
    cardTotal = makeCard("📋", "任务总数", "0");
    cardTotal->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    cardDone = makeCard("✅", "已完成率", "0%");
    cardDone->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    cardOnTime = makeCard("⏰", "按时完成率", "0%");
    cardOnTime->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    cardAvg = makeCard("📅", "平均提前", "0 天");
    cardAvg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    cards->addWidget(cardTotal,1);
    cards->addWidget(cardDone,1);
    cards->addWidget(cardOnTime,1);
    cards->addWidget(cardAvg,1);
    root->addLayout(cards);

    // 热力日历（本月）
    QFrame *heatCard = new QFrame;
    heatCard->setStyleSheet("QFrame{background:white;border-radius:20px;padding:20px;}");
    QVBoxLayout *heatLayout = new QVBoxLayout(heatCard);
    heatLayout->setSpacing(16);
    heatGrid = new QGridLayout;
    heatGrid->setSpacing(8);
    heatLayout->addLayout(heatGrid);
    root->addWidget(heatCard);

    // 趋势图（最近7天）
    QFrame *trendCard = new QFrame;
    trendCard->setStyleSheet("QFrame{background:white;border-radius:20px;padding:16px;}");
    QVBoxLayout *trendLayout = new QVBoxLayout(trendCard);
    QLabel *trendTitle = new QLabel("最近7天任务完成趋势");
    trendTitle->setStyleSheet("font-weight:700;color:#222;margin-bottom:8px;");
    trendLayout->addWidget(trendTitle);
    trendContainer = new QHBoxLayout;
    trendContainer->setSpacing(8);
    trendLayout->addLayout(trendContainer);
    root->addWidget(trendCard);

    // 课程任务数量排行
    QFrame *suggestCard = new QFrame;
    suggestCard->setStyleSheet("QFrame{background:white;border-radius:20px;padding:16px;}");
    QVBoxLayout *suggestLayout = new QVBoxLayout(suggestCard);
    QLabel *suggestTitle = new QLabel("课程任务数量排行");
    suggestTitle->setStyleSheet("font-weight:700;color:#222;margin-bottom:8px;");
    suggestLayout->addWidget(suggestTitle);
    suggestContainer = new QVBoxLayout;
    suggestContainer->setSpacing(8);
    suggestLayout->addLayout(suggestContainer);
    root->addWidget(suggestCard);

    emptyStateWidget = new EmptyStateWidget;
    emptyStateWidget->setContent("📊", "暂无统计数据", "完成任务后这里将生成学习分析");
    emptyStateWidget->hide();
    root->addWidget(emptyStateWidget, 1);

    refresh();

    connect(&DataManager::instance(), &DataManager::tasksChanged, this, &StatsPage::refresh);
    connect(&DataManager::instance(), &DataManager::coursesChanged, this, &StatsPage::refresh);

    scrollArea->setWidget(contentWidget);

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);
}

void StatsPage::refresh()
{
    const QList<Task>& tasks = DataManager::instance().tasks();
    const QList<Course>& courses = DataManager::instance().courses();

    if (tasks.isEmpty()) {
        updateEmptyState();
        return;
    }

    if (emptyStateWidget) {
        emptyStateWidget->hide();
    }

    updateSummary(tasks);
    
    updateHeatmap(tasks);
    updateTrend(tasks);
    updateSuggestions(tasks, courses);
}

void StatsPage::updateEmptyState()
{
    if (emptyStateWidget) {
        emptyStateWidget->show();
    }

    QLabel* totalVal = cardTotal->findChild<QLabel*>("value");
    QLabel* doneVal = cardDone->findChild<QLabel*>("value");
    QLabel* onTimeVal = cardOnTime->findChild<QLabel*>("value");
    QLabel* avgVal = cardAvg->findChild<QLabel*>("value");
    if (totalVal) totalVal->setText("0");
    if (doneVal) doneVal->setText("0%");
    if (onTimeVal) onTimeVal->setText("0%");
    if (avgVal) avgVal->setText("0 天");

    clearLayout(heatGrid);
    clearLayout(trendContainer);
    clearLayout(suggestContainer);

    QLabel* empty = new QLabel("暂无任务");
    empty->setStyleSheet("color:#999;font-size:12px;padding:16px;");
    suggestContainer->addWidget(empty);
}

void StatsPage::clearLayout(QLayout* layout)
{
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void StatsPage::updateSummary(const QList<Task>& tasks)
{
    int total = tasks.size();
    int completedCount = 0;
    int onTimeCount = 0;
    double totalDays = 0;
    int validCount = 0;

    for (const Task& t : tasks) {
        if (t.completed) {
            completedCount++;
            if (t.completedAt.isValid()) {
                if (t.completedAt <= t.deadline) {
                    onTimeCount++;
                }
                qint64 secs = t.completedAt.secsTo(t.deadline);
                double days = secs / 86400.0;
                totalDays += days;
                validCount++;
            }
        }
    }

    double completionRate = total > 0 ? completedCount * 100.0 / total : 0;
    double onTimeRate = completedCount > 0 ? onTimeCount * 100.0 / completedCount : 0;
    double avgEarly = validCount > 0 ? totalDays / validCount : 0;

    QLabel* totalVal = cardTotal->findChild<QLabel*>("value");
    QLabel* doneVal = cardDone->findChild<QLabel*>("value");
    QLabel* onTimeVal = cardOnTime->findChild<QLabel*>("value");
    QLabel* avgVal = cardAvg->findChild<QLabel*>("value");

    if (totalVal) totalVal->setText(QString::number(total));
    if (doneVal) doneVal->setText(QString::number(qRound(completionRate)) + "%");
    if (onTimeVal) onTimeVal->setText(QString::number(qRound(onTimeRate)) + "%");
    if (avgVal) avgVal->setText(QString::number(qRound(avgEarly)) + " 天");
}



void StatsPage::updateHeatmap(const QList<Task>& tasks)
{
    clearLayout(heatGrid);

    if (!currentMonth.isValid()) {
        currentMonth = QDate::currentDate();
    }
    QDate firstDayOfMonth(currentMonth.year(), currentMonth.month(), 1);
    int daysInMonth = firstDayOfMonth.daysInMonth();
    QDate today = QDate::currentDate();

    QMap<QDate, QList<const Task*>> completedDDLs;
    QMap<QDate, QList<const Task*>> pendingDDLs;
    for (const Task& t : tasks) {
        QDate d = t.deadline.date();
        if (d.year() == currentMonth.year() && d.month() == currentMonth.month()) {
            if (t.completed && t.completedAt.isValid()) {
                QDate completedDate = t.completedAt.date();
                completedDDLs[completedDate].append(&t);
            } else if (!t.completed) {
                pendingDDLs[d].append(&t);
            }
        }
    }

    QWidget* headerRow = new QWidget;
    QHBoxLayout* headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* titleLabel = new QLabel(QString("%1月 DDL 热力图").arg(currentMonth.month()));
    titleLabel->setStyleSheet(QString("font-size:18px;font-weight:700;color:%1;").arg(Theme::PRIMARY));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    QWidget* monthNav = new QWidget;
    QHBoxLayout* navLayout = new QHBoxLayout(monthNav);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(8);

    QPushButton* prevBtn = new QPushButton("◀");
    prevBtn->setFixedSize(28, 28);
    prevBtn->setStyleSheet("QPushButton{background:#F5F5F5;border-radius:14px;border:none;}QPushButton:hover{background:#E0E0E0;}");
    connect(prevBtn, &QPushButton::clicked, [this]() { changeMonth(-1); });

    QLabel* currentLabel = new QLabel(QString::number(currentMonth.month()));
    currentLabel->setStyleSheet("font-size:14px;font-weight:600;color:#333;min-width:24px;");
    currentLabel->setAlignment(Qt::AlignCenter);

    QPushButton* nextBtn = new QPushButton("▶");
    nextBtn->setFixedSize(28, 28);
    nextBtn->setStyleSheet("QPushButton{background:#F5F5F5;border-radius:14px;border:none;}QPushButton:hover{background:#E0E0E0;}");
    connect(nextBtn, &QPushButton::clicked, [this]() { changeMonth(1); });

    navLayout->addWidget(prevBtn);
    navLayout->addWidget(currentLabel);
    navLayout->addWidget(nextBtn);
    headerLayout->addWidget(monthNav);
    heatGrid->addWidget(headerRow, 0, 0, 1, 7);

    int totalDDLs = completedDDLs.size() + pendingDDLs.size();
    int doneCount = completedDDLs.size();
    int pendingCount = pendingDDLs.size();
    int overdueCount = 0;
    QDate busiestDay;
    int maxTasks = 0;
    for (const QDate& d : completedDDLs.keys()) {
        int cnt = completedDDLs.value(d).size();
        if (cnt > maxTasks) { maxTasks = cnt; busiestDay = d; }
    }
    for (const QDate& d : pendingDDLs.keys()) {
        int cnt = pendingDDLs.value(d).size();
        if (cnt > maxTasks) { maxTasks = cnt; busiestDay = d; }
    }

    QWidget* statsRow = new QWidget;
    QHBoxLayout* statsLayout = new QHBoxLayout(statsRow);
    statsLayout->setContentsMargins(0, 8, 0, 8);
    statsLayout->setSpacing(12);

    auto makeStatCard = [&](const QString& num, const QString& label, const QString& color) -> QFrame* {
        QFrame* card = new QFrame;
        card->setStyleSheet(QString("background:%1;border-radius:8px;padding:8px 12px;").arg(color));
        QVBoxLayout* l = new QVBoxLayout(card);
        l->setContentsMargins(0, 0, 0, 0);
        QLabel* n = new QLabel(num);
        n->setStyleSheet("font-size:16px;font-weight:700;color:#333;");
        QLabel* t = new QLabel(label);
        t->setStyleSheet("font-size:10px;color:#666;");
        l->addWidget(n);
        l->addWidget(t);
        return card;
    };

    statsLayout->addWidget(makeStatCard(QString::number(totalDDLs), "本月DDL", "#F5F5F5"));
    statsLayout->addWidget(makeStatCard(QString::number(doneCount), "已完成", "#E8F5E9"));
    statsLayout->addWidget(makeStatCard(QString::number(pendingCount), "待完成", pendingCount > 0 ? "#FFEBEE" : "#F5F5F5"));
    statsLayout->addWidget(makeStatCard(busiestDay.isValid() ? busiestDay.toString("M/d") : "-", "最忙日", busiestDay.isValid() ? "#FFF3E0" : "#F5F5F5"));
    statsLayout->addStretch();

    heatGrid->addWidget(statsRow, 1, 0, 1, 7);

    QStringList weekDays = {"一", "二", "三", "四", "五", "六", "日"};
    for (int i = 0; i < 7; ++i) {
        QLabel* day = new QLabel(weekDays[i]);
        day->setStyleSheet("color:#888;font-size:11px;font-weight:600;");
        day->setAlignment(Qt::AlignCenter);
        heatGrid->addWidget(day, 2, i);
    }

    int startWeekday = firstDayOfMonth.dayOfWeek();
    int row = 3;
    int col = startWeekday - 1;

    for (int day = 1; day <= daysInMonth; ++day) {
        QDate d(currentMonth.year(), currentMonth.month(), day);
        QList<const Task*> dayCompleted = completedDDLs.value(d);
        QList<const Task*> dayPending = pendingDDLs.value(d);
        int completedCount = dayCompleted.size();
        int pendingCount = dayPending.size();

        QFrame* cell = new QFrame;
        cell->setFixedSize(36, 36);
        cell->setCursor(Qt::PointingHandCursor);
        cell->setStyleSheet("border-radius:8px;");

        QVBoxLayout* cellLayout = new QVBoxLayout(cell);
        cellLayout->setContentsMargins(0,0,0,0);
        cellLayout->setSpacing(0);
        cellLayout->setAlignment(Qt::AlignCenter);

        QString bgColor = "#FAFAFA";
        QString textColor = "#999";
        if (completedCount > 0 && pendingCount == 0) {
            bgColor = completedCount == 1 ? "#C8E6C9" : (completedCount <= 3 ? "#A5D6A7" : "#81C784");
            textColor = "#2E7D32";
        } else if (pendingCount > 0) {
            if (pendingCount == 1) { bgColor = "#FFCDD2"; textColor = "#C62828"; }
            else if (pendingCount <= 3) { bgColor = "#EF9A9A"; textColor = "#C62828"; }
            else if (pendingCount <= 5) { bgColor = "#E57373"; textColor = "#B71C1C"; }
            else { bgColor = "#F44336"; textColor = "#B71C1C"; }
        } else if (d == today) {
            bgColor = "#E3F2FD";
            textColor = Theme::PRIMARY;
        }

        QLabel* dayLabel = new QLabel(QString::number(day));
        dayLabel->setAlignment(Qt::AlignCenter);
        dayLabel->setStyleSheet(QString("font-size:13px;font-weight:600;color:%1;").arg(textColor));
        cell->setStyleSheet(QString("background:%1;border-radius:8px;").arg(bgColor));
        cell->layout()->addWidget(dayLabel);

        QString tooltipText;
        if (completedCount == 0 && pendingCount == 0) {
            tooltipText = QString("📅 %1\n未完成DDL").arg(d.toString("M月d日"));
        } else {
            tooltipText = QString("📅 %1\n\n").arg(d.toString("M月d日"));
            if (completedCount > 0) {
                tooltipText += QString("✓ 已完成 (%1):\n").arg(completedCount);
                for (const Task* t : dayCompleted) {
                    tooltipText += "  " + t->title + "\n";
                }
            }
            if (pendingCount > 0) {
                tooltipText += QString("\n⚠ 待完成 (%1):\n").arg(pendingCount);
                for (const Task* t : dayPending) {
                    tooltipText += "  " + t->title + "\n";
                }
            }
        }

        QLabel* tip = new QLabel(tooltipText);
        tip->setWordWrap(true);
        tip->setStyleSheet(R"(
            background:#FFFFFF;
            color:#333333;
            padding:16px;
            border-radius:12px;
            font-size:13px;
        )");
        tip->setMinimumWidth(220);
        tip->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
        tip->setAttribute(Qt::WA_TranslucentBackground);

        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
        shadow->setBlurRadius(20);
        shadow->setColor(QColor(0, 0, 0, 40));
        shadow->setOffset(0, 4);
        tip->setGraphicsEffect(shadow);

        tip->hide();

        cell->installEventFilter(this);
        cell->setProperty("tipWidget", QVariant::fromValue<QObject*>(tip));

        heatGrid->addWidget(cell, row, col);

        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }

    QFrame* legend = new QFrame;
    legend->setStyleSheet("background:transparent;padding:12px 0 0 0;");
    QHBoxLayout* legLayout = new QHBoxLayout(legend);
    legLayout->setSpacing(12);

    QLabel* noneLeg = new QLabel("○ 无");
    noneLeg->setFixedSize(24, 24);
    noneLeg->setAlignment(Qt::AlignCenter);
    noneLeg->setStyleSheet("font-size:10px;color:#999;background:#FAFAFA;border-radius:4px;");

    QLabel* lowLeg = new QLabel("1");
    lowLeg->setFixedSize(24, 24);
    lowLeg->setAlignment(Qt::AlignCenter);
    lowLeg->setStyleSheet("font-size:10px;color:#2E7D32;background:#C8E6C9;border-radius:4px;");

    QLabel* midLeg = new QLabel("2-3");
    midLeg->setFixedSize(24, 24);
    midLeg->setAlignment(Qt::AlignCenter);
    midLeg->setStyleSheet("font-size:9px;color:#2E7D32;background:#A5D6A7;border-radius:4px;");

    QLabel* highLeg = new QLabel("4-5");
    highLeg->setFixedSize(24, 24);
    highLeg->setAlignment(Qt::AlignCenter);
    highLeg->setStyleSheet("font-size:9px;color:#B71C1C;background:#EF9A9A;border-radius:4px;");

    QLabel* busyLeg = new QLabel("6+");
    busyLeg->setFixedSize(24, 24);
    busyLeg->setAlignment(Qt::AlignCenter);
    busyLeg->setStyleSheet("font-size:9px;color:white;background:#F44336;border-radius:4px;");

    legLayout->addStretch();
    legLayout->addWidget(noneLeg);
    legLayout->addWidget(lowLeg);
    legLayout->addWidget(midLeg);
    legLayout->addWidget(highLeg);
    legLayout->addWidget(busyLeg);

heatGrid->addWidget(legend, row + 1, 0, 1, 7);
}

void StatsPage::updateTrend(const QList<Task>& tasks)
{
    clearLayout(trendContainer);

    QDate today = QDate::currentDate();
    QMap<QDate, int> completedPerDay;

    for (int i = 6; i >= 0; i--) {
        QDate d = today.addDays(-i);
        completedPerDay[d] = 0;
    }

    for (const Task& t : tasks) {
        if (t.completed && t.completedAt.isValid()) {
            QDate d = t.completedAt.date();
            if (completedPerDay.contains(d)) {
                completedPerDay[d]++;
            }
        }
    }

    int maxCount = 1;
    for (auto v : completedPerDay.values()) {
        if (v > maxCount) maxCount = v;
    }

    for (int i = 6; i >= 0; i--) {
        QDate d = today.addDays(-i);
        int count = completedPerDay.value(d, 0);

        QFrame* dayFrame = new QFrame;
        dayFrame->setStyleSheet("background:#FEFEFE;border-radius:8px;padding:8px;");
        QVBoxLayout* dayLayout = new QVBoxLayout(dayFrame);
        dayLayout->setContentsMargins(4,4,4,4);
        dayLayout->setSpacing(6);

        QLabel* countLabel = new QLabel(QString::number(count));
        countLabel->setStyleSheet(QString("font-size:14px;font-weight:700;color:%1;").arg(Theme::PRIMARY));
        countLabel->setAlignment(Qt::AlignCenter);

        QString dayStr;
        if (i == 0) dayStr = "今天";
        else if (i == 1) dayStr = "昨天";
        else dayStr = QString("%1日").arg(d.day());

        QLabel* dayLabel = new QLabel(dayStr);
        dayLabel->setStyleSheet("color:#999;font-size:10px;");
        dayLabel->setAlignment(Qt::AlignCenter);

        QProgressBar* bar = new QProgressBar;
        bar->setOrientation(Qt::Vertical);
        bar->setRange(0, maxCount);
        bar->setValue(count);
        bar->setFixedWidth(12);
        bar->setMinimumHeight(60);
        bar->setStyleSheet(QString(R"(
            QProgressBar {
                border: none;
                background: %1;
                border-radius: 6px;
            }
            QProgressBar::chunk {
                background: %2;
                border-radius: 6px;
            }
        )").arg(Theme::BORDER).arg(Theme::PRIMARY));
        bar->setTextVisible(false);

        dayLayout->addWidget(countLabel, 0, Qt::AlignCenter);
        dayLayout->addWidget(bar, 1, Qt::AlignCenter);
        dayLayout->addWidget(dayLabel, 0, Qt::AlignCenter);

        trendContainer->addWidget(dayFrame, 1);
    }
}

void StatsPage::updateSuggestions(const QList<Task>& tasks, const QList<Course>& courses)
{
    clearLayout(suggestContainer);

    QMap<QString, int> totalCounter;
    QMap<QString, int> completedCounter;
    for (const Task& t : tasks) {
        totalCounter[t.course]++;
        if (t.completed) {
            completedCounter[t.course]++;
        }
    }

    QVector<QPair<QString, int>> sorted;
    for (auto it = totalCounter.begin(); it != totalCounter.end(); ++it) {
        sorted.append(qMakePair(it.key(), it.value()));
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    for (const auto& pair : sorted) {
        int total = pair.second;
        int completed = completedCounter.value(pair.first, 0);
        int pending = total - completed;

        QFrame* row = new QFrame;
        row->setStyleSheet("background:#FEFEFE;border-radius:8px;padding:8px;");

        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(8,4,8,4);

        QLabel* name = new QLabel(pair.first);
        name->setStyleSheet("color:#333;font-size:12px;font-weight:500;min-width:80px;");
        rowLayout->addWidget(name);

        QFrame* barBg = new QFrame;
        barBg->setFixedHeight(12);
        barBg->setStyleSheet("background:#E0E0E0;border-radius:4px;");
        QHBoxLayout* barLayout = new QHBoxLayout(barBg);
        barLayout->setContentsMargins(0, 0, 0, 0);
        barLayout->setSpacing(0);

        if (completed > 0) {
            QFrame* doneBar = new QFrame;
            doneBar->setMinimumHeight(12);
            doneBar->setMaximumHeight(12);
            doneBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            doneBar->setStyleSheet("background:#2196F3;border-top-left-radius:3px;border-bottom-left-radius:3px;");
            barLayout->addWidget(doneBar, completed);
        }
        if (pending > 0) {
            QFrame* pendingBar = new QFrame;
            pendingBar->setMinimumHeight(12);
            pendingBar->setMaximumHeight(12);
            pendingBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
            if (completed > 0) {
                pendingBar->setStyleSheet("background:#F44336;border-top-right-radius:3px;border-bottom-right-radius:3px;");
            } else {
                pendingBar->setStyleSheet("background:#F44336;border-radius:3px;");
            }
            barLayout->addWidget(pendingBar, pending);
        }

        rowLayout->addWidget(barBg, 1);

        QLabel* count = new QLabel(QString::number(total));
        count->setStyleSheet("color:#666;font-size:12px;min-width:24px;text-align:right;");
        rowLayout->addWidget(count);

        suggestContainer->addWidget(row);
    }

    if (sorted.isEmpty()) {
        QLabel* empty = new QLabel("暂无任务");
        empty->setStyleSheet("color:#999;font-size:12px;padding:16px;");
        suggestContainer->addWidget(empty);
    }
}

void StatsPage::refreshData()
{
    refresh();
}

void StatsPage::changeMonth(int delta)
{
    currentMonth = currentMonth.addMonths(delta);
    const QList<Task>& tasks = DataManager::instance().tasks();
    updateHeatmap(tasks);
}

void StatsPage::showWeeklySummary()
{
    WeeklySummaryDialog dlg(this);
    dlg.exec();
}

bool StatsPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        QObject *tipObj = obj->property("tipWidget").value<QObject*>();
        if (tipObj) {
            QLabel *tip = qobject_cast<QLabel*>(tipObj);
            if (tip) {
                QWidget *cell = qobject_cast<QWidget*>(obj);
                if (cell) {
                    tip->adjustSize();
                    QPoint pos = cell->mapToGlobal(QPoint(0, cell->height() + 8));
                    tip->move(pos);
                    tip->raise();
                    tip->show();
                }
            }
        }
    } else if (event->type() == QEvent::Leave) {
        QObject *tipObj = obj->property("tipWidget").value<QObject*>();
        if (tipObj) {
            QLabel *tip = qobject_cast<QLabel*>(tipObj);
            if (tip) {
                tip->hide();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}