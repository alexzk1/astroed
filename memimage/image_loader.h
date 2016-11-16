#pragma once

#include "utils/inst_once.h"
#include <QString>
#include <QImage>
#include <map>
#include <mutex>
#include <atomic>
#include <stdint.h>


namespace imaging
{
    using image_buffer_ptr = std::shared_ptr<QImage>;
    class image_cacher //: public utility::ItCanBeOnlyOne<image_loader>
    {
    private:
        using image_buffer_wptr = std::weak_ptr<QImage>;

        using time_ptr_t = std::pair<int64_t, image_buffer_ptr>;
        using ptrs_t     = std::map<QString, time_ptr_t>;


        using size_wptr_t = std::pair<size_t, image_buffer_wptr>;
        using weaks_t = std::map<QString, size_wptr_t>;

        ptrs_t  cache;
        weaks_t wcache;
        std::recursive_mutex mutex;
        std::atomic<size_t> lastSize;
    protected:
        virtual image_buffer_ptr createImage(const QString& key) const = 0;
    public:
        image_cacher();
        image_buffer_ptr getImage(const QString& fileName);
        void gc(bool no_wait = false);

        size_t getMemoryUsed() const;
        virtual ~image_cacher();
    };

    class image_loader : public utility::ItCanBeOnlyOne<image_loader>, public image_cacher
    {
    protected:
        virtual image_buffer_ptr createImage(const QString& key) const override;
    public:
        image_loader() = default;
    };

    class image_preview_loader : public utility::ItCanBeOnlyOne<image_preview_loader>, public image_cacher
    {
    protected:
        virtual image_buffer_ptr createImage(const QString& key) const override;
    public:
        image_preview_loader() = default;
    };
}

#ifndef IMAGE_LOADER
#define IMAGE_LOADER utility::globalInstance<imaging::image_loader>()
#endif

#ifndef PREVIEW_LOADER
#define PREVIEW_LOADER utility::globalInstance<imaging::image_preview_loader>()
#endif
