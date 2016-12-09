#include "theapi.h"
#include "../mainwindow.h"
#include <QPixmap>

TheAPI::TheAPI()
{
    connect(this, &TheAPI::showPreviewImage, this, [](const imaging::image_buffer_ptr img, bool reset) //need to have shared_ptr copy, so it will not be gc'ed
    {
        auto& lbl = MainWindow::instance()->openPreviewTab(img->size());

        //all this trick with signal/slot was for this line - pixmaps can be created ONLY in GUI thread
        lbl.setPixmap(QPixmap::fromImage(*img));
        if (reset)
            MainWindow::instance()->resetPreview();

    }, Qt::QueuedConnection);
}

TheAPI::~TheAPI()
{

}

void TheAPI::showPreview(const QString &fileName, bool need_reset)
{
    emit showPreviewImage(IMAGE_LOADER.getImage(fileName), need_reset);
}

void TheAPI::showStatusHint(const QString &hint, int delay)
{
    MainWindow::instance()->showTempNotify(hint, delay);
}
