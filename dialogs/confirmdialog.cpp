#include "confirmdialog.h"
#include "../ui/theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QApplication>

ConfirmDialog::ConfirmDialog(
    QWidget* parent,
    const QString& title,
    const QString& message,
    const QString& confirmText,
    bool isDangerous
) : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setMinimumWidth(380);
    setMaximumWidth(420);

    QFrame *container = new QFrame(this);
    container->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 20px;
        }
    )");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 40));
    container->setGraphicsEffect(shadow);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->addWidget(container);

    QVBoxLayout *contentLayout = new QVBoxLayout(container);
    contentLayout->setSpacing(16);
    contentLayout->setContentsMargins(28, 28, 28, 24);

    QLabel *titleLabel = new QLabel(title, container);
    titleLabel->setStyleSheet(QString(
        "font-size: 18px; font-weight: 700; color: %1;"
    ).arg(isDangerous ? "#D32F2F" : Theme::TEXT_PRIMARY));
    titleLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(titleLabel);

    QLabel *messageLabel = new QLabel(message, container);
    messageLabel->setStyleSheet(QString(
        "font-size: 14px; color: %1; line-height: 22px;"
    ).arg(Theme::TEXT_SECONDARY));
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(messageLabel);

    contentLayout->addSpacing(8);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    QPushButton *cancelBtn = new QPushButton("取消", container);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedHeight(44);
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            background: #F5F5F5;
            border: none;
            border-radius: 12px;
            color: #666666;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #EEEEEE;
        }
    )");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    QPushButton *confirmBtn = new QPushButton(confirmText, container);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    confirmBtn->setFixedHeight(44);
    confirmBtn->setDefault(true);

    if (isDangerous) {
        confirmBtn->setStyleSheet(R"(
            QPushButton {
                background: #D32F2F;
                border: none;
                border-radius: 12px;
                color: white;
                font-size: 14px;
                font-weight: 600;
            }
            QPushButton:hover {
                background: #C62828;
            }
            QPushButton:pressed {
                background: #B71C1C;
            }
        )");
    } else {
        confirmBtn->setStyleSheet(QString(R"(
            QPushButton {
                background: %1;
                border: none;
                border-radius: 12px;
                color: white;
                font-size: 14px;
                font-weight: 600;
            }
            QPushButton:hover {
                background: %2;
            }
            QPushButton:pressed {
                background: %3;
            }
        )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK).arg(Theme::PRIMARY_DARK));
    }
    connect(confirmBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(cancelBtn, 1);
    btnLayout->addWidget(confirmBtn, 1);

    contentLayout->addLayout(btnLayout);
}

bool ConfirmDialog::confirm(
    QWidget* parent,
    const QString& title,
    const QString& message,
    const QString& confirmText,
    bool isDangerous
) {
    ConfirmDialog dialog(parent, title, message, confirmText, isDangerous);

    QWidget *effectiveParent = parent;
    if (!effectiveParent) {
        effectiveParent = QApplication::activeWindow();
    }

    if (effectiveParent) {
        // 强制对话框先计算自身大小
        dialog.adjustSize();
        
        QPoint center = effectiveParent->mapToGlobal(effectiveParent->rect().center());
        dialog.move(center.x() - dialog.width() / 2, center.y() - dialog.height() / 2);
    }

    return dialog.exec() == QDialog::Accepted;
}