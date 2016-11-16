#pragma once
#include <QString>
#include <QImage>
#include <mutex>
#include "memimage/image_loader.h"

class PreviewsModelData
{
private:
    imaging::image_buffer_ptr preview;
public:
    QString filePath;

    bool    selected;
    PreviewsModelData(const QString& path):
        preview(new QImage()),
        filePath(path),
        selected(false)
    {
    }

    bool loadPreview()
    {
        bool res = preview->isNull();
        if (res)
        {
            preview = PREVIEW_LOADER.getImage(filePath);
        }
        return res;
    }

    const QImage& getPreview() const
    {
        return *preview;
    }
};
