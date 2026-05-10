#include "onboardingdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QMouseEvent>

OnboardingDialog::OnboardingDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(false);
    resize(400, 280);
    setStyleSheet(R"(
        QDialog {
            background: #F5F5F5;
        }
        QPushButton {
            border: none;
            border-radius: 8px;
            padding: 12px 24px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton#okBtn {
            background: #8B1E2D;
            color: white;
        }
        QPushButton#okBtn:hover {
            background: #A02030;
        }
    )");

    setupUI();
}

void OnboardingDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QFrame *card = new QFrame;
    card->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 16px;
            border: 1px solid #EAEAEA;
        }
    )");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    cardLayout->setSpacing(16);

    QLabel *iconLabel = new QLabel("📚");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 48px;");

    QLabel *titleLabel = new QLabel("暂无课程");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: 600; color: #333;");

    QLabel *tipLabel = new QLabel("在课程表空白处\n右键编辑课程 / 双击编辑课程");
    tipLabel->setAlignment(Qt::AlignCenter);
    tipLabel->setStyleSheet("font-size: 14px; color: #666; line-height: 1.8;");
    tipLabel->setWordWrap(true);

    QPushButton *okBtn = new QPushButton("我知道了");
    okBtn->setObjectName("okBtn");
    okBtn->setCursor(Qt::PointingHandCursor);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);

    cardLayout->addWidget(iconLabel);
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(tipLabel);
    cardLayout->addWidget(okBtn);

    mainLayout->addWidget(card);
}

void OnboardingDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        accept();
    }
    QDialog::mousePressEvent(event);
}