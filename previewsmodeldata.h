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

    bool loadPreview() //will be called from thread which iterates over files and does loading (so no hard pressure on HDD)
    {
        bool res = preview->isNull();
        if (res)
        {
            preview = PREVIEW_LOADER.getImage(filePath);
        }
        return res;
    }

    QString getFilePath() const
    {
        return filePath;
    }

    QString getPreviewInfo() const
    {
        return PREVIEW_LOADER.getExif(filePath).getStringValue();
    }

    const QImage& getPreview() const
    {
        return *preview;
    }
};
