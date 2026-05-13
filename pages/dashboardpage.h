#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include "../services/iconfigprovider.h"
#include "../models/course.h"
#include "../models/task.h"
#include <vector>

class QGridLayout;
class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;
class QHBoxLayout;
class EmptyStateWidget;
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QProgressDialog>

enum class VisionModelType {
    Gemini,
    Doubao
};

class ScheduleVisionParser {
public:
    virtual ~ScheduleVisionParser() = default;
    virtual void parseImage(
        const QString& imagePath,
        const QString& apiKey,
        QNetworkAccessManager* networkManager,
        std::function<void(const QString&)> onSuccess,
        std::function<void(const QString&)> onError
    ) = 0;
};

class GeminiParser : public ScheduleVisionParser {
public:
    void parseImage(
        const QString& imagePath,
        const QString& apiKey,
        QNetworkAccessManager* networkManager,
        std::function<void(const QString&)> onSuccess,
        std::function<void(const QString&)> onError
    ) override;
};

class DoubaoParser : public ScheduleVisionParser {
public:
    void parseImage(
        const QString& imagePath,
        const QString& apiKey,
        QNetworkAccessManager* networkManager,
        std::function<void(const QString&)> onSuccess,
        std::function<void(const QString&)> onError
    ) override;
};

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(IConfigProvider *configProvider, QWidget *parent = nullptr);

public slots:
    void createCourse(int row, int col);
    void editCourse(int index); // for editing/deleting
    void editCourseDirect(int index); // Direct edit without ActionDialog
    void applyCourseUpdate(const Course& updatedCourse);
    void refreshCourseUrgency();
    void importSchedule();
    void parseTimeString(const QString& timeStr, Course& c);

signals:
    void navigateToTodoPageRequested();
    void openCourseDetail(const Course& course);

private:
    IConfigProvider *m_configProvider; // not owned

    void renderCourses();

    int getNearestDDL(const QString& courseName);

    QGridLayout *grid;
    QWidget *gridContainer;

    QLabel *weekLabel;
    QLabel *timeLabel;
    QProgressBar *semesterProgress;

    int currentWeek = 0;
    int realWeek = 0;

    void initGrid();
    QWidget* createTopBar();
    QWidget* createBottomStats();
    QWidget* createRightPanel();
    void updateBottomStats();

    QVBoxLayout *ddlLayout;
    QVBoxLayout *todayCourseLayout = nullptr;
    QLabel *todayCourseValue = nullptr;
    QLabel *todayDdlValue = nullptr;
    QLabel *weekDdlValue = nullptr;

    void updateDDLWidget();
    void updateTodayCourses();
    void updateWeekInfo(bool useCurrentWeek = false);
    QWidget* createSuggestionCard();

    void importFromImage();
    VisionModelType selectVisionModel();
    QString getModelApiKey(VisionModelType model);
    void callVisionAPI(VisionModelType model, const QString& apiKey, const QString& imagePath);
    void onVisionReplyFinished(QNetworkReply* reply);
    QNetworkAccessManager* m_networkManager = nullptr;
    QProgressDialog* m_loadingDialog = nullptr;
    QString m_lastResponse;
};

#endif