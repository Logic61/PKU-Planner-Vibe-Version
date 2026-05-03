#ifndef COURSEEDITDIALOG_H
#define COURSEEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>

class CourseEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CourseEditDialog(int defaultStart = 1, int defaultEnd = 2, QWidget *parent = nullptr);

    QString getName() const;
    QString getTeacher() const;
    QString getLocation() const;
    QString getExamTime() const;
    int getStart() const;
    int getEnd() const;
    int getWeekType() const;

    void setCourseData(const QString &name, const QString &teacher, 
                      const QString &location, const QString &examTime,
                      int start, int end, int weekType = 0);

private slots:
    void onAccepted();

private:
    QLineEdit *nameEdit;
    QLineEdit *teacherEdit;
    QLineEdit *locationEdit;
    QLineEdit *examEdit;
    QComboBox *startCombo;
    QComboBox *endCombo;
    QComboBox *weekTypeCombo;
};

#endif // COURSEEDITDIALOG_H
