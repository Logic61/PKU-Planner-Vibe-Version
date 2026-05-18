#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDate>
#include <QDebug>
#include "mainwindow.h"
#include "services/configservice.h"

static void ensureDataFiles()
{
    QString dataDir = QCoreApplication::applicationDirPath();
    qDebug() << "[Startup] Ensuring data files in:" << dataDir;

    QJsonObject configDefault;
    configDefault["reminderEnabled"] = true;
    configDefault["reminderHours"] = 24;
    configDefault["detailDrawerMode"] = true;
    configDefault["onboardingShown"] = false;
    configDefault["semesterStart"] = QDate::currentDate().toString("yyyy-MM-dd");
    configDefault["semesterEnd"] = QDate::currentDate().addMonths(4).toString("yyyy-MM-dd");
    configDefault["teachingUsername"] = "";
    configDefault["teachingPassword"] = "";
    
    QStringList files = {"courses.json", "tasks.json", "config.json"};
    QList<QJsonObject> defaults = {QJsonObject(), QJsonObject(), configDefault};

    for (int i = 0; i < files.size(); ++i) {
        QString filePath = QDir(dataDir).absoluteFilePath(files[i]);
        if (!QFile::exists(filePath)) {
            qDebug() << "[Startup] Creating default file:" << filePath;
            QJsonDocument doc(defaults[i]);
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
            }
        }
    }

    qDebug() << "[Startup] Data files ensured";
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Course Helper");
    a.setApplicationDisplayName("Course Helper");

    ensureDataFiles();

    MainWindow w(&ConfigService::instance());
    w.setWindowTitle("Course Helper");
    w.show();
    return a.exec();
}