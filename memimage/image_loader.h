#pragma once

#include "utils/inst_once.h"

#include <QString>
#include <QImage>
#include <QTemporaryFile>

#include <type_traits>

#include <map>
#include <mutex>
#include <atomic>
#include <stdint.h>
#include <tuple>
#include <algorithm>
#include <exiv2/exiv2.hpp>
#include <stdint.h>
#include "opencv_vid.h"

//videofs is expexted to be in form: videofs://file_path.mov?frame_number
#define VFS_PREFIX "videofs"
#define VFS_CACHED_FOLDER ".cached_frames"

namespace imaging
{
    using image_buffer_ptr  = std::shared_ptr<QImage>;
    using image_buffer_wptr = std::weak_ptr<QImage>;

    //helpers to work with libexiv2
    namespace exiv2_helpers
    {
        using RationalArray  = std::vector<Exiv2::Rational>;
        using URationalArray = std::vector<Exiv2::URational>;

        template <typename T>       struct isvector: std::false_type {};
        template <typename... Args> struct isvector<std::vector<Args...>>: std::true_type {};

        template <typename T>       struct ispair: std::false_type {};
        template <typename... Args> struct ispair<std::pair<Args...>>: std::true_type {};

        namespace exiv_rationals
        {
            template <class T>
            typename std::enable_if<ispair<T>::value, double>::type toDouble(const T& v)
            {
                if (v.second == 0)
                    return 0; //resolving x/0 as 0

                return v.first / v.second;
            }

            template <class T>
            typename std::enable_if<ispair<T>::value, T>::type div(const T& up, const T& down) // = up/down
            {
                T res;
                res.first  = up.first * down.second;
                res.second = up.second * down.first;
                return res;
            }

            //next dumb functions allow to change variable declaration (double/rational) and keep all other code the same
            inline double toDouble(double v)
            {
                return v;
            }


            template <class T>
            typename std::enable_if<ispair<T>::value, const T&>::type varInit()
            {
                const static T def(0, 1); //rationals default init
                return def;
            }

            template <class T>
            typename std::enable_if < std::is_integral<T>::value || std::is_floating_point<T>::value, T >::type varInit()
            {
                return 0;
            }
        }
        using keys_t = std::vector<std::string>;
        template <class T, class R>
        void meta_value(const R& src, T& result, long index);

        template <class T, class R>
        void meta_value(const R& src, std::string& res, long index = 0)
        {
            res = src.toString(index);
        }

        template <class T, class R>
        void meta_value(const R& src, long& res, long index = 0)
        {
            res = src.toLong(index);
        }

        template <class T, class R>
        void meta_value(const R& src, int16_t& res, long index = 0)
        {
            res = 0xFFFF & src.toLong(index);
        }

        template <class T, class R>
        void meta_value(const R& src, uint16_t& res, long index = 0)
        {
            res = 0xFFFF & src.toLong(index);
        }

        template <class T, class R>
        void meta_value(const R& src, float& res, long index = 0)
        {
            res = src.toFloat(index);
        }

        template <class T, class R>
        void meta_value(const R& src, double& res, long index = 0)
        {
            res = static_cast<double>(src.toFloat(index));
        }

        template <class T, class R>
        void meta_value(const R& src, Exiv2::Rational& res, long index = 0)
        {
            res = src.toRational(index);
        }

        template <class T, class R>
        void meta_value(const R& src, Exiv2::URational& res, long index = 0)
        {
            auto tmp   = src.toRational(index);
            res.first  = static_cast<Exiv2::URational::first_type>(tmp.first);
            res.second = static_cast<Exiv2::URational::second_type>(tmp.second);
        }

        template <class T>
        typename std::enable_if<std::is_fundamental<T>::value, size_t>::type getTypeSize()
        {
            return sizeof (T);
        }

        template <class T>
        typename std::enable_if<ispair<T>::value, size_t>::type getTypeSize()
        {
            return sizeof (typename T::first_type) + sizeof (typename T::second_type);
        }

        template <class Vec, class R, class T = typename std::enable_if<isvector<Vec>::value, typename Vec::value_type>::type>
        void meta_value(const R& src, Vec& res)
        {
            const long sz = src.count();
            const size_t typesz = getTypeSize<T>();
            const size_t srcsz  = static_cast<decltype(srcsz)>(src.size() / sz);
            if (typesz != srcsz)
                throw std::runtime_error("Invalid type compiled for the field stored.");

            res.reserve(sz);
            for (long i = 0; i < sz; ++i)
            {
                T tmp;
                meta_value<T>(src, tmp, i);
                res.push_back(tmp);
            }
        }

        template<class T, class R>
        bool getValue(const T& src, const std::string& key, R& result)
        {
            auto r = std::find_if(src.begin(), src.end(), [&key](const auto & val)
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
        bool wasLoaded;
        long  iso;
        double exposure;
        Exiv2::URational aperture;
        double optZoom;
        QString getStringValue() const;

        struct precalcs_t
        {
            double blureness{-1};
        } precalcs;

        void loadExif(const QString& fileName);
        void precalculate(const image_buffer_ptr &img);
    };

    class image_cacher
    {
    protected:
        friend class image_preview_loader; //need access from previews object to full object
        template<class Ptr>
        struct image_t
        {
            Ptr data;
            meta_t meta;
#ifdef USING_VIDEO_FS
            //we want to keep same loader for all frames in same file
            std::shared_ptr<QTemporaryFile> tempFile;
#endif
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

            image_t_s& operator = (image_t_w&& c)
            {
                meta = std::move(c.meta);
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


        using size_wptr_t = image_t_w;
        using weaks_t = std::map<QString, size_wptr_t>;


        ptrs_t  cache;
        weaks_t wcache;
        bool assumeMirrored;

        void clear_cacher();
    protected:
        mutable std::recursive_mutex mutex;
        virtual image_t_s createImage(const QString& key) const = 0;
        void    findImage(const QString& key, image_t_s &res);

        bool isUsingCached() const;
        QString cachedFileName(const QUrl& url) const;
    public:
        image_cacher();
        image_buffer_ptr getImage(const QString& fileName);
        meta_t           getMeta(const QString& fileName);
        bool             isLoaded(const QString& fileName) const;

        virtual void gc(bool no_wait = false);
        virtual void wipe();
        static size_t getMemoryUsed();
        //ok, maybe mirroring on loading is not really good idea for scripting, but .. at least images in ram are already properly loaded = time
        void setNewtoneTelescope(bool isNewtone); //if true, applies mirror transformation to all images on load
        bool isNewtoneTelescope() const;
        virtual ~image_cacher();

        static bool isProperVfs(const QUrl& url);
    };

    class image_loader : public utility::ItCanBeOnlyOne<image_loader>, public image_cacher
    {
    protected:
#ifdef USING_VIDEO_FS
        using VideoPair = std::pair<QString, VideoFileReadPtr>;
        mutable VideoPair frameLoaders; //now we have video = folder, so should not keep many videos loaded at once
        std::atomic<bool> dctor;
        VideoFileReadPtr getVideoCapturer(const QString &filePath) const;
        void closeVideoCapturer() const;
#endif
        virtual image_t_s createImage(const QString& key) const override;
        void clear_loader();
    public:
        image_loader() = default;
        virtual void wipe()override;

        //can be used to generate list of all possible frames inside video
        QStringList getVideoFramesLinks(const QString& videoFileName);

        //makes temporary file out of frame if needed
        QString getFileLinkForExternalTools(const QString& originalLink) const;
        virtual ~image_loader() final;
    };

    class image_preview_loader : public utility::ItCanBeOnlyOne<image_preview_loader>, public image_cacher
    {
    protected:
        virtual image_t_s createImage(const QString& key) const override;
    public:
        image_preview_loader() = default;
        virtual ~image_preview_loader() final = default;
    };
}

#ifndef IMAGE_LOADER
    #define IMAGE_LOADER utility::globalInstance<imaging::image_loader>()
#endif

#ifndef PREVIEW_LOADER
    #define PREVIEW_LOADER utility::globalInstance<imaging::image_preview_loader>()
#endif
