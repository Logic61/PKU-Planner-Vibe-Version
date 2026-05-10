#ifndef STATSPAGE_H
#define STATSPAGE_H

#include <QWidget>
#include <QLayout>
#include <QEvent>
#include <QDate>

class QFrame;
class QLabel;
class QProgressBar;
class QGridLayout;
class QScrollArea;
class EmptyStateWidget;
class Task;
class Course;

class StatsPage : public QWidget
{
    Q_OBJECT
public:
    explicit StatsPage(QWidget *parent = nullptr);
    void refresh();
    void refreshData();

private slots:
    void showWeeklySummary();
    void changeMonth(int delta);

private:
    void updateEmptyState();
    void clearLayout(QLayout* layout);
    void updateSummary(const QList<Task>& tasks);
    void updateHeatmap(const QList<Task>& tasks);
    void updateTrend(const QList<Task>& tasks);
    void updateSuggestions(const QList<Task>& tasks, const QList<Course>& courses);
    bool eventFilter(QObject *obj, QEvent *event) override;

    QFrame* cardTotal;
    QFrame* cardDone;
    QFrame* cardOnTime;
    QFrame* cardAvg;
    QGridLayout* heatGrid;
    QHBoxLayout* trendContainer;
    QVBoxLayout* suggestContainer;
    QWidget* contentWidget;
    QScrollArea* scrollArea;
    EmptyStateWidget* emptyStateWidget = nullptr;
    QDate currentMonth;
};

#endif // STATSPAGE_H