#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QStringList>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct TodoItem {
    QString text;
    QDateTime deadline;
    bool isCompleted;
    QString remarks;
    QString source; // e.g. "manual", "sync"
};

struct Course {
    QString name;
    QString location;
    QString teacher;
    QDateTime examTime;
    QString folderPath;
    int day;           // 1-7 (周一到周日)
    int start;         // 起始节次
    int duration;      // 持续节数
    enum WeekType { All, Odd, Even } weekType;
    QString color; // 课程卡片颜色（hex）
    QList<TodoItem> todos; // 待办事项列表
};

class QTableWidget;
class QTableWidgetItem;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QLineEdit;
class QPushButton;
class QDateTimeEdit;
class QComboBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCellClicked(int row, int column);
    void onCellDoubleClicked(int row, int column);
    void onTodoItemClicked(QTableWidgetItem *item);
    void onAddTodoClicked();
    void updateTime();
    void showDashboardDialog();
    void showCourseDetailsDialog(int courseIndex);
    void onToggleThemeClicked();
    void applyTheme();

private:    
    Ui::MainWindow *ui;
    bool m_isDarkMode = false;
    QList<Course> m_courses;
    QTableWidget *m_tableWidget;
    QTableWidget *m_todoTableWidget;
    QLabel *m_timeLabel;
    QPushButton *m_mascotBtn;
    QLineEdit *m_todoInput;
    QDateTimeEdit *m_todoDateInput;
    QPushButton *m_addTodoBtn;
    
    QComboBox *m_todoSortCombo;
    QComboBox *m_todoTimeFilterCombo;
    QComboBox *m_todoCourseFilterCombo;
    
    int m_currentDay = -1;
    int m_currentSection = -1;
    int m_currentSelectedCourseIndex = -1;
    
    void initCalendarView();
    void refreshSchedule();
    void loadInitialData();
    void refreshTodoList();
    // popup todo near the course cell
    void showTodoPopupForCourse(int courseIndex, int row, int col);
    void closeTodoPopup();

    QWidget *m_popup = nullptr;
    QListWidget *m_popupTodoList = nullptr;
    int m_popupCourseIndex = -1;
};
#endif // MAINWINDOW_H
