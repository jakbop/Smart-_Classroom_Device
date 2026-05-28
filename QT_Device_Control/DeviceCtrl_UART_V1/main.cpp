#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{

    //这是V1版本
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
