#include "pageanimator.h"

#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>

bool PageAnimator::isAnimating = false;

void PageAnimator::slideToPage(QStackedWidget *stack, QWidget *targetPage)
{
    if (!stack || !targetPage || isAnimating) {
        return;
    }

    QWidget *currentPage = stack->currentWidget();
    if (currentPage == targetPage) {
        return;
    }

    isAnimating = true;

    int width = stack->width();
    int height = stack->height();

    bool forward = stack->indexOf(targetPage) > stack->indexOf(currentPage);

    QWidget *oldPage = currentPage;
    QWidget *newPage = targetPage;

    oldPage->setGeometry(0, 0, width, height);
    newPage->setGeometry(forward ? width : -width, 0, width, height);
    newPage->setVisible(true);

    QPropertyAnimation *slideOut = new QPropertyAnimation(oldPage, "pos", oldPage);
    slideOut->setDuration(250);
    slideOut->setEasingCurve(QEasingCurve::InOutQuad);
    slideOut->setStartValue(QPoint(0, 0));
    slideOut->setEndValue(QPoint(forward ? -width : width, 0));

    QPropertyAnimation *slideIn = new QPropertyAnimation(newPage, "pos", newPage);
    slideIn->setDuration(250);
    slideIn->setEasingCurve(QEasingCurve::InOutQuad);
    slideIn->setStartValue(QPoint(forward ? width : -width, 0));
    slideIn->setEndValue(QPoint(0, 0));

    QParallelAnimationGroup *group = new QParallelAnimationGroup(oldPage);
    group->addAnimation(slideOut);
    group->addAnimation(slideIn);

    QObject::connect(group, &QParallelAnimationGroup::finished, stack, [oldPage, newPage, width, height, stack, slideOut, slideIn, group]() {
        oldPage->setVisible(false);
        oldPage->setGeometry(0, 0, width, height);
        stack->setCurrentWidget(newPage);
        newPage->setGeometry(0, 0, width, height);
        isAnimating = false;
        group->deleteLater();
        slideOut->deleteLater();
        slideIn->deleteLater();
    });

    group->start();
}

void PageAnimator::slideToIndex(QStackedWidget *stack, int targetIndex)
{
    if (!stack || targetIndex < 0 || targetIndex >= stack->count() || isAnimating) {
        return;
    }

    QWidget *targetPage = stack->widget(targetIndex);
    slideToPage(stack, targetPage);
}