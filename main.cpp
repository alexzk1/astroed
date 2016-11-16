#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("AstroEd");
    a.setApplicationVersion("0.1");
    a.setApplicationDisplayName("AstroEd");
    a.setOrganizationDomain("pasteover.net");
    a.setOrganizationName("Oleksiy Zakharov");

    MainWindow w;
    w.show();

    return a.exec();
}
