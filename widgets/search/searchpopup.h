#ifndef SEARCHPOPUP_H
#define SEARCHPOPUP_H

#include <QWidget>
#include <QVector>
#include <QMouseEvent>
#include <QEvent>
#include <QFrame>
#include "../../services/searchservice.h"

class QVBoxLayout;
class QLabel;

class ClickableFrame : public QFrame {
    Q_OBJECT
public:
    explicit ClickableFrame(QWidget* parent = nullptr) : QFrame(parent) {}
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent* event) override {
        QFrame::mousePressEvent(event);
        emit clicked();
    }
    void enterEvent(QEnterEvent* event) override {
        QFrame::enterEvent(event);
        setProperty("hovered", true);
        setStyleSheet(styleSheet());
    }
    void leaveEvent(QEvent* event) override {
        QFrame::leaveEvent(event);
        setProperty("hovered", false);
        setStyleSheet(styleSheet());
    }
};

class SearchPopup : public QWidget
{
    Q_OBJECT
public:
    explicit SearchPopup(QWidget* parent = nullptr);
    void showResults(const QVector<SearchResult>& results, const QString& keyword = QString());

 signals:
    void courseSelected(const QString& courseName);
    void taskSelected(int taskIndex);
    void fileSelected(const QString& filePath);

private:
    void addSection(const QString& title, const QString& icon, const QVector<SearchResult>& items, const QString& keyword);
    void clearResults();
    QString highlightText(const QString& text, const QString& keyword);
    bool eventFilter(QObject* obj, QEvent* event) override;
    QVBoxLayout* resultsLayout;
    QWidget* contentWidget;
    QString currentKeyword;
};

#endif