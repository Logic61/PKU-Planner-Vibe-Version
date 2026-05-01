#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(R"(
        QWidget {
            font-family: "Microsoft YaHei";
        }
    )");

    MainWindow w;
    w.resize(1200, 800);
    w.show();

    return a.exec();
}