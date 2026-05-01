#include "coursecellwidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QToolTip>
#include <QTimer>
#include <QEnterEvent>

CourseCellWidget::CourseCellWidget(QWidget *parent)
    : QFrame(parent)
{
    setMinimumSize(80, 60);

    setStyleSheet(R"(
        QFrame {
            background:#FAFAFA;
            border-radius:10px;
        }
        QFrame:hover {
            background:#FFEAEA;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6,6,6,6);

    title = new QLabel("");
    title->setStyleSheet("font-weight:bold; font-size:12px;");

    info = new QLabel("");
    info->setStyleSheet("font-size:11px; color:#666;");

    layout->addWidget(title);
    layout->addWidget(info);
}

void CourseCellWidget::setCourse(QString name, QString location)
{
    title->setText(name);
    info->setText(location);

    setStyleSheet(R"(
        QFrame {
            background: #E8F0FF;
            border-radius: 10px;
        }
        QFrame:hover {
            background: #D6E4FF;
        }
    )");
}

void CourseCellWidget::enterEvent(QEnterEvent *)
{
    QTimer::singleShot(300, this, [this]() {
        if(!title->text().isEmpty())
        {
            QToolTip::showText(cursor().pos(),
                title->text() + "\n地点: " + info->text() +
                "\nDDL: 3天后");
        }
    });
}

void CourseCellWidget::leaveEvent(QEvent *)
{
    QToolTip::hideText();
}