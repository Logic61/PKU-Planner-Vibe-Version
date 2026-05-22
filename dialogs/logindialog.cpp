#include "logindialog.h"
#include "../ui/theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFrame>
#include <QGraphicsDropShadowEffect>


LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setFixedWidth(400);

    QFrame *container = new QFrame(this);
    container->setStyleSheet(R"(
        QFrame {
            background: white;
            border-radius: 20px;
        }
    )");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(40);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(0, 0, 0, 50));
    container->setGraphicsEffect(shadow);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->addWidget(container);

    QVBoxLayout *main = new QVBoxLayout(container);
    main->setSpacing(16);
    main->setContentsMargins(28, 28, 28, 24);

    QLabel *titleLabel = new QLabel("教学网登录", container);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString(
        "font-size: 20px; font-weight: 700; color: %1;"
    ).arg(Theme::PRIMARY));
    main->addWidget(titleLabel);

    main->addSpacing(4);

    QLabel *ul = new QLabel("用户名", container);
    ul->setStyleSheet(QString("font-size: 13px; font-weight: 500; color: %1;").arg(Theme::TEXT_SECONDARY));
    main->addWidget(ul);
    usernameEdit = new QLineEdit(container);
    usernameEdit->setPlaceholderText("请输入用户名");
    main->addWidget(usernameEdit);

    QLabel *pl = new QLabel("密码", container);
    pl->setStyleSheet(QString("font-size: 13px; font-weight: 500; color: %1;").arg(Theme::TEXT_SECONDARY));
    main->addWidget(pl);
    passwordEdit = new QLineEdit(container);
    passwordEdit->setPlaceholderText("请输入密码");
    passwordEdit->setEchoMode(QLineEdit::Password);
    main->addWidget(passwordEdit);

    otpLabel = new QLabel("手机令牌 (OTP，可选)", container);
    otpLabel->setStyleSheet(QString("font-size: 13px; font-weight: 500; color: %1;").arg(Theme::TEXT_SECONDARY));
    main->addWidget(otpLabel);
    otpEdit = new QLineEdit(container);
    otpEdit->setPlaceholderText("如需双重认证请输入动态码");
    main->addWidget(otpEdit);

    rememberCheck = new QCheckBox("记住凭证", container);
    main->addWidget(rememberCheck);

    main->addSpacing(8);

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->setSpacing(12);

    QPushButton *cancel = new QPushButton("取消", container);
    cancel->setCursor(Qt::PointingHandCursor);
    cancel->setFixedHeight(44);
    cancel->setMinimumWidth(100);

    QPushButton *ok = new QPushButton("登录", container);
    ok->setCursor(Qt::PointingHandCursor);
    ok->setFixedHeight(44);
    ok->setMinimumWidth(100);
    ok->setDefault(true);

    btnRow->addStretch();
    btnRow->addWidget(cancel);
    btnRow->addWidget(ok);
    btnRow->addStretch();
    main->addLayout(btnRow);

    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    setStyleSheet(QString(R"(
        QLineEdit {
            border: 1px solid #E8DADA;
            border-radius: 10px;
            padding: 10px 12px;
            background: #FEFEFE;
            color: %1;
            font-size: 13px;
        }
        QLineEdit:focus {
            border: 2px solid %2;
            background: white;
        }
        QLineEdit:disabled {
            background: #F5F5F5;
            color: #999;
        }
        QCheckBox {
            color: %3;
            font-size: 13px;
            padding: 4px 0;
        }
        QCheckBox::indicator {
            width: 20px;
            height: 20px;
            border-radius: 4px;
            border: 2px solid #E8DADA;
            background: white;
        }
        QCheckBox::indicator:checked {
            background: %2;
            border: 2px solid %2;
        }
        QPushButton {
            font-size: 14px;
            font-weight: 600;
            border-radius: 12px;
            padding: 10px 24px;
        }
    )").arg(Theme::TEXT_PRIMARY).arg(Theme::PRIMARY).arg(Theme::TEXT_SECONDARY));

    cancel->setStyleSheet(R"(
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

    ok->setStyleSheet(QString(R"(
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
            background: #6A1520;
        }
    )").arg(Theme::PRIMARY).arg(Theme::PRIMARY_DARK));

    // by default hide OTP (user can enable when needed)
    setOtpVisible(false);
}

void LoginDialog::setUsername(const QString &u) { usernameEdit->setText(u); }
void LoginDialog::setPassword(const QString &p) { passwordEdit->setText(p); }
QString LoginDialog::username() const { return usernameEdit->text().trimmed(); }
QString LoginDialog::password() const { return passwordEdit->text(); }
QString LoginDialog::otp() const { return otpEdit->text().trimmed(); }
bool LoginDialog::remember() const { return rememberCheck->isChecked(); }
void LoginDialog::setOtpVisible(bool visible) {
    otpEdit->setVisible(visible);
    otpLabel->setVisible(visible);
}
