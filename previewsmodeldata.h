#pragma once
#include <QString>
#include <QImage>
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
        loadPreview();
    }

    void loadPreview()
    {
        auto src = IMAGE_LOADER.getImage(filePath);
        if (!src->isNull())
        {
            preview = src->scaled(500, 500, Qt::KeepAspectRatio);
        }
    }

    const QImage& getPreview() const
    {
        return preview;
    }
};
