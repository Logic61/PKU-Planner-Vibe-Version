#include "topbarwidget.h"
#include "../ui/theme.h"
#include "../services/searchservice.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDebug>

TopbarWidget::TopbarWidget(QWidget *parent)
    : QWidget(parent)
    , searchPopup(nullptr)
    , searchTimer(new QTimer(this))
{
    setFixedHeight(50);
    setStyleSheet("background:#F7F3EF;");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 0, 16, 0);

    QLabel *title = new QLabel("PKU Planner+");
    title->setStyleSheet("font-weight:bold; font-size:16px; color:#222;");

    searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText("搜索课程、任务、文件...");
    searchEdit->setFixedWidth(260);
    searchEdit->setFixedHeight(32);
    searchEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background: white;
            border: 1px solid #E0D5D5;
            border-radius: 16px;
            padding: 0 14px;
            font-size: 13px;
            color: #222;
        }
        QLineEdit:focus {
            border: 1px solid %1;
            background: white;
        }
        QLineEdit::placeholder {
            color: #BDBDBD;
        }
    )").arg(Theme::PRIMARY));

    searchPopup = new SearchPopup(this);

    searchTimer->setSingleShot(true);
    searchTimer->setInterval(150);

    connect(searchEdit, &QLineEdit::textChanged, this, &TopbarWidget::onSearchTextChanged);
    connect(searchEdit, &QLineEdit::returnPressed, this, &TopbarWidget::onSearchReturned);
    connect(searchPopup, &SearchPopup::courseSelected, this, &TopbarWidget::onCourseSelected);
    connect(searchPopup, &SearchPopup::taskSelected, this, &TopbarWidget::onTaskSelected);
    connect(searchPopup, &SearchPopup::fileSelected, this, &TopbarWidget::onFileSelected);
    connect(searchTimer, &QTimer::timeout, this, &TopbarWidget::doSearch);

    layout->addWidget(title);
    layout->addStretch();
    layout->addWidget(searchEdit);
}

TopbarWidget::~TopbarWidget()
{
}

QLineEdit* TopbarWidget::getSearchEdit() const {
    return searchEdit;
}

void TopbarWidget::onSearchTextChanged(const QString& text)
{
    currentSearchText = text;
    if (text.trimmed().isEmpty()) {
        searchTimer->stop();
        searchPopup->hide();
        return;
    }

    searchTimer->stop();
    searchTimer->start();
}

void TopbarWidget::doSearch()
{
    const QString& text = currentSearchText;
    if (text.trimmed().isEmpty()) {
        searchPopup->hide();
        return;
    }

    QVector<SearchResult> results = SearchService::search(text);
    searchPopup->showResults(results);

    if (!results.isEmpty()) {
        positionPopup();
        searchPopup->show();
    } else {
        searchPopup->hide();
    }
}

void TopbarWidget::onSearchReturned()
{
    QString text = searchEdit->text().trimmed();
    if (!text.isEmpty()) {
        QVector<SearchResult> results = SearchService::search(text);
        if (!results.isEmpty()) {
            const SearchResult &first = results.first();
            if (first.type == SearchResult::Course) {
                emit searchCourseRequested(first.title);
            } else if (first.type == SearchResult::Task) {
                bool ok = false;
                int idx = first.id.toInt(&ok);
                if (ok) emit searchTaskRequested(idx);
            }
        }
        searchPopup->hide();
    }
}

void TopbarWidget::onCourseSelected(const QString& courseName)
{
    qDebug() << "[Topbar] Course selected:" << courseName;
    searchEdit->clear();
    searchPopup->hide();
    emit searchCourseRequested(courseName);
}

void TopbarWidget::onTaskSelected(int taskIndex)
{
    qDebug() << "[Topbar] Task selected index:" << taskIndex;
    searchEdit->clear();
    searchPopup->hide();
    emit searchTaskRequested(taskIndex);
}

void TopbarWidget::onFileSelected(const QString& filePath)
{
    qDebug() << "[Topbar] File selected:" << filePath;
    searchEdit->clear();
    searchPopup->hide();
    emit fileSelected(filePath);
}

void TopbarWidget::positionPopup()
{
    QPoint globalPos = searchEdit->mapToGlobal(QPoint(0, searchEdit->height() + 4));
    int popupWidth = searchEdit->width();
    searchPopup->setFixedWidth(popupWidth);
    searchPopup->move(globalPos);
}