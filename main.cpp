#include "mainwindow.h"
#include "logic/theapi.h"

#include <QApplication>
#include <QVector>
#include <QDebug>
#include <QWindow>
#include <QStyleFactory>
#include <QObject>
#include "singleapp/singleapplication.h"

int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    SingleApplication a(argc, argv, false, SingleApplication::Mode::SecondaryNotification | SingleApplication::Mode::User);

    THEAPI; //ensuring global is created

    a.setApplicationName("AstroEd");
    a.setApplicationVersion("0.1");
    a.setApplicationDisplayName("AstroEd");
    a.setOrganizationDomain("pasteover.net");
    a.setOrganizationName("pasteover.net");


    MainWindow w;
    QObject::connect(&a, &SingleApplication::instanceStarted, [&w, &a]()
    {
        //actual popup of window will be dependent on desktop settings, for example in kde it is something like "bring to front demanding attention"
        if (a.activeWindow())
            a.activeWindow()->raise();
        else
            w.raise();
    });

    w.show();

    return a.exec();
}
