#ifndef COURSE_H
#define COURSE_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>

struct Course {
    QString name;
    QString teacher;
    QString location;
    QString examTime;
    int day;           // 1-7 (周一到周日)
    int startPeriod;   // 起始节次
    int endPeriod;     // 结束节次
    int weekType = 0;  // 0: All, 1: Odd, 2: Even

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["teacher"] = teacher;
        obj["location"] = location;
        obj["examTime"] = examTime;
        obj["day"] = day;
        obj["startPeriod"] = startPeriod;
        obj["endPeriod"] = endPeriod;
        obj["weekType"] = weekType;
        return obj;
    }

    static Course fromJson(QJsonObject obj) {
        Course c;
        c.name = obj["name"].toString();
        c.teacher = obj["teacher"].toString();
        c.location = obj["location"].toString();
        c.examTime = obj["examTime"].toString();
        c.day = obj["day"].toInt();
        c.startPeriod = obj["startPeriod"].toInt();
        c.endPeriod = obj["endPeriod"].toInt();
        c.weekType = obj["weekType"].toInt(0);
        return c;
    }
};

#endif // COURSE_H