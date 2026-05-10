#include "settingspage.h"
#include "../ui/theme.h"
#include "../models/datamanager.h"
#include "../services/configservice.h"
#include "../dialogs/confirmdialog.h"
#include "../components/toastwidget.h"
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDateTime>
#include <QProgressBar>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QScrollArea>
#include <QDateEdit>
#include <QDir>
#include <QCoreApplication>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(QString("background:%1;").arg(Theme::BACKGROUND));

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background:transparent;");

    QWidget *container = new QWidget;
    container->setStyleSheet("background:transparent;");

    QVBoxLayout *root = new QVBoxLayout(container);
    root->setContentsMargins(40, 32, 40, 32);
    root->setSpacing(20);

    QHBoxLayout *headerRow = new QHBoxLayout;
    headerRow->setSpacing(16);

    QVBoxLayout *titleCol = new QVBoxLayout;
    titleCol->setSpacing(4);

    QLabel *title = new QLabel("设置", this);
    title->setStyleSheet(QString(
        "font-size:32px;"
        "font-weight:700;"
        "color:%1;"
    ).arg(Theme::TEXT_PRIMARY));
    titleCol->addWidget(title);

    QLabel *subtitle = new QLabel("个性化你的 PKU Planner", this);
    subtitle->setStyleSheet(QString(
        "font-size:14px;"
        "color:%1;"
    ).arg(Theme::TEXT_TERTIARY));
    titleCol->addWidget(subtitle);

    headerRow->addLayout(titleCol);
    headerRow->addStretch();

    QLabel *versionLabel = new QLabel("v1.0", this);
    versionLabel->setStyleSheet(QString(
        "background:%1;"
        "color:%2;"
        "padding:6px 12px;"
        "border-radius:8px;"
        "font-size:12px;"
    ).arg(Theme::PRIMARY_LIGHT).arg(Theme::PRIMARY));
    versionLabel->setAlignment(Qt::AlignCenter);
    headerRow->addWidget(versionLabel);

    root->addLayout(headerRow);
    root->addSpacing(16);

    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(20);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    grid->addWidget(createReminderCard(), 0, 0);
    grid->addWidget(createSemesterCard(), 0, 1);
    grid->addWidget(createDataCard(), 1, 0);
    grid->addWidget(createAboutCard(), 1, 1);

    root->addLayout(grid);
    root->addStretch();

    scrollArea->setWidget(container);

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);

    loadSettings();

    connect(reminderCheck, &QCheckBox::toggled, this, &SettingsPage::saveSettings);
    connect(reminderInterval, &QComboBox::currentIndexChanged, this, &SettingsPage::saveSettings);
}

QFrame* SettingsPage::createReminderCard()
{
    QFrame *card = new QFrame;
    card->setStyleSheet(QString(
        "QFrame{"
        "background:%1;"
        "border-radius:24px;"
        "padding:24px;"
        "}"
    ).arg(Theme::CARD_BG));

    QVBoxLayout *v = new QVBoxLayout(card);
    v->setSpacing(12);

    QHBoxLayout *header = new QHBoxLayout;
    header->setSpacing(12);
    QLabel *icon = new QLabel("🔔");
    icon->setStyleSheet("font-size:22px;");
    header->addWidget(icon);

    QLabel *title = new QLabel("DDL 提醒");
    title->setStyleSheet(QString(
        "font-size:18px;"
        "font-weight:600;"
        "color:%1;"
    ).arg(Theme::TEXT_PRIMARY));
    header->addWidget(title);
    header->addStretch();

    QLabel *statusBadge = new QLabel("已开启");
    statusBadge->setStyleSheet(QString(
        "background:%1;"
        "color:%2;"
        "padding:4px 12px;"
        "border-radius:12px;"
        "font-size:12px;"
        "font-weight:600;"
    ).arg(Theme::PRIMARY_LIGHT).arg(Theme::PRIMARY));
    statusLabel = statusBadge;
    header->addWidget(statusBadge);
    v->addLayout(header);

    QLabel *desc = new QLabel("帮助你避免忘记作业截止时间");
    desc->setStyleSheet(QString(
        "font-size:13px;"
        "color:%1;"
    ).arg(Theme::TEXT_TERTIARY));
    v->addWidget(desc);

    v->addSpacing(16);

    QFrame *toggleBox = new QFrame;
    toggleBox->setStyleSheet(QString(
        "background:%1;"
        "border-radius:16px;"
        "padding:16px;"
    ).arg(Theme::BACKGROUND));
    QHBoxLayout *toggleRow = new QHBoxLayout(toggleBox);
    toggleRow->setSpacing(12);

    QLabel *toggleLabel = new QLabel("启用提醒", this);
    toggleLabel->setStyleSheet(QString(
        "font-size:15px;"
        "font-weight:500;"
        "color:%1;"
    ).arg(Theme::TEXT_PRIMARY));
    toggleRow->addWidget(toggleLabel);
    toggleRow->addStretch();

    reminderCheck = new QCheckBox(this);
    reminderCheck->setFixedSize(56, 28);
    reminderCheck->setStyleSheet(QString(
        "QCheckBox{spacing:0;}"
        "QCheckBox::indicator{"
        "width:56px;"
        "height:28px;"
        "border-radius:14px;"
        "background:%1;"
        "border:none;"
        "image:none;"
        "}"
        "QCheckBox::indicator:unchecked{background:%2;}"
        "QCheckBox::indicator:checked{background:%3;}"
    ).arg(Theme::PRIMARY).arg("#E0E0E0").arg(Theme::PRIMARY));
    toggleRow->addWidget(reminderCheck);
    v->addWidget(toggleBox);

    v->addSpacing(8);

    QFrame *freqBox = new QFrame;
    freqBox->setStyleSheet(QString(
        "background:%1;"
        "border-radius:16px;"
        "padding:16px;"
    ).arg(Theme::BACKGROUND));
    QHBoxLayout *freqRow = new QHBoxLayout(freqBox);
    freqRow->setSpacing(12);

    QLabel *freqLabel = new QLabel("提醒频率", this);
    freqLabel->setStyleSheet(QString(
        "font-size:15px;"
        "font-weight:500;"
        "color:%1;"
    ).arg(Theme::TEXT_PRIMARY));
    freqRow->addWidget(freqLabel);
    freqRow->addStretch();

    reminderInterval = new QComboBox(this);
    reminderInterval->addItems({"15分钟", "30分钟", "1小时", "2小时"});
    reminderInterval->setStyleSheet(QString(
        "QComboBox{"
        "border:none;"
        "border-radius:10px;"
        "padding:10px 16px;"
        "background:%1;"
        "color:%2;"
        "font-size:14px;"
        "font-weight:500;"
        "}"
        "QComboBox:hover{background:%3;}"
        "QComboBox::drop-down{border:none;width:30px;}"
        "QComboBox::down-arrow{image:none;border-left:5px solid transparent;border-right:5px solid transparent;border-top:5px solid %2;margin-right:8px;}"
    ).arg(Theme::CARD_BG).arg(Theme::TEXT_PRIMARY).arg(Theme::PRIMARY_LIGHT));
    reminderInterval->setFixedWidth(140);
    freqRow->addWidget(reminderInterval);
    v->addWidget(freqBox);

    v->addStretch();

    return card;
}

QFrame* SettingsPage::createSemesterCard()
{
    QFrame *card = new QFrame;
    card->setStyleSheet(QString(
        "QFrame{"
        "background:%1;"
        "border-radius:24px;"
        "padding:24px;"
        "}"
    ).arg(Theme::CARD_BG));

    QVBoxLayout *v = new QVBoxLayout(card);
    v->setSpacing(12);

    QHBoxLayout *header = new QHBoxLayout;
    header->setSpacing(12);
    QLabel *icon = new QLabel("📅");
    icon->setStyleSheet("font-size:22px;");
    header->addWidget(icon);

    QLabel *title = new QLabel("当前学期");
    title->setStyleSheet(QString(
        "font-size:18px;"
        "font-weight:600;"
        "color:%1;"
    ).arg(Theme::TEXT_PRIMARY));
    header->addWidget(title);
    header->addStretch();
    v->addLayout(header);

    QDate startDate = ConfigService::instance().getSemesterStart();
    QDate endDate = ConfigService::instance().getSemesterEnd();
    int currentWeek = ConfigService::instance().getCurrentWeek();
    bool isSingle = ConfigService::instance().isSingleWeek();

    int totalWeeks = startDate.daysTo(endDate) / 7;
    int progress = qMin(100, (currentWeek * 100) / qMax(1, totalWeeks));

    semesterNameLabel = new QLabel(QString("%1 %2").arg(startDate.year()).arg(startDate.month() <= 6 ? "Spring" : "Fall"), this);
    semesterNameLabel->setStyleSheet(QString(
        "font-size:28px;"
        "font-weight:700;"
        "color:%1;"
    ).arg(Theme::PRIMARY));
    v->addWidget(semesterNameLabel);

    QHBoxLayout *dateRange = new QHBoxLayout;
    dateRange->setSpacing(8);
    startDateLabel = new QLabel(startDate.toString("MM/dd"), this);
    startDateLabel->setStyleSheet(QString("font-size:14px;color:%1;").arg(Theme::TEXT_SECONDARY));
    dateRange->addWidget(startDateLabel);

    QLabel *arrow = new QLabel("→", this);
    arrow->setStyleSheet(QString("color:%1;font-size:14px;").arg(Theme::TEXT_TERTIARY));
    dateRange->addWidget(arrow);

    endDateLabel = new QLabel(endDate.toString("MM/dd"), this);
    endDateLabel->setStyleSheet(QString("font-size:14px;color:%1;").arg(Theme::TEXT_SECONDARY));
    dateRange->addWidget(endDateLabel);
    dateRange->addStretch();
    v->addLayout(dateRange);

    v->addSpacing(12);

    QFrame *progressBox = new QFrame;
    progressBox->setStyleSheet(QString(
        "background:%1;"
        "border-radius:16px;"
        "padding:16px;"
    ).arg(Theme::BACKGROUND));
    QVBoxLayout *progV = new QVBoxLayout(progressBox);
    progV->setSpacing(8);

    QHBoxLayout *progHeader = new QHBoxLayout;
    QLabel *progLabel = new QLabel("学期进度", this);
    progLabel->setStyleSheet(QString("font-size:13px;color:%1;").arg(Theme::TEXT_SECONDARY));
    progHeader->addWidget(progLabel);
    progHeader->addStretch();

    percentLabel = new QLabel(QString("%1%").arg(progress), this);
    percentLabel->setStyleSheet(QString(
        "font-size:14px;"
        "font-weight:700;"
        "color:%1;"
    ).arg(Theme::PRIMARY));
    progHeader->addWidget(percentLabel);
    progV->addLayout(progHeader);

    progressBar = new QProgressBar;
    progressBar->setValue(progress);
    progressBar->setFixedHeight(10);
    progressBar->setStyleSheet(QString(
        "QProgressBar{"
        "background:%1;"
        "border:none;"
        "border-radius:5px;"
        "}"
        "QProgressBar::chunk{"
        "background:%2;"
        "border-radius:5px;"
        "}"
    ).arg("#E8E8E8").arg(Theme::PRIMARY));
    progV->addWidget(progressBar);

    QHBoxLayout *progStats = new QHBoxLayout;
    progStats->setSpacing(16);

    weeksLeftLabel = new QLabel(QString("%1周剩余").arg(qMax(0, totalWeeks - currentWeek)), this);
    weeksLeftLabel->setStyleSheet(QString("font-size:12px;color:%1;").arg(Theme::TEXT_TERTIARY));
    progStats->addWidget(weeksLeftLabel);

    singleWeekLabel = new QLabel(isSingle ? "单周" : "双周", this);
    singleWeekLabel->setStyleSheet(QString(
        "font-size:12px;"
        "color:%1;"
        "background:%2;"
        "padding:2px 8px;"
        "border-radius:8px;"
    ).arg(Theme::PRIMARY).arg(Theme::PRIMARY_LIGHT));
    progStats->addWidget(singleWeekLabel);

    progStats->addStretch();
    progV->addLayout(progStats);

    v->addWidget(progressBox);

    v->addSpacing(12);

    QPushButton *editBtn = new QPushButton("修改学期日期", this);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet(QString(
        "QPushButton{"
        "background:%1;"
        "color:white;"
        "border:none;"
        "border-radius:14px;"
        "padding:14px 20px;"
        "font-size:14px;"
        "font-weight:600;"
        "}"
        "QPushButton:hover{"
        "background:%2;"
        "}"
    ).arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));
    connect(editBtn, &QPushButton::clicked, this, &SettingsPage::editSemester);
    v->addWidget(editBtn);

    v->addStretch();

    return card;
}

QFrame* SettingsPage::createDataCard()
{
    QFrame *card = new QFrame;
    card->setStyleSheet(QString(
        "QFrame{"
        "background:%1;"
        "border-radius:24px;"
        "padding:24px;"
        "}"
    ).arg(Theme::CARD_BG));

    QVBoxLayout *v = new QVBoxLayout(card);
    v->setSpacing(12);

    QHBoxLayout *header = new QHBoxLayout;
    header->setSpacing(12);
    QLabel *icon = new QLabel("💾");
    icon->setStyleSheet("font-size:22px;");
    header->addWidget(icon);

    QLabel *title = new QLabel("数据管理");
    title->setStyleSheet(QString(
        "font-size:18px;"
        "font-weight:600;"
        "color:%1;"
    ).arg(Theme::TEXT_PRIMARY));
    header->addWidget(title);
    header->addStretch();
    v->addLayout(header);

    QLabel *desc = new QLabel("管理你的课程和任务数据");
    desc->setStyleSheet(QString(
        "font-size:13px;"
        "color:%1;"
    ).arg(Theme::TEXT_TERTIARY));
    v->addWidget(desc);

    v->addSpacing(16);

    QGridLayout *btnGrid = new QGridLayout;
    btnGrid->setSpacing(12);
    btnGrid->setColumnStretch(0, 1);
    btnGrid->setColumnStretch(1, 1);

    exportBtn = new QPushButton("📤 导出报告", this);
    exportBtn->setCursor(Qt::PointingHandCursor);
    exportBtn->setStyleSheet(QString(
        "QPushButton{"
        "background:%1;"
        "color:white;"
        "border:none;"
        "border-radius:14px;"
        "padding:16px 12px;"
        "font-size:14px;"
        "font-weight:500;"
        "}"
        "QPushButton:hover{"
        "background:%2;"
        "}"
    ).arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));
    connect(exportBtn, &QPushButton::clicked, this, &SettingsPage::exportToCSV);
    btnGrid->addWidget(exportBtn, 0, 0);

    QPushButton *openFolderBtn = new QPushButton("📁 打开目录", this);
    openFolderBtn->setCursor(Qt::PointingHandCursor);
    openFolderBtn->setStyleSheet(QString(
        "QPushButton{"
        "background:%1;"
        "color:%2;"
        "border:none;"
        "border-radius:14px;"
        "padding:16px 12px;"
        "font-size:14px;"
        "font-weight:500;"
        "}"
        "QPushButton:hover{"
        "background:%3;"
        "}"
    ).arg(Theme::PRIMARY_LIGHT).arg(Theme::PRIMARY).arg(Theme::PRIMARY_LIGHTER));
    connect(openFolderBtn, &QPushButton::clicked, this, &SettingsPage::openDataFolder);
    btnGrid->addWidget(openFolderBtn, 0, 1);

    QPushButton *resetGuideBtn = new QPushButton("🔄 重置引导", this);
    resetGuideBtn->setCursor(Qt::PointingHandCursor);
    resetGuideBtn->setStyleSheet(QString(
        "QPushButton{"
        "background:%1;"
        "color:%2;"
        "border:none;"
        "border-radius:14px;"
        "padding:16px 12px;"
        "font-size:14px;"
        "font-weight:500;"
        "}"
        "QPushButton:hover{"
        "background:%3;"
        "}"
    ).arg(Theme::PRIMARY_LIGHT).arg(Theme::PRIMARY).arg(Theme::PRIMARY_LIGHTER));
    btnGrid->addWidget(resetGuideBtn, 1, 0);

    QPushButton *backupBtn = new QPushButton("💿 备份数据", this);
    backupBtn->setCursor(Qt::PointingHandCursor);
    backupBtn->setStyleSheet(QString(
        "QPushButton{"
        "background:%1;"
        "color:%2;"
        "border:none;"
        "border-radius:14px;"
        "padding:16px 12px;"
        "font-size:14px;"
        "font-weight:500;"
        "}"
        "QPushButton:hover{"
        "background:%3;"
        "}"
    ).arg(Theme::PRIMARY_LIGHT).arg(Theme::PRIMARY).arg(Theme::PRIMARY_LIGHTER));
    btnGrid->addWidget(backupBtn, 1, 1);

    v->addLayout(btnGrid);

    v->addSpacing(12);

    QFrame *dangerZone = new QFrame;
    dangerZone->setStyleSheet(QString(
        "background:%1;"
        "border-radius:16px;"
        "padding:16px;"
    ).arg("#FFF5F5"));
    QVBoxLayout *dangerV = new QVBoxLayout(dangerZone);
    dangerV->setSpacing(8);

    QLabel *dangerTitle = new QLabel("⚠️ 危险操作", this);
    dangerTitle->setStyleSheet(QString(
        "font-size:13px;"
        "font-weight:600;"
        "color:%1;"
    ).arg(Theme::DANGER));
    dangerV->addWidget(dangerTitle);

    QHBoxLayout *dangerBtns = new QHBoxLayout;
    dangerBtns->setSpacing(12);

    QPushButton *clearBtn = new QPushButton("清空所有数据", this);
    clearBtn->setCursor(Qt::PointingHandCursor);
    clearBtn->setStyleSheet(QString(
        "QPushButton{"
        "background:white;"
        "color:%1;"
        "border:1px solid %1;"
        "border-radius:12px;"
        "padding:12px 16px;"
        "font-size:13px;"
        "font-weight:500;"
        "}"
        "QPushButton:hover{"
        "background:%1;"
        "color:white;"
        "}"
    ).arg(Theme::DANGER));
    connect(clearBtn, &QPushButton::clicked, this, &SettingsPage::clearAllData);
    dangerBtns->addWidget(clearBtn);

    QPushButton *resetAllBtn = new QPushButton("恢复出厂设置", this);
    resetAllBtn->setCursor(Qt::PointingHandCursor);
    resetAllBtn->setStyleSheet(QString(
        "QPushButton{"
        "background:white;"
        "color:%1;"
        "border:1px solid %1;"
        "border-radius:12px;"
        "padding:12px 16px;"
        "font-size:13px;"
        "font-weight:500;"
        "}"
        "QPushButton:hover{"
        "background:%1;"
        "color:white;"
        "}"
    ).arg(Theme::DANGER));
    dangerBtns->addWidget(resetAllBtn);
    dangerV->addLayout(dangerBtns);

    v->addWidget(dangerZone);

    v->addStretch();

    return card;
}

QFrame* SettingsPage::createAboutCard()
{
    QFrame *card = new QFrame;
    card->setStyleSheet(QString(
        "QFrame{"
        "background:%1;"
        "border-radius:24px;"
        "padding:24px;"
        "}"
    ).arg(Theme::CARD_BG));

    QVBoxLayout *v = new QVBoxLayout(card);
    v->setSpacing(12);

    QHBoxLayout *header = new QHBoxLayout;
    header->setSpacing(12);
    QLabel *icon = new QLabel("ℹ️");
    icon->setStyleSheet("font-size:22px;");
    header->addWidget(icon);

    QLabel *title = new QLabel("关于 PKU Planner", this);
    title->setStyleSheet(QString(
        "font-size:18px;"
        "font-weight:600;"
        "color:%1;"
    ).arg(Theme::TEXT_PRIMARY));
    header->addWidget(title);
    header->addStretch();
    v->addLayout(header);

    v->addSpacing(12);

    QLabel *logo = new QLabel("🎓", this);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("font-size:56px;");
    v->addWidget(logo);

    QLabel *appName = new QLabel("PKU Planner", this);
    appName->setAlignment(Qt::AlignCenter);
    appName->setStyleSheet(QString(
        "font-size:24px;"
        "font-weight:700;"
        "color:%1;"
    ).arg(Theme::PRIMARY));
    v->addWidget(appName);

    QLabel *version = new QLabel("Version 1.0.0", this);
    version->setAlignment(Qt::AlignCenter);
    version->setStyleSheet(QString(
        "font-size:13px;"
        "color:%1;"
    ).arg(Theme::TEXT_TERTIARY));
    v->addWidget(version);

    v->addSpacing(16);

    QFrame *taglineBox = new QFrame;
    taglineBox->setStyleSheet(QString(
        "background:%1;"
        "border-radius:16px;"
        "padding:20px;"
    ).arg(Theme::PRIMARY_LIGHT));
    QVBoxLayout *taglineV = new QVBoxLayout(taglineBox);
    taglineV->setSpacing(4);

    QLabel *tagline = new QLabel("「未名湖畔好读书」", this);
    tagline->setAlignment(Qt::AlignCenter);
    tagline->setStyleSheet(QString(
        "font-size:16px;"
        "font-weight:500;"
        "color:%1;"
    ).arg(Theme::PRIMARY));
    taglineV->addWidget(tagline);

    QLabel *desc = new QLabel("By 此处应有AC", this);
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet(QString(
        "font-size:12px;"
        "color:%1;"
    ).arg(Theme::TEXT_SECONDARY));
    taglineV->addWidget(desc);

    v->addWidget(taglineBox);

    v->addSpacing(12);

    QHBoxLayout *linkRow = new QHBoxLayout;
    linkRow->setSpacing(12);
    linkRow->addStretch();

    QPushButton *docsBtn = new QPushButton("📖 使用文档", this);
    docsBtn->setCursor(Qt::PointingHandCursor);
    docsBtn->setStyleSheet(QString(
        "QPushButton{"
        "background:%1;"
        "color:%2;"
        "border:none;"
        "border-radius:12px;"
        "padding:12px 16px;"
        "font-size:13px;"
        "font-weight:500;"
        "}"
        "QPushButton:hover{"
        "background:%3;"
        "}"
    ).arg(Theme::PRIMARY_LIGHT).arg(Theme::PRIMARY).arg(Theme::PRIMARY_LIGHTER));
    linkRow->addWidget(docsBtn);

    QPushButton *feedbackBtn = new QPushButton("💬 反馈", this);
    feedbackBtn->setCursor(Qt::PointingHandCursor);
    feedbackBtn->setStyleSheet(QString(
        "QPushButton{"
        "background:%1;"
        "color:%2;"
        "border:none;"
        "border-radius:12px;"
        "padding:12px 16px;"
        "font-size:13px;"
        "font-weight:500;"
        "}"
        "QPushButton:hover{"
        "background:%3;"
        "}"
    ).arg(Theme::PRIMARY_LIGHT).arg(Theme::PRIMARY).arg(Theme::PRIMARY_LIGHTER));
    linkRow->addWidget(feedbackBtn);

    v->addLayout(linkRow);

    v->addStretch();

    return card;
}

void SettingsPage::loadSettings()
{
    reminderCheck->setChecked(true);
    reminderInterval->setCurrentIndex(1);
}

void SettingsPage::saveSettings()
{
    QSettings settings("PKUPlanner", "CourseHelper");
    settings.setValue("reminderEnabled", reminderCheck->isChecked());
    settings.setValue("reminderInterval", reminderInterval->currentIndex());

    if (statusLabel) {
        statusLabel->setText(reminderCheck->isChecked() ? "已开启" : "已关闭");
        statusLabel->setStyleSheet(QString(
            "background:%1;"
            "color:%2;"
            "padding:4px 12px;"
            "border-radius:12px;"
            "font-size:12px;"
            "font-weight:600;"
        ).arg(reminderCheck->isChecked() ? Theme::PRIMARY_LIGHT : "#F0F0F0")
          .arg(reminderCheck->isChecked() ? Theme::PRIMARY : Theme::TEXT_TERTIARY));
    }
}

void SettingsPage::exportToCSV()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出数据",
        QDateTime::currentDateTime().toString("yyyyMMdd") + "_course_helper.csv",
        "CSV Files (*.csv)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法创建文件");
        return;
    }

    QTextStream out(&file);
    out << "类型,名称,详情,截止时间,状态,优先级,创建时间\n";

    const auto courses = DataManager::instance().courses();
    for (const Course& c : courses) {
        out << QString("课程,%1,%2,%3,%4,,,%5\n")
            .arg(c.name).arg(c.teacher).arg(c.location).arg(c.examTime).arg(c.day);
    }

    const auto tasks = DataManager::instance().tasks();
    for (const Task& t : tasks) {
        QString status = t.completed ? "已完成" : (t.isOverdue() ? "逾期" : "进行中");
        QString priority = t.priority == 2 ? "高" : (t.priority == 1 ? "中" : "低");
        out << QString("任务,%1,%2,%3,%4,%5,%6\n")
            .arg(t.title).arg(t.course).arg(t.deadline.toString("yyyy-MM-dd hh:mm"))
            .arg(status).arg(priority).arg(t.hasCompletionTime() ? t.completedAt.toString("yyyy-MM-dd") : "-");
    }

    file.close();

    QMessageBox::information(this, "导出成功", QString("数据已导出到:\n%1").arg(fileName));
}

void SettingsPage::openDataFolder()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(appDataPath));
}

void SettingsPage::clearAllData()
{
    if (!ConfirmDialog::confirm(
        this,
        "清空所有数据",
        "确定要清空所有数据吗？此操作不可恢复！\n所有课程和任务都将被删除。",
        "清空",
        true
    )) {
        return;
    }

    QString dataPath = QCoreApplication::instance()
        ? QCoreApplication::applicationDirPath()
        : QDir::currentPath();
    QFile coursesFile(QDir(dataPath).absoluteFilePath("courses.json"));
    QFile tasksFile(QDir(dataPath).absoluteFilePath("tasks.json"));
    coursesFile.remove();
    tasksFile.remove();
    ToastWidget::showToast(this, "所有数据已清空，请重启应用", 4000);
}

void SettingsPage::editSemester()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("修改学期日期");
    dialog->setMinimumWidth(400);

    QVBoxLayout *v = new QVBoxLayout(dialog);
    v->setSpacing(16);

    QLabel *title = new QLabel("设置学期起止日期", dialog);
    title->setStyleSheet("font-size:16px;font-weight:600;");
    v->addWidget(title);

    QFrame *startBox = new QFrame;
    startBox->setStyleSheet("background:#F7F3EF;border-radius:12px;padding:16px;");
    QHBoxLayout *startLayout = new QHBoxLayout(startBox);
    QLabel *startLabel = new QLabel("学期开始:", dialog);
    startLabel->setStyleSheet("font-size:14px;");
    startLayout->addWidget(startLabel);

    QDateEdit *startEdit = new QDateEdit(ConfigService::instance().getSemesterStart(), dialog);
    startEdit->setCalendarPopup(true);
    startEdit->setDisplayFormat("yyyy-MM-dd");
    startEdit->setStyleSheet(R"(
        QDateEdit {
            border:1px solid #DDD;
            border-radius:8px;
            padding:8px;
            background:white;
            color: #222; /* Set text color to black */
        }
        QDateEdit::drop-down {
            border: none;
        }
        QDateEdit::down-arrow {
            image: url(none); /* Hide default arrow */
        }
        QCalendarWidget QAbstractItemView {
            color: #222; /* Ensure calendar text is black */
        }
    )");
    startLayout->addWidget(startEdit);
    v->addWidget(startBox);

    QFrame *endBox = new QFrame;
    endBox->setStyleSheet("background:#F7F3EF;border-radius:12px;padding:16px;");
    QHBoxLayout *endLayout = new QHBoxLayout(endBox);
    QLabel *endLabel = new QLabel("学期结束:", dialog);
    endLabel->setStyleSheet("font-size:14px;");
    endLayout->addWidget(endLabel);

    QDateEdit *endEdit = new QDateEdit(ConfigService::instance().getSemesterEnd(), dialog);
    endEdit->setCalendarPopup(true);
    endEdit->setDisplayFormat("yyyy-MM-dd");
    endEdit->setStyleSheet(R"(
        QDateEdit {
            border:1px solid #DDD;
            border-radius:8px;
            padding:8px;
            background:white;
            color: #222; /* Set text color to black */
        }
        QDateEdit::drop-down {
            border: none;
        }
        QDateEdit::down-arrow {
            image: url(none); /* Hide default arrow */
        }
        QCalendarWidget QAbstractItemView {
            color: #222; /* Ensure calendar text is black */
        }
    )");
    endLayout->addWidget(endEdit);
    v->addWidget(endBox);

    v->addSpacing(16);

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();

    QPushButton *cancelBtn = new QPushButton("取消", dialog);
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            background:white;
            color:#666;
            border:1px solid #DDD;
            border-radius:12px;
            padding:12px 24px;
            font-size:14px;
        }
        QPushButton:hover {background:#F5F5F5;}
    )");
    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    QPushButton *saveBtn = new QPushButton("保存", dialog);
    saveBtn->setStyleSheet(QString(R"(
        QPushButton {
            background:%1;
            color:white;
            border:none;
            border-radius:12px;
            padding:12px 24px;
            font-size:14px;
            font-weight:600;
        }
        QPushButton:hover {background:%2;}
    )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));
    connect(saveBtn, &QPushButton::clicked, [=]() {
        ConfigService::instance().setSemesterStart(startEdit->date());
        ConfigService::instance().setSemesterEnd(endEdit->date());
        dialog->accept();
    });
    btnRow->addWidget(saveBtn);

    v->addLayout(btnRow);

    dialog->exec();
    delete dialog;

    updateSemesterDisplay();
}

void SettingsPage::updateSemesterDisplay()
{
    QDate startDate = ConfigService::instance().getSemesterStart();
    QDate endDate = ConfigService::instance().getSemesterEnd();
    int currentWeek = ConfigService::instance().getCurrentWeek();
    bool isSingle = ConfigService::instance().isSingleWeek();

    int totalWeeks = startDate.daysTo(endDate) / 7;
    int progress = qMin(100, (currentWeek * 100) / qMax(1, totalWeeks));

    semesterNameLabel->setText(QString("%1 %2").arg(startDate.year()).arg(startDate.month() <= 6 ? "Spring" : "Fall"));
    startDateLabel->setText(startDate.toString("MM/dd"));
    endDateLabel->setText(endDate.toString("MM/dd"));
    percentLabel->setText(QString("%1%").arg(progress));
    progressBar->setValue(progress);
    weeksLeftLabel->setText(QString("%1周剩余").arg(qMax(0, totalWeeks - currentWeek)));
    singleWeekLabel->setText(isSingle ? "单周" : "双周");
}