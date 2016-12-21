#include "image_loader.h"
#include <QImage>
#include <QDebug>

#include "utils/erase_if.h"
#include <chrono>
#include <vector>
#include <exiv2/exiv2.hpp>
#include <exiv2/exif.hpp>

using namespace imaging;

extern size_t getMemorySize();
const static size_t sysMemory   = getMemorySize();
const static size_t maxMemUsage = sysMemory / 3; //mem pressure until it will start "gc" loops
const static auto& dumb = IMAGE_LOADER; //ensuring single instance is created on program init

const static int64_t longestDelay = 240; //how long at most image will remain cached since last access
const static int     previewSize  = 300; //recommended size

static int64_t nows()
{
    using namespace std::chrono;
    return duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
};

image_cacher::image_cacher():
    cache(),
    wcache(),
    mutex(),
    lastSize(0)
{
    qRegisterMetaType<image_buffer_ptr>("image_buffer_ptr");
    qRegisterMetaType<imaging::image_buffer_ptr>("imaging::image_buffer_ptr");
}

image_buffer_ptr image_cacher::getImage(const QString &fileName)
{
    image_t_s t;
    findImage(fileName, t);
    return t.data;
}

meta_t image_cacher::getExif(const QString &fileName)
{
    image_t_s t;
    findImage(fileName, t);
    return t.meta;
}

void image_cacher::findImage(const QString& key, image_t_s& res)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    if (wcache.count(key))
    {
        //trying to restore pointer to somewhere existing image (it is possible somebody else holds shared_ptr copy)
        res = wcache.at(key).second;
    }

    if (!res)
    {
        //the image is no longer exists in memory. Loading from disk.
        res = createImage(key);
        auto sz = static_cast<size_t>(res.data->byteCount());
        //idea is to keep images hard locked in RAM until we have sufficient memory, then we unlock it
        //but it will still be in RAM until processing algorithms use it

        wcache[key] = std::make_pair(sz, static_cast<image_t_w>(res));
        lastSize += sz;
    }

    cache[key] = std::make_pair(nows(), res); //even if object exists in cache, just updating time
    gc();
}

void image_cacher::gc(bool no_wait)
{
    //kinda "gc" loop - removing weak pointers which were deleted already
    if (lastSize > maxMemUsage)
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        //1 step, removing too old hard links to images (pretty dumb if here)
        auto t = nows();
        utility::erase_if(cache, [this, t, no_wait](const auto& sp)
        {
            auto delay = t - sp.second.first;
            return (sp.second.second.data.unique() && (no_wait || delay > 20)) || delay > longestDelay;
        });

        //2 step, if no hard links left anywhere - clearing weaks too, meaning memory is freed already
        utility::erase_if(wcache, [this](const auto& sp)
        {
            bool r = sp.second.second.data.expired();
            if (r)
                lastSize -= sp.second.first;
            return r;
        });
    }
}

size_t image_cacher::getMemoryUsed() const
{
    return lastSize;
}

image_cacher::~image_cacher()
{
}

//todo: need some kinda of "virtual filesystem" to dig into avi/mp4 and pick frames there
image_cacher::image_t_s image_loader::createImage(const QString &key) const
{
    image_t_s tmp;
    { //ensuring img will close file "key"
        QImage img(key);
        tmp.data  = std::make_shared<QImage>();
        *tmp.data = img.convertToFormat(QImage::Format_RGB888);
    }

    //done: this should load exif too
    {
        tmp.meta.load(key);
    }
    return tmp;
}

image_cacher::image_t_s image_preview_loader::createImage(const QString &key) const
{
    //this class caches previews, not full objects, so we access OTHER object with full images
    image_t_s src;
    IMAGE_LOADER.findImage(key, src);
    //keeping aspect ratio
    int width = static_cast<decltype(width)>(previewSize * src.data->width() / (double) src.data->height());
    src.data = image_buffer_ptr(new QImage(src.data->scaled(width, previewSize, Qt::KeepAspectRatio)));
    return src;
}

meta_t::meta_t():
    iso(0)
{

}

QString meta_t::getStringValue() const //should prepare human readable value
{
    return QString("ISO: %1").arg(iso);
}

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

void meta_t::load(const QString &fileName)
{
    auto image = Exiv2::ImageFactory::open(fileName.toStdString());
    if (image.get() != nullptr)
    {
        image->readMetadata();

        auto getValue = [&image](const auto& src, const std::string& key, auto& result) -> bool
        {
            auto r = std::find_if(src.begin(), src.end(), [&key](const auto& val){
               // qDebug() << val.key().c_str();
                return val.key() == key;
            });
            bool res = r != src.end();
            if (res)
                meta_value<typename std::remove_reference<decltype(result)>::type>(r->value(), result);
            else
                qDebug() << QString("Could not find key %1").arg( key.c_str());
            return res;
        };
        auto getValueLst = [&image, &getValue](const auto& src, const std::vector<std::string>& keys, auto& result) -> bool
        {
            bool res = false;
            for (const auto& s : keys)
            {
                res = getValue(src, s, result);
                if (res)
                    break;
            }
            return res;
        };
        getValueLst(image->exifData(), {"Exif.Photo.ISOSpeedRatings", "Exif.Image.ISOSpeedRatings"}, iso);
    }
}
