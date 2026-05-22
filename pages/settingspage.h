#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QSettings>
#include <QFrame>
#include <QDir>
#include <QDateEdit>
#include <QProgressBar>

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

signals:
    void syncTodosFromTeachingPlatformRequested();

private:
    QCheckBox *reminderCheck;
    QComboBox *reminderInterval;
    QPushButton *exportBtn;
    QPushButton *importBtn;
    QLabel *statusLabel;

    QLabel *semesterNameLabel;
    QLabel *startDateLabel;
    QLabel *endDateLabel;
    QLabel *percentLabel;
    QLabel *weeksLeftLabel;
    QLabel *singleWeekLabel;
    QProgressBar *progressBar;

    QFrame* createReminderCard();
    QFrame* createSemesterCard();
    QFrame* createDataCard();
    QFrame* createAboutCard();

    void loadSettings();
    void saveSettings();
    void importSchedule();
    void exportToCSV();
    void openDocs();
    void backupData();
    void resetGuide();
    void factoryReset();
    void openFeedback();
    void openDataFolder();
    void clearAllData();
    void editSemester();
    void updateSemesterDisplay();
};

#endif