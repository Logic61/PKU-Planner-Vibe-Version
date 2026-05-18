#include "logindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("教学网登录");
    setModal(true);

    QVBoxLayout *main = new QVBoxLayout(this);

    QHBoxLayout *urow = new QHBoxLayout;
    QLabel *ul = new QLabel("用户名:");
    usernameEdit = new QLineEdit;
    urow->addWidget(ul);
    urow->addWidget(usernameEdit);
    main->addLayout(urow);

    QHBoxLayout *prow = new QHBoxLayout;
    QLabel *pl = new QLabel("密码:");
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    prow->addWidget(pl);
    prow->addWidget(passwordEdit);
    main->addLayout(prow);

    QHBoxLayout *orow = new QHBoxLayout;
    QLabel *ol = new QLabel("手机令牌(OTP，可选):");
    otpEdit = new QLineEdit;
    otpEdit->setEchoMode(QLineEdit::Normal);
    orow->addWidget(ol);
    orow->addWidget(otpEdit);
    main->addLayout(orow);

    rememberCheck = new QCheckBox("记住凭证");
    main->addWidget(rememberCheck);

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    QPushButton *ok = new QPushButton("登录");
    QPushButton *cancel = new QPushButton("取消");
    btnRow->addWidget(ok);
    btnRow->addWidget(cancel);
    main->addLayout(btnRow);

    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    // by default hide OTP (user can enable when needed)
    setOtpVisible(false);
}

void LoginDialog::setUsername(const QString &u) { usernameEdit->setText(u); }
void LoginDialog::setPassword(const QString &p) { passwordEdit->setText(p); }
QString LoginDialog::username() const { return usernameEdit->text().trimmed(); }
QString LoginDialog::password() const { return passwordEdit->text(); }
QString LoginDialog::otp() const { return otpEdit->text().trimmed(); }
bool LoginDialog::remember() const { return rememberCheck->isChecked(); }
void LoginDialog::setOtpVisible(bool visible) { otpEdit->setVisible(visible); }
