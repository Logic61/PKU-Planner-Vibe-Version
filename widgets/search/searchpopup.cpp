#include "searchpopup.h"
#include "../../ui/theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QGraphicsDropShadowEffect>
#include <QDesktopServices>
#include <QUrl>

SearchPopup::SearchPopup(QWidget* parent)
    : QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowFlags(windowFlags() | Qt::Tool);

    QFrame* container = new QFrame(this);
    container->setStyleSheet("QFrame { background: white; border-radius: 16px; }");

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(container);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 30));
    container->setGraphicsEffect(shadow);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(container);

    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    QScrollArea* scrollArea = new QScrollArea(container);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent; border: none;");

    contentWidget = new QWidget;
    contentWidget->setStyleSheet("background: transparent;");
    resultsLayout = new QVBoxLayout(contentWidget);
    resultsLayout->setContentsMargins(16, 12, 16, 12);
    resultsLayout->setSpacing(8);

    scrollArea->setWidget(contentWidget);
    containerLayout->addWidget(scrollArea);

    setMinimumWidth(400);
    setMaximumWidth(480);
    setMinimumHeight(60);
    setMaximumHeight(400);
}

void SearchPopup::showResults(const QVector<SearchResult>& results)
{
    clearResults();

    QVector<SearchResult> courses;
    QVector<SearchResult> tasks;
    QVector<SearchResult> files;

    for (const SearchResult& r : results) {
        switch (r.type) {
            case SearchResult::Course: courses.append(r); break;
            case SearchResult::Task: tasks.append(r); break;
            case SearchResult::File: files.append(r); break;
        }
    }

    auto addSection = [this](const QString& title, const QString& icon, const QVector<SearchResult>& items) {
        if (items.isEmpty()) return;

        QFrame* section = new QFrame(contentWidget);
        section->setStyleSheet("QFrame { background: transparent; }");
        QVBoxLayout* sectionLayout = new QVBoxLayout(section);
        sectionLayout->setContentsMargins(0, 8, 0, 4);
        sectionLayout->setSpacing(6);

        QLabel* titleLabel = new QLabel(icon + " " + title, section);
        titleLabel->setStyleSheet("color: #888; font-size: 12px; font-weight: 600; padding: 4px 0;");
        sectionLayout->addWidget(titleLabel);

        for (const SearchResult& r : items) {
            ClickableFrame* item = new ClickableFrame(section);
            item->setCursor(Qt::PointingHandCursor);
            item->setStyleSheet(QString(R"(
                QFrame {
                    background: %1;
                    border-radius: 10px;
                    padding: 10px 12px;
                }
                QFrame:hover {
                    background: %2;
                }
            )").arg(Theme::PRIMARY_LIGHTER).arg(Theme::PRIMARY_LIGHT));

            QHBoxLayout* itemLayout = new QHBoxLayout(item);
            itemLayout->setContentsMargins(8, 6, 8, 6);
            itemLayout->setSpacing(10);

            QLabel* iconLabel = new QLabel(r.icon, item);
            iconLabel->setStyleSheet("font-size: 20px;");
            itemLayout->addWidget(iconLabel);

            QVBoxLayout* textLayout = new QVBoxLayout;
            textLayout->setSpacing(2);
            QLabel* titleLabel2 = new QLabel(r.title, item);
            titleLabel2->setStyleSheet("color: #222; font-size: 14px; font-weight: 600;");
            textLayout->addWidget(titleLabel2);

            QLabel* subtitleLabel = new QLabel(r.subtitle, item);
            subtitleLabel->setStyleSheet("color: #888; font-size: 12px;");
            subtitleLabel->setWordWrap(true);
            textLayout->addWidget(subtitleLabel);

            itemLayout->addLayout(textLayout, 1);
            sectionLayout->addWidget(item);

            switch (r.type) {
                case SearchResult::Course:
                    connect(item, &ClickableFrame::clicked, this, [this, r]() {
                        emit courseSelected(r.id);
                    });
                    break;
                case SearchResult::Task:
                    connect(item, &ClickableFrame::clicked, this, [this, r]() {
                        bool ok;
                        int index = r.id.toInt(&ok);
                        if (ok) emit taskSelected(index);
                    });
                    break;
                case SearchResult::File:
                    connect(item, &ClickableFrame::clicked, this, [this, r]() {
                        emit fileSelected(r.id);
                    });
                    break;
            }
        }

        resultsLayout->addWidget(section);
    };

    addSection("课程", "📚", courses);
    addSection("任务", "📝", tasks);
    addSection("文件", "📄", files);

    if (results.isEmpty()) {
        QLabel* noResult = new QLabel("未找到相关内容", contentWidget);
        noResult->setAlignment(Qt::AlignCenter);
        noResult->setStyleSheet("color: #999; font-size: 14px; padding: 30px;");
        resultsLayout->addWidget(noResult);
    }

    resultsLayout->addStretch();
    setMinimumHeight(qMin(400, 60 + results.size() * 70));
}

void SearchPopup::clearResults()
{
    QLayoutItem* item;
    while ((item = resultsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}