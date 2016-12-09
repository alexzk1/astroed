#include "theapi.h"
#include "../mainwindow.h"
#include <QPixmap>

TheAPI::TheAPI()
{
    connect(this, &TheAPI::showPreviewImage, this, [](const imaging::image_buffer_ptr img) //need to have shared_ptr copy, so it will not be gc'ed
    {
        auto& lbl = MainWindow::instance()->openPreviewTab(img->size());

        //all this trick with signal/slot was for this line - pixmaps can be created ONLY in GUI thread
        lbl.setPixmap(QPixmap::fromImage(*img));

    }, Qt::QueuedConnection);
}

TheAPI::~TheAPI()
{

}

void TheAPI::showPreview(const QString &fileName)
{
    emit showPreviewImage(IMAGE_LOADER.getImage(fileName));
}
