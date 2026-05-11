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
#include <QScreen>
#include <QGuiApplication>

ConfirmDialog::ConfirmDialog(
    QWidget* parent,
    const QString& title,
    const QString& message,
    const QString& confirmText,
    bool isDangerous
) : QDialog(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setMinimumWidth(400);
    setMaximumWidth(440);

    QFrame *container = new QFrame(this);
    container->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 24px;
        }
    )");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(0, 0, 0, 50));
    container->setGraphicsEffect(shadow);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->addWidget(container);

    QVBoxLayout *contentLayout = new QVBoxLayout(container);
    contentLayout->setSpacing(20);
    contentLayout->setContentsMargins(32, 32, 32, 28);

    QString iconEmoji = isDangerous ? "⚠️" : "💡";
    QLabel *iconLabel = new QLabel(iconEmoji, container);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 48px;");
    contentLayout->addWidget(iconLabel);

    QLabel *titleLabel = new QLabel(title, container);
    titleLabel->setStyleSheet(QString(
        "font-size: 20px; font-weight: 700; color: %1;"
    ).arg(isDangerous ? "#D32F2F" : Theme::TEXT_PRIMARY));
    titleLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(titleLabel);

    QLabel *messageLabel = new QLabel(message, container);
    messageLabel->setStyleSheet(QString(
        "font-size: 14px; color: %1; line-height: 24px;"
    ).arg(Theme::TEXT_SECONDARY));
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setMinimumHeight(60);
    contentLayout->addWidget(messageLabel);

    contentLayout->addSpacing(12);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(16);

    QPushButton *cancelBtn = new QPushButton("取消", container);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedHeight(48);
    cancelBtn->setMinimumWidth(100);
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            background: #F5F5F5;
            border: none;
            border-radius: 14px;
            color: #666666;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #EEEEEE;
        }
    )");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    QPushButton *confirmBtn = new QPushButton(confirmText, container);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    confirmBtn->setFixedHeight(48);
    confirmBtn->setMinimumWidth(100);
    confirmBtn->setDefault(true);

    if (isDangerous) {
        confirmBtn->setStyleSheet(R"(
            QPushButton {
                background: #D32F2F;
                border: none;
                border-radius: 14px;
                color: white;
                font-size: 15px;
                font-weight: 600;
            }
            QPushButton:hover {
                background: #E53935;
            }
        )");
    } else {
        confirmBtn->setStyleSheet(QString(R"(
            QPushButton {
                background: %1;
                border: none;
                border-radius: 14px;
                color: white;
                font-size: 15px;
                font-weight: 600;
            }
            QPushButton:hover {
                background: %2;
            }
        )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));
    }
    connect(confirmBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(confirmBtn);
    btnLayout->addStretch();

    contentLayout->addLayout(btnLayout);
}

ConfirmDialog::ConfirmDialog(
    QWidget* parent,
    const QString& title,
    const QString& message,
    const QString& yesText,
    const QString& noText,
    bool isDangerous
) : QDialog(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setMinimumWidth(420);
    setMaximumWidth(460);

    QFrame *container = new QFrame(this);
    container->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 24px;
        }
    )");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(0, 0, 0, 50));
    container->setGraphicsEffect(shadow);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->addWidget(container);

    QVBoxLayout *contentLayout = new QVBoxLayout(container);
    contentLayout->setSpacing(20);
    contentLayout->setContentsMargins(32, 32, 32, 28);

    QString iconEmoji = isDangerous ? "⚠️" : "❓";
    QLabel *iconLabel = new QLabel(iconEmoji, container);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 48px;");
    contentLayout->addWidget(iconLabel);

    QLabel *titleLabel = new QLabel(title, container);
    titleLabel->setStyleSheet(QString(
        "font-size: 20px; font-weight: 700; color: %1;"
    ).arg(isDangerous ? "#D32F2F" : Theme::TEXT_PRIMARY));
    titleLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(titleLabel);

    QLabel *messageLabel = new QLabel(message, container);
    messageLabel->setStyleSheet(QString(
        "font-size: 14px; color: %1; line-height: 24px;"
    ).arg(Theme::TEXT_SECONDARY));
    messageLabel->setWordWrap(true);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setMinimumHeight(60);
    contentLayout->addWidget(messageLabel);

    contentLayout->addSpacing(12);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    QPushButton *cancelBtn = new QPushButton("取消", container);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedHeight(48);
    cancelBtn->setMinimumWidth(90);
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            background: #F5F5F5;
            border: none;
            border-radius: 14px;
            color: #666666;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #EEEEEE;
        }
    )");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    QPushButton *noBtn = new QPushButton(noText, container);
    noBtn->setCursor(Qt::PointingHandCursor);
    noBtn->setFixedHeight(48);
    noBtn->setMinimumWidth(90);
    noBtn->setStyleSheet(R"(
        QPushButton {
            background: #F5F5F5;
            border: 1px solid #E0E0E0;
            border-radius: 14px;
            color: #333333;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #EEEEEE;
        }
    )");
    connect(noBtn, &QPushButton::clicked, this, [this]() {
        done(QMessageBox::No);
    });

    QPushButton *yesBtn = new QPushButton(yesText, container);
    yesBtn->setCursor(Qt::PointingHandCursor);
    yesBtn->setFixedHeight(48);
    yesBtn->setMinimumWidth(90);
    yesBtn->setDefault(true);

    if (isDangerous) {
        yesBtn->setStyleSheet(R"(
            QPushButton {
                background: #D32F2F;
                border: none;
                border-radius: 14px;
                color: white;
                font-size: 15px;
                font-weight: 600;
            }
            QPushButton:hover {
                background: #E53935;
            }
        )");
    } else {
        yesBtn->setStyleSheet(QString(R"(
            QPushButton {
                background: %1;
                border: none;
                border-radius: 14px;
                color: white;
                font-size: 15px;
                font-weight: 600;
            }
            QPushButton:hover {
                background: %2;
            }
        )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));
    }
    connect(yesBtn, &QPushButton::clicked, this, [this]() {
        done(QMessageBox::Yes);
    });

    btnLayout->addStretch();
    btnLayout->addWidget(yesBtn);
    btnLayout->addWidget(noBtn);
    btnLayout->addWidget(cancelBtn);
    btnLayout->addStretch();

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
    
    dialog.layout()->activate();
    QSize size = dialog.sizeHint();

    QScreen *screen = nullptr;
    if (parent && parent->window() && parent->window()->screen()) {
        screen = parent->window()->screen();
    } else {
        screen = QGuiApplication::primaryScreen();
    }

    if (screen) {
        QRect screenGeom = screen->availableGeometry();
        int x = screenGeom.left() + (screenGeom.width() - size.width()) / 2;
        int y = screenGeom.top() + (screenGeom.height() - size.height()) / 2;
        dialog.move(x, y);
    }

    return dialog.exec() == QDialog::Accepted;
}

QMessageBox::StandardButton ConfirmDialog::confirm3(
    QWidget* parent,
    const QString& title,
    const QString& message,
    const QString& yesText,
    const QString& noText,
    bool isDangerous
) {
    ConfirmDialog dialog(parent, title, message, yesText, noText, isDangerous);
    
    dialog.layout()->activate();
    QSize size = dialog.sizeHint();

    QScreen *screen = nullptr;
    if (parent && parent->window() && parent->window()->screen()) {
        screen = parent->window()->screen();
    } else {
        screen = QGuiApplication::primaryScreen();
    }

    if (screen) {
        QRect screenGeom = screen->availableGeometry();
        int x = screenGeom.left() + (screenGeom.width() - size.width()) / 2;
        int y = screenGeom.top() + (screenGeom.height() - size.height()) / 2;
        dialog.move(x, y);
    }

    int result = dialog.exec();
    return (QMessageBox::StandardButton)result;
}
