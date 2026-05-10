#ifndef ONBOARDINGDIALOG_H
#define ONBOARDINGDIALOG_H

#include <QDialog>
#include <QMouseEvent>

class OnboardingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OnboardingDialog(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupUI();
};

#endif