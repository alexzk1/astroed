#include "theapi.h"
#include "../mainwindow.h"
#include <QPixmap>

TheAPI::TheAPI()
{
    connect(this, &TheAPI::showPreview, this, [](const QImage* img)
    {
        auto lbl = MainWindow::instance()->openPreviewTab();
        if (lbl)
        {
            //all this trick with signal/slot was for this line - pixmaps can be created ONLY in GUI thread
            lbl->setPixmap(QPixmap::fromImage(*img));
        }
    }, Qt::QueuedConnection);
}

TheAPI::~TheAPI()
{

}
