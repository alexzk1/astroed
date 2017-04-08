#include "mainwindow.h"

#include <QApplication>
#include <QVector>
#include <QDebug>
#include <QWindow>
#include <QStyleFactory>
#include <QObject>
#include <QMessageBox>
#include "singleapp/singleapplication.h"

static QMessageBox* noMemoryBox = nullptr;
[[ noreturn ]] static void no_memory ()
{
  std::cerr << "Failed to allocate memory!\n";
  if (noMemoryBox)
      noMemoryBox->exec();
  std::exit (1);
}


int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    SingleApplication a(argc, argv, false, SingleApplication::Mode::SecondaryNotification | SingleApplication::Mode::User);

    noMemoryBox = new QMessageBox(QMessageBox::Critical, "AstroEd", "AstroEd failed to allocate memory.\nPlease close some applications.");
    std::set_new_handler(no_memory);

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
    int r = a.exec();

    if (noMemoryBox)
        delete noMemoryBox;

    return r;
}
