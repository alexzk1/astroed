#pragma once

#include "utils/inst_once.h"
#include <QString>
#include <QImage>
#include <map>
#include <mutex>
#include <atomic>
#include <stdint.h>
#include <tuple>

namespace imaging
{
    using image_buffer_ptr = std::shared_ptr<QImage>;
    using exif_t           = QString; //must be copyable

    class image_cacher
    {
    protected:
        friend class image_preview_loader; //need access from previews object to full object
        template<class C>
        struct image_t
        {
            C data;
            exif_t exif;
        };
        using image_buffer_wptr = std::weak_ptr<QImage>;
        using image_t_w         = image_t<image_buffer_wptr>;

        struct image_t_s : public image_t<image_buffer_ptr>
        {
            image_t_s& operator = (const image_t_w& c)
            {
                exif = c.exif;
                data = c.data.lock();
                return *this;
            }
            explicit operator image_t_w() const
            {
                image_t_w tmp;
                tmp.data = data;
                tmp.exif = exif;
                return tmp;
            }
        };

    private:

        using time_ptr_t = std::pair<int64_t, image_t_s>;
        using ptrs_t     = std::map<QString,  time_ptr_t>;


        using size_wptr_t = std::pair<size_t, image_t_w>;
        using weaks_t = std::map<QString, size_wptr_t>;

        ptrs_t  cache;
        weaks_t wcache;
        std::recursive_mutex mutex;
        std::atomic<size_t> lastSize;
    protected:
        virtual image_t_s createImage(const QString& key) const = 0;
        image_t_s         findImage(const QString& key);
    public:
        image_cacher();
        image_buffer_ptr getImage(const QString& fileName);
        exif_t           getExif(const QString& fileName);

        void gc(bool no_wait = false);

        size_t getMemoryUsed() const;
        virtual ~image_cacher();
    };

    class image_loader : public utility::ItCanBeOnlyOne<image_loader>, public image_cacher
    {
    protected:
        virtual image_t_s createImage(const QString& key) const override;
    public:
        image_loader() = default;
    };

    class image_preview_loader : public utility::ItCanBeOnlyOne<image_preview_loader>, public image_cacher
    {
    protected:
        virtual image_t_s createImage(const QString& key) const override;
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
