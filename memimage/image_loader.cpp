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
const static size_t sysMemory     = getMemorySize();
constexpr static size_t lowMemory = 2ll * 1024 * 1024 * 1024;
const static size_t maxMemUsage   = (sysMemory > lowMemory)?(sysMemory / 3): (sysMemory * 3/ 4); //mem pressure until it will start "gc" loops
const static auto& dumb           = IMAGE_LOADER; //ensuring single instance is created on program init

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
    lastSize(0),
    mutex(),
    assumeMirrored(false)
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

void image_cacher::wipe()
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    cache.clear();
    wcache.clear();
    lastSize = 0;
}

size_t image_cacher::getMemoryUsed() const
{
    return lastSize;
}

void image_cacher::setNewtoneTelescope(bool isNewtone)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    if (assumeMirrored != isNewtone)
    {
        wipe();
        assumeMirrored = isNewtone;
    }
}

bool image_cacher::isNewtoneTelescope() const
{
    return assumeMirrored;
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
        if (isNewtoneTelescope())
            img = img.mirrored(true, true);
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
    src.data = std::make_shared<QImage>(src.data->scaled(width, previewSize, Qt::KeepAspectRatio));
    return src;
}

meta_t::meta_t():
    iso(0),
    exposure(0, 1),
    aperture(0, 1),
    optZoom(1)
{

}

QString meta_t::getStringValue() const //should prepare human readable value
{
    using namespace exiv2_helpers::exiv_rationals;
    return QString("ISO: %1\nExposure: %2 s\nAperture: %3 mm\nOptical Zoom: x%4")
            .arg(iso)
            .arg(toDouble(exposure), 0, 'f', 2)
            .arg(toDouble(aperture), 0, 'f', 2)
            .arg(optZoom, 0, 'f', 2)
            ;
}



void meta_t::load(const QString &fileName)
{
    using namespace exiv2_helpers;
    auto image = Exiv2::ImageFactory::open(fileName.toStdString());
    if (image.get() != nullptr)
    {
        image->readMetadata();
        const auto test_all_metas = [&image](const std::vector<keys_t>& e_x_i, auto& result)
        {
            if (!getValueOneOf(image->exifData(),    e_x_i.at(0), result))
                if (!getValueOneOf(image->xmpData(), e_x_i.at(1), result))
                    getValueOneOf(image->iptcData(), e_x_i.at(2), result);
        };

        //todo: add keys to read values from xmp/iptc http://www.exiv2.org/tags.html
        test_all_metas({
                           {"Exif.Photo.ISOSpeedRatings", "Exif.Image.ISOSpeedRatings"}, //exif keys
                           {}, //xmp keys
                           {}, //iptc keys
                       }, iso);

        test_all_metas({
                           {"Exif.Photo.ExposureTime", "Exif.Image.ExposureTime"}, //exif keys
                           {}, //xmp keys
                           {}, //iptc keys
                       }, exposure);

        test_all_metas({
                           {"Exif.Photo.ApertureValue", "Exif.Image.ApertureValue", "Exif.Image.FNumber"}, //exif keys
                           {"Xmp.exif.ApertureValue", "Xmp.exif.FNumber"}, //xmp keys
                           {}, //iptc keys
                       }, aperture);
        Exiv2::Rational actualFocus;
        test_all_metas({
                           {"Exif.Photo.FocalLength", "Exif.Image.FocalLength"}, //exif keys
                           {"Xmp.exif.FocalLength"}, //xmp keys
                           {}, //iptc keys
                       }, actualFocus);
        RationalArray lensSpec;
        test_all_metas({
                           {"Exif.Photo.LensSpecification", "Exif.Image.LensSpecification"}, //exif keys
                           {}, //xmp keys
                           {}, //iptc keys
                       }, lensSpec);

        if (!lensSpec.size())
        {
            std::vector<uint16_t> sf;
            test_all_metas({
                               {"Exif.CanonCs.Lens"}, //exif keys
                               {}, //xmp keys
                               {}, //iptc keys
                           }, sf);
            if (sf.size() == 3)
            {
                lensSpec.push_back({sf.at(1), sf.at(2)});
            }
        }
        if (lensSpec.size())
            optZoom = exiv_rationals::toDouble(exiv_rationals::div(actualFocus, lensSpec.at(0)));
    }
}
