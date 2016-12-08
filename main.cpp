#include "mainwindow.h"
#include "logic/theapi.h"

#include <QApplication>
#include <QVector>
#include <QDebug>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    QApplication a(argc, argv);
    THEAPI; //ensuring global is created

    a.setApplicationName("AstroEd");
    a.setApplicationVersion("0.1");
    a.setApplicationDisplayName("AstroEd");
    a.setOrganizationDomain("pasteover.net");
    a.setOrganizationName("Oleksiy Zakharov");


    MainWindow w;
    w.show();

    return a.exec();
}
