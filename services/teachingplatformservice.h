#ifndef TEACHINGPLATFORMSERVICE_H
#define TEACHINGPLATFORMSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include <QJsonObject>
#include <QUrl>
#include <QString>
#include <QStringList>

class TeachingPlatformService : public QObject {
    Q_OBJECT

public:
    explicit TeachingPlatformService(QObject *parent = nullptr);

    void login(const QString &username,
               const QString &password,
               const QString &otp = QString());

    void fetchTodoTasks();

    void fetchTodoTasksWithCredentials(const QString &username,
                                       const QString &password,
                                       const QString &otp = QString());

    void fetchCourseTable();

    bool hasCredentials() const {
        return !lastUsername.isEmpty() && !lastPassword.isEmpty();
    }

signals:
    void loginSuccess();
    void loginFailed(const QString &error);

    void tasksFetched(const QList<QJsonObject> &tasks);
    void fetchFailed(const QString &error);
    void todoAuthRequired(const QString &error);

    void courseTableFetched(const QJsonObject &data);
    void courseTableFetchFailed(const QString &error);

private:
    QNetworkAccessManager *networkManager;
    QString sessionCookie;

    QString lastUsername;
    QString lastPassword;
    QString lastOtp;

    void handleLoginResponse(QNetworkReply *reply);
    void handleFetchTasksResponse(QNetworkReply *reply);

    void followRedirectAndEstablishSession(const QUrl &url, int remaining = 6);

    // Blackboard 待办同步
    void loginBlackboardAndFetchTodoTasks();
    void fetchTodoTasksAfterBlackboardLogin();

    // Portal 课表同步
    void loginPortalAndFetchCourseTable();
    void fetchCourseTableAfterPortalLogin();

    // 旧接口保留：cpp 里仍有空实现，避免链接/声明不一致
    QStringList extractCourseKeysFromHomepage(const QString &html);
    QList<QJsonObject> extractAssignmentsFromCoursePage(const QString &html,
                                                        const QString &courseKey);
    QList<QJsonObject> extractAssignmentsFromCoursePageTokenizer(const QString &html,
                                                                 const QString &courseKey);
};

#endif // TEACHINGPLATFORMSERVICE_H
