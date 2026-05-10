#ifndef TOPBARWIDGET_H
#define TOPBARWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QTimer>
#include "../widgets/search/searchpopup.h"

class TopbarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TopbarWidget(QWidget *parent = nullptr);
    ~TopbarWidget();

 signals:
    void searchCourseRequested(const QString& courseName);
    void searchTaskRequested(int taskIndex);
    void fileSelected(const QString& filePath);

public:
    QLineEdit* getSearchEdit() const;

private slots:
    void onSearchTextChanged(const QString& text);
    void onSearchReturned();
    void onCourseSelected(const QString& courseName);
    void onTaskSelected(int taskIndex);
    void onFileSelected(const QString& filePath);
    void doSearch();

private:
    void positionPopup();
    bool eventFilter(QObject *obj, QEvent *event) override;

    QLineEdit *searchEdit;
    SearchPopup *searchPopup;
    QTimer *searchTimer;
    QString currentSearchText;
};

#endif