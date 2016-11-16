#pragma once
#include <QString>
#include <QImage>
#include <mutex>
#include "memimage/image_loader.h"

class PreviewsModelData
{
private:
    QImage  preview;
public:
    QString filePath;

    bool    selected;
    PreviewsModelData(const QString& path):
        preview(),
        filePath(path),
        selected(false)
    {
    }

    bool loadPreview(std::recursive_mutex& mut)
    {
        bool res = preview.isNull();
        if (res)
        {
            auto src = IMAGE_LOADER.getImage(filePath);
            if (!src->isNull())
            {
                std::lock_guard<decltype (mut)> grd(mut);
                preview = src->scaled(300, 300, Qt::KeepAspectRatio);
            }
        }
        return res;
    }

    const QImage& getPreview() const
    {
        return preview;
    }
};
