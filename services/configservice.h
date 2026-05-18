#ifndef CONFIGSERVICE_H
#define CONFIGSERVICE_H

#include <QObject>
#include <QString>
#include <QDate>
#include "iconfigprovider.h"

class ConfigService : public QObject, public IConfigProvider
{
    Q_OBJECT

public:
    static ConfigService& instance();

    bool isReminderEnabled() const;
    void setReminderEnabled(bool enabled);

    int getReminderHours() const;
    void setReminderHours(int hours);

    bool isDetailDrawerMode() const;
    void setDetailDrawerMode(bool drawerMode);

    QString getExportPath() const;
    void setExportPath(const QString& path);

    bool isOnboardingShown() const;
    void setOnboardingShown(bool shown);

    QDate getSemesterStart() const;
    void setSemesterStart(const QDate& date);

    QDate getSemesterEnd() const;
    void setSemesterEnd(const QDate& date);

    QDate getLastSummaryDate() const;
    void setLastSummaryDate(const QDate& date);

    int getCurrentWeek() const;
    bool isSingleWeek() const;

    void resetOnboarding();
    void resetAllData();

    QString getDataPath() const;
    QString getTeachingUsername() const { return m_teachingUsername; }
    QString getTeachingPassword() const { return m_teachingPassword; }
    void setTeachingUsername(const QString &u) { m_teachingUsername = u; save(); emit configChanged(); }
    void setTeachingPassword(const QString &p) { m_teachingPassword = p; save(); emit configChanged(); }


    void onConfigChanged() override { emit configChanged(); }

signals:
    void configChanged();

private:
    ConfigService();
    void load();
    void save();

    bool m_reminderEnabled;
    int m_reminderHours;
    bool m_detailDrawerMode;
    QString m_exportPath;
    bool m_onboardingShown;
    QDate m_semesterStart;
    QDate m_semesterEnd;
    QDate m_lastSummaryDate;
    QString m_teachingUsername;
    QString m_teachingPassword;
};

#endif