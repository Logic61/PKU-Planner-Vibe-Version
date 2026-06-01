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

    QString geminiModel() const { return m_geminiModel; }
    void setGeminiModel(const QString &v) { m_geminiModel = v; save(); }

    QString doubaoApiUrl() const { return m_doubaoApiUrl; }
    void setDoubaoApiUrl(const QString &v) { m_doubaoApiUrl = v; save(); }

    QString doubaoModel() const { return m_doubaoModel; }
    void setDoubaoModel(const QString &v) { m_doubaoModel = v; save(); }

    QString geminiApiKey() const { return m_geminiApiKey; }
    void setGeminiApiKey(const QString &v) { m_geminiApiKey = v; save(); emit configChanged(); }

    QString doubaoApiKey() const { return m_doubaoApiKey; }
    void setDoubaoApiKey(const QString &v) { m_doubaoApiKey = v; save(); emit configChanged(); }

    QString deepseekApiUrl() const { return m_deepseekApiUrl; }
    void setDeepseekApiUrl(const QString &v) { m_deepseekApiUrl = v; save(); }

    QString deepseekModel() const { return m_deepseekModel; }
    void setDeepseekModel(const QString &v) { m_deepseekModel = v; save(); }

    QString deepseekApiKey() const { return m_deepseekApiKey; }
    void setDeepseekApiKey(const QString &v) { m_deepseekApiKey = v; save(); emit configChanged(); }

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
    QString m_geminiModel;
    QString m_doubaoApiUrl;
    QString m_doubaoModel;
    QString m_geminiApiKey;
    QString m_doubaoApiKey;
    QString m_deepseekApiUrl;
    QString m_deepseekModel;
    QString m_deepseekApiKey;
};

#endif