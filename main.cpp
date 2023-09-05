#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName("IPbus packet Constructor");
    QCoreApplication::setApplicationVersion("1.2.2");
    MainWindow w;
    w.show();
    return a.exec();
}
