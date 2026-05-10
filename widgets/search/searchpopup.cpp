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

void SearchPopup::showResults(const QVector<SearchResult>& results, const QString& keyword)
{
    clearResults();
    currentKeyword = keyword;

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

    addSection("课程", "📚", courses, keyword);
    addSection("任务", "📝", tasks, keyword);
    addSection("文件", "📄", files, keyword);

    if (results.isEmpty()) {
        QLabel* noResult = new QLabel("未找到相关内容", contentWidget);
        noResult->setAlignment(Qt::AlignCenter);
        noResult->setStyleSheet("color: #999; font-size: 14px; padding: 30px;");
        resultsLayout->addWidget(noResult);
    }

    resultsLayout->addStretch();
    setMinimumHeight(qMin(400, 60 + results.size() * 70));
}

void SearchPopup::addSection(const QString& title, const QString& icon, const QVector<SearchResult>& items, const QString& keyword)
{
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
            QFrame[hovered="true"] {
                background: %2;
            }
            QFrame[hovered="true"] QLabel {
                color: white;
            }
        )").arg(Theme::PRIMARY_LIGHTER).arg(Theme::PRIMARY));
        
connect(item, &ClickableFrame::clicked, this, [this, r]() {
            if (r.type == SearchResult::Course) emit courseSelected(r.id);
            else if (r.type == SearchResult::Task) emit taskSelected(r.id.toInt());
            else if (r.type == SearchResult::File) emit fileSelected(r.id);
        });

        QHBoxLayout* itemLayout = new QHBoxLayout(item);
        itemLayout->setContentsMargins(8, 6, 8, 6);
        itemLayout->setSpacing(10);

        QLabel* iconLabel = new QLabel(r.icon, item);
        iconLabel->setStyleSheet("font-size: 20px;");
        itemLayout->addWidget(iconLabel);

        QVBoxLayout* textLayout = new QVBoxLayout;
        textLayout->setSpacing(2);
        QLabel* titleLabel2 = new QLabel(highlightText(r.title, keyword), item);
        titleLabel2->setStyleSheet("font-size: 14px; font-weight: 600;");
        textLayout->addWidget(titleLabel2);

        QLabel* subtitleLabel = new QLabel(highlightText(r.subtitle, keyword), item);
        subtitleLabel->setStyleSheet("font-size: 12px;");
        subtitleLabel->setWordWrap(true);
        textLayout->addWidget(subtitleLabel);

        itemLayout->addLayout(textLayout, 1);
        sectionLayout->addWidget(item);
    }

    resultsLayout->addWidget(section);
}

QString SearchPopup::highlightText(const QString& text, const QString& keyword)
{
    if (keyword.isEmpty()) return text;

    QString lowerText = text.toLower();
    QString lowerKeyword = keyword.toLower();

    int index = lowerText.indexOf(lowerKeyword);
    if (index >= 0) {
        QString before = text.left(index);
        QString match = text.mid(index, keyword.length());
        QString after = text.mid(index + keyword.length());
        return before + "<span style=\"color:" + Theme::WARNING + ";font-weight:700;\">" + match + "</span>" + after;
    }

    return text;
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