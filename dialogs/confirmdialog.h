#ifndef CONFIRMDIALOG_H
#define CONFIRMDIALOG_H

#include <QDialog>
#include <QString>
#include <QMessageBox>

class ConfirmDialog : public QDialog
{
    Q_OBJECT
public:
    static bool confirm(
        QWidget* parent,
        const QString& title,
        const QString& message,
        const QString& confirmText = "确认",
        bool isDangerous = false
    );

    static QMessageBox::StandardButton confirm3(
        QWidget* parent,
        const QString& title,
        const QString& message,
        const QString& yesText = "是",
        const QString& noText = "否",
        bool isDangerous = false
    );

private:
    explicit ConfirmDialog(
        QWidget* parent,
        const QString& title,
        const QString& message,
        const QString& confirmText,
        bool isDangerous
    );

    explicit ConfirmDialog(
        QWidget* parent,
        const QString& title,
        const QString& message,
        const QString& yesText,
        const QString& noText,
        bool isDangerous
    );
};

#endif