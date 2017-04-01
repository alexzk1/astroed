#pragma once
#include <QString>
#include <QImage>
#include <QVariant>
#include <mutex>
#include <map>
#include "memimage/image_loader.h"
#include "lua/variant_convert.h"

class PreviewsModelData
{
private:
    imaging::image_buffer_ptr preview;
public:
    QString  filePath;
    std::map<int, QVariant>          valuesPerColumn;
    bool brokenPreview;
    bool wasLoaded;

    PreviewsModelData(const QString& path):
        preview(nullptr),
        filePath(path),
        valuesPerColumn(),
        brokenPreview(false),
        wasLoaded(false)
    {
    }

    bool loadPreview() //will be called from thread which iterates over files and does loading (so no hard pressure on HDD)
    {
        bool res = preview == nullptr || preview->isNull();
        if (res)
        {
            preview = PREVIEW_LOADER.getImage(filePath);
            brokenPreview = preview == nullptr || preview->isNull();
            wasLoaded = !brokenPreview;
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
        const static QImage empty;
        if (!preview)
            return empty;
        return *preview;
    }

    template<class T>
    T getValue(int column, const T& def) const
    {
        if (!valuesPerColumn.count(column))
            return def;
        return luavm::variantTo<T>(valuesPerColumn.at(column));
    }
};
