#include <QApplication>
#include "models/datamanager.h"
#include "mainwindow.h"
#include <QDebug>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(R"(
        QWidget {
            font-family: "Microsoft YaHei";
        }
    )");

    MainWindow w;
    
    // Initialize DataManager after MainWindow is created
    DataManager::instance();
    
    w.resize(1200, 800);
    w.show();

    return a.exec();
}