#include "dataservice.h"
#include "../components/toastwidget.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QSaveFile>
#include <QTimer>
#include <QApplication>

QString DataService::getDataDir()
{
    return QCoreApplication::instance()
        ? QCoreApplication::applicationDirPath()
        : QDir::currentPath();
}

QJsonObject DataService::loadJson(const QString& path, const QJsonObject& defaultValue)
{
    QString fullPath = QDir(getDataDir()).absoluteFilePath(path);
    QFile file(fullPath);

    if (!file.exists()) {
        qWarning() << "[DataService] File does not exist:" << fullPath;
        return defaultValue;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[DataService] Cannot open file for reading:" << fullPath;
        return defaultValue;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || doc.isNull()) {
        qWarning() << "[DataService] JSON parse error:" << error.errorString() << "in file:" << fullPath;
        backupCorruptedFile(fullPath);

        QString safePath = QDir(getDataDir()).absoluteFilePath(path);
        QFile safeFile(safePath);
        if (safeFile.open(QIODevice::WriteOnly)) {
            QJsonDocument defaultDoc(defaultValue);
            safeFile.write(defaultDoc.toJson(QJsonDocument::Indented));
            safeFile.close();
            qDebug() << "[DataService] Reset to default data";
        }

        // Removed: QMetaObject::invokeMethod(... "postEvent", Q_ARG(QEvent*, nullptr)) was UB/crash
        // postEvent requires non-null receiver and non-null event; only one Q_ARG was provided.
        QTimer::singleShot(100, nullptr, [message = QString("检测到数据损坏，已自动恢复") ] {
            qDebug() << message;
        });

        return defaultValue;
    }

    if (!doc.isObject()) {
        qWarning() << "[DataService] JSON is not an object, returning default";
        return defaultValue;
    }

    qDebug() << "[DataService] Successfully loaded:" << fullPath;
    return doc.object();
}

bool DataService::saveJson(const QString& path, const QJsonObject& data)
{
    QString fullPath = QDir(getDataDir()).absoluteFilePath(path);
    QJsonDocument doc(data);

    QSaveFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[DataService] Cannot open file for writing:" << fullPath;
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));

    if (!file.commit()) {
        qWarning() << "[DataService] Failed to commit file:" << fullPath;
        return false;
    }

    qDebug() << "[DataService] Successfully saved:" << fullPath;
    return true;
}

void DataService::ensureDataFiles()
{
    QString dataDir = getDataDir();
    qDebug() << "[DataService] Ensuring data files in:" << dataDir;

    QJsonObject coursesDefault;
    QJsonObject tasksDefault;
    QJsonObject configDefault;

    configDefault["reminderEnabled"] = true;
    configDefault["reminderHours"] = 24;
    configDefault["detailDrawerMode"] = true;
    configDefault["onboardingShown"] = false;
    configDefault["semesterStart"] = QDate::currentDate().toString("yyyy-MM-dd");
    configDefault["semesterEnd"] = QDate::currentDate().addMonths(4).toString("yyyy-MM-dd");

    QStringList files = {"courses.json", "tasks.json", "config.json"};
    QList<QJsonObject> defaults = {coursesDefault, tasksDefault, configDefault};

    for (int i = 0; i < files.size(); ++i) {
        QString filePath = QDir(dataDir).absoluteFilePath(files[i]);
        if (!QFile::exists(filePath)) {
            qDebug() << "[DataService] Creating default file:" << filePath;
            QJsonDocument doc(defaults[i]);
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
            }
        }
    }

    qDebug() << "[DataService] Data files ensured";
}

QString DataService::backupCorruptedFile(const QString& path)
{
    QString backupPath = path + "." + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".corrupted";
    if (QFile::copy(path, backupPath)) {
        qDebug() << "[DataService] Backed up corrupted file to:" << backupPath;
        return backupPath;
    }
    return QString();
}

void DataService::showErrorToast(QWidget* parent, const QString& message)
{
    if (parent) {
        ToastWidget::showToast(parent, message, 4000);
    } else {
        qDebug() << "[DataService] Error:" << message;
    }
}