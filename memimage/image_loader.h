#pragma once

#include "utils/inst_once.h"
#include <QString>
#include <QImage>
#include <map>
#include <mutex>
#include <atomic>
#include <stdint.h>
#include <tuple>
#include <algorithm>


namespace imaging
{
    using image_buffer_ptr = std::shared_ptr<QImage>;

    //helpers to work with libexiv2
    namespace exiv2_helpers {
        using keys_t = std::vector<std::string>;
        template <class T, class R>
        void meta_value(const R& src, T& result);

        template <class T, class R>
        void meta_value(const R& src, std::string& res)
        {
            res = src.toString();
        }

        template <class T, class R>
        void meta_value(const R& src, long& res)
        {
            res = src.toLong();
        }

        template <class T, class R>
        void meta_value(const R& src, float& res)
        {
            res = src.toFloat();
        }

        template<class T, class R>
        bool getValue(const T& src, const std::string& key, R& result)
        {
            auto r = std::find_if(src.begin(), src.end(), [&key](const auto& val)
            {
                return val.key() == key;
            });
            bool res = r != src.end();
            if (res)
                meta_value<typename std::remove_reference<decltype(result)>::type>(r->value(), result);
            return res;
        }

        template<class T, class R>
        bool getValueOneOf(const T& src, const keys_t& keys, R& result)
        {
            bool res = false;
            for (const auto& s : keys)
            {
                res = getValue(src, s, result);
                if (res)
                    break;
            }
            return res;
        }
    }

    struct meta_t
    {
        meta_t();
        long  iso;
        float exposure;
        float aperture;

        QString getStringValue() const;
        void load(const QString& fileName);
    };

    class image_cacher
    {
    protected:
        friend class image_preview_loader; //need access from previews object to full object
        template<class C>
        struct image_t
        {
            C data;
            meta_t meta;
            operator bool() const
            {
                return data != nullptr;
            }
        };
        using image_buffer_wptr = std::weak_ptr<QImage>;
        using image_t_w         = image_t<image_buffer_wptr>;

        struct image_t_s : public image_t<image_buffer_ptr>
        {
            image_t_s& operator = (const image_t_w& c)
            {
                meta = c.meta;
                data = c.data.lock();
                return *this;
            }
            explicit operator image_t_w() const
            {
                image_t_w tmp;
                tmp.data = data;
                tmp.meta = meta;
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
        std::atomic<size_t> lastSize;
        std::recursive_mutex mutex;
        bool assumeMirrored;
    protected:
        virtual image_t_s createImage(const QString& key) const = 0;
        void    findImage(const QString& key, image_t_s &res);
    public:
        image_cacher();
        image_buffer_ptr getImage(const QString& fileName);
        meta_t           getExif(const QString& fileName);

        void gc(bool no_wait = false);
        void wipe();
        size_t getMemoryUsed() const;
        //ok, maybe mirroring on loading is not really good idea for scripting, but .. at least images in ram are already properly loaded = time
        void setNewtoneTelescope(bool isNewtone); //if true, applies mirror transformation to all images on load
        bool isNewtoneTelescope() const;
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
