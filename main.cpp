#include "mainwindow.h"
#include "logic/theapi.h"

#include <QApplication>
#include <QVector>
#include <QDebug>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    QApplication a(argc, argv);
    THEAPI; //ensuring global is created
    a.setStyle(QStyleFactory::create("Plastique"));

//    QFile style(":/styles/darkorange");
//    style.open(QIODevice::ReadOnly | QFile::Text);
//    a.setStyleSheet(style.readAll());

    a.setApplicationName("AstroEd");
    a.setApplicationVersion("0.1");
    a.setApplicationDisplayName("AstroEd");
    a.setOrganizationDomain("pasteover.net");
    a.setOrganizationName("Oleksiy Zakharov");


    MainWindow w;
    w.show();

    return a.exec();
}
