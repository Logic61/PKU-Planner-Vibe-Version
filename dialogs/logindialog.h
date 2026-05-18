#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QCheckBox;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

    void setUsername(const QString &u);
    void setPassword(const QString &p);
    QString username() const;
    QString password() const;
    QString otp() const;
    bool remember() const;
    void setOtpVisible(bool visible);

private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLineEdit *otpEdit;
    QCheckBox *rememberCheck;
};

#endif // LOGINDIALOG_H
