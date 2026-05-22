#include "configservice.h"
#include "../models/datarepository.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QSaveFile>
#include <QDateTime>

ConfigService& ConfigService::instance()
{
    static ConfigService instance;
    return instance;
}

ConfigService::ConfigService()
    : QObject(nullptr)
    , m_reminderEnabled(true)
    , m_reminderHours(24)
    , m_detailDrawerMode(true)
    , m_onboardingShown(false)
    , m_semesterStart(QDate(2026, 3, 1))
    , m_semesterEnd(QDate(2026, 6, 28))
    , m_lastSummaryDate()
    , m_teachingUsername("")
    , m_teachingPassword("")
{
    load();
}

void ConfigService::load()
{
    QString dataPath = DataRepository::dataDirectory();
    QString fullPath = QDir(dataPath).absoluteFilePath("config.json");

    QFile file(fullPath);
    if (!file.exists()) {
        qDebug() << "[ConfigService] config.json not found, will be created";
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[ConfigService] Cannot open config file:" << fullPath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || doc.isNull() || !doc.isObject()) {
        qWarning() << "[ConfigService] JSON parse error:" << error.errorString();
        
        QString backupPath = fullPath + "." + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".corrupted";
        QFile::copy(fullPath, backupPath);
        qWarning() << "[ConfigService] Backed up corrupted config to:" << backupPath;
        
        QFile::remove(fullPath);
        return;
    }

    QJsonObject obj = doc.object();
    m_reminderEnabled = obj.value("reminderEnabled").toBool(true);
    m_reminderHours = obj.value("reminderHours").toInt(24);
    m_detailDrawerMode = obj.value("detailDrawerMode").toBool(true);
    m_exportPath = obj.value("exportPath").toString("");
    m_onboardingShown = obj.value("onboardingShown").toBool(false);
    m_teachingUsername = obj.value("teachingUsername").toString("");
    m_teachingPassword = obj.value("teachingPassword").toString("");

    QString startStr = obj.value("semesterStart").toString();
    QString endStr = obj.value("semesterEnd").toString();
    if (!startStr.isEmpty()) {
        m_semesterStart = QDate::fromString(startStr, "yyyy-MM-dd");
    }
    if (!endStr.isEmpty()) {
        m_semesterEnd = QDate::fromString(endStr, "yyyy-MM-dd");
    }

    QString summaryStr = obj.value("lastSummaryDate").toString();
    if (!summaryStr.isEmpty()) {
        m_lastSummaryDate = QDate::fromString(summaryStr, "yyyy-MM-dd");
    }

    qDebug() << "[ConfigService] Loaded config";
}

void ConfigService::save()
{
    QString dataPath = DataRepository::dataDirectory();
    QString fullPath = QDir(dataPath).absoluteFilePath("config.json");

    QJsonObject obj;
    obj["reminderEnabled"] = m_reminderEnabled;
    obj["reminderHours"] = m_reminderHours;
    obj["detailDrawerMode"] = m_detailDrawerMode;
    obj["exportPath"] = m_exportPath;
    obj["onboardingShown"] = m_onboardingShown;
    obj["semesterStart"] = m_semesterStart.toString("yyyy-MM-dd");
    obj["semesterEnd"] = m_semesterEnd.toString("yyyy-MM-dd");
    if (m_lastSummaryDate.isValid()) {
        obj["lastSummaryDate"] = m_lastSummaryDate.toString("yyyy-MM-dd");
    }
    obj["teachingUsername"] = m_teachingUsername;
    obj["teachingPassword"] = m_teachingPassword;

    QJsonDocument doc(obj);

    QSaveFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[ConfigService] Cannot open config file for writing:" << fullPath;
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));

    if (!file.commit()) {
        qWarning() << "[ConfigService] Failed to commit config file";
        return;
    }

    qDebug() << "[ConfigService] Saved config";
}

bool ConfigService::isReminderEnabled() const
{
    return m_reminderEnabled;
}

void ConfigService::setReminderEnabled(bool enabled)
{
    m_reminderEnabled = enabled;
    save();
    emit configChanged();
}

int ConfigService::getReminderHours() const
{
    return m_reminderHours;
}

void ConfigService::setReminderHours(int hours)
{
    m_reminderHours = hours;
    save();
    emit configChanged();
}

bool ConfigService::isDetailDrawerMode() const
{
    return m_detailDrawerMode;
}

void ConfigService::setDetailDrawerMode(bool drawerMode)
{
    m_detailDrawerMode = drawerMode;
    save();
    emit configChanged();
}

QString ConfigService::getExportPath() const
{
    if (m_exportPath.isEmpty()) {
        return DataRepository::dataDirectory();
    }
    return m_exportPath;
}

void ConfigService::setExportPath(const QString& path)
{
    m_exportPath = path;
    save();
    emit configChanged();
}

bool ConfigService::isOnboardingShown() const
{
    return m_onboardingShown;
}

void ConfigService::setOnboardingShown(bool shown)
{
    m_onboardingShown = shown;
    save();
    emit configChanged();
}

void ConfigService::resetOnboarding()
{
    setOnboardingShown(false);
}

void ConfigService::resetAllData()
{
    QString dataPath = DataRepository::dataDirectory();

    QFile coursesFile(dataPath + "/courses.json");
    if (coursesFile.exists()) {
        coursesFile.remove();
    }

    QFile tasksFile(dataPath + "/tasks.json");
    if (tasksFile.exists()) {
        tasksFile.remove();
    }

    QFile configFile(dataPath + "/config.json");
    if (configFile.exists()) {
        configFile.remove();
    }

    load();

    qDebug() << "[ConfigService] All data reset";
}

QString ConfigService::getDataPath() const
{
    return DataRepository::dataDirectory();
}

QDate ConfigService::getSemesterStart() const
{
    return m_semesterStart;
}

void ConfigService::setSemesterStart(const QDate& date)
{
    m_semesterStart = date;
    save();
    emit configChanged();
}

QDate ConfigService::getSemesterEnd() const
{
    return m_semesterEnd;
}

void ConfigService::setSemesterEnd(const QDate& date)
{
    m_semesterEnd = date;
    save();
    emit configChanged();
}

QDate ConfigService::getLastSummaryDate() const
{
    return m_lastSummaryDate;
}

void ConfigService::setLastSummaryDate(const QDate& date)
{
    m_lastSummaryDate = date;
    save();
}

int ConfigService::getCurrentWeek() const
{
    if (!m_semesterStart.isValid()) return 1;
    int days = m_semesterStart.daysTo(QDate::currentDate());
    if (days < 0) return 1;
    return days / 7 + 1;
}

bool ConfigService::isSingleWeek() const
{
    return getCurrentWeek() % 2 == 1;
}