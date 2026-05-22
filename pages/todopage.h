#ifndef TODOPAGE_H
#define TODOPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>

class QScrollArea;
class QVBoxLayout;
class QLabel;
class EmptyStateWidget;

class TodoPage : public QWidget
{
    Q_OBJECT

public:
    explicit TodoPage(QWidget *parent = nullptr);

public slots:
    void reloadTasks();
    void highlightTask(int taskIndex);
    void setupUI();

private:
    QComboBox *courseFilter = nullptr;
    QComboBox *timeFilter = nullptr;
    QComboBox *statusFilter = nullptr;
    QComboBox *sortCombo = nullptr;
    QCheckBox *hideCompletedCheck = nullptr;
    QLineEdit *searchEdit = nullptr;
    QLabel *summaryLabel = nullptr;
    QScrollArea *scrollArea = nullptr;
    QWidget *boardWidget = nullptr;
    QVBoxLayout *boardLayout = nullptr;

    QWidget* createFilterBar();
    QWidget* createSectionHeader(const QString &title, int count, const QString &accent);
    void refreshCourseFilter();
    void refreshTasks();
    void applyFilter();
    void rebuildBoard();
    void editTaskByIndex(int sourceIndex);
    EmptyStateWidget *emptyStateWidget = nullptr;
};

#endif