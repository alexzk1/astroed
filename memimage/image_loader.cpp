#include "image_loader.h"
#include <QImage>
#include <QDebug>
#include <iostream>

#include "utils/cont_utils.h"

#include <chrono>
#include <vector>
#include <stdint.h>
#include <exiv2/exiv2.hpp>
#include <exiv2/exif.hpp>
#include <QUrl>
#include <QTimer>
#include <QFile>
#include <QDir>
#include "config_ui/globalsettings.h"
#include "utils/qt_cv_utils.h"
#include "utils/runners.h"

#if defined(USING_VIDEO_FS) || defined (USING_OPENCV)
    #include "opencv_utils/opencv_utils.h"
#endif

using namespace imaging;

extern size_t getMemorySize();
extern size_t getCurrentRSS();

const static size_t sysMemory     = getMemorySize();
constexpr static size_t lowMemory = 2ll * 1024 * 1024 * 1024;
//mem pressure until it will start "gc" loops (with systems <2Gb ram we will have to use most of ram I think)
const static auto& dumb           = IMAGE_LOADER; //ensuring single instance is created on program init

const static int64_t longestDelay = 300; //how long at most image will remain cached since last access
const static int     previewSize  = 300; //recommended size

const static QString vfs_scheme(VFS_PREFIX);

constexpr static auto base_frames_to_string_numbering = 10;

static int64_t nows()
{
    using namespace std::chrono;
    return duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
};

image_cacher::image_cacher():
    cache(),
    wcache(),
    assumeMirrored(false),
    mutex()
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

meta_t image_cacher::getMeta(const QString &fileName)
{
    image_t_s t;
    findImage(fileName, t);
    return t.meta;
}

bool image_cacher::isLoaded(const QString &fileName) const
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    if (!wcache.count(fileName))
        return false;
    return !wcache.at(fileName).data.expired();
}

void image_cacher::clear_cacher()
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    cache.clear();
    wcache.clear();
}

void image_cacher::findImage(const QString& key, image_t_s& res)
{
    std::lock_guard<std::recursive_mutex> guard(mutex);

    if (cache.count(key))
        res = cache.at(key).second;

    if (!res && wcache.count(key))
    {
        //trying to restore pointer to somewhere existing image (it is possible somebody else holds shared_ptr copy)
        res = wcache.at(key); //overloaded operator =, it does lock()
    }

    if (!res)
    {
        //the image is no longer exists in memory. Loading from disk.
        res = createImage(key);
        //idea is to keep images hard locked in RAM until we have sufficient memory, then we unlock it
        //but it will still be in RAM until processing algorithms use it
        wcache[key] = static_cast<image_t_w>(res);
    }

    cache[key] = std::make_pair(nows(), res); //even if object exists in cache, just updating time
    gc();
}


void image_cacher::gc(bool no_wait)
{
    //kinda "gc" loop - removing weak pointers which were deleted already
    //const static size_t maxMemUsage   = (sysMemory > lowMemory)?(sysMemory / 2): (sysMemory * 3/ 4);
    size_t maxMemUsage = sysMemory * static_cast<size_t>(StaticSettingsMap::getGlobalSetts().readInt("Int_mem_pressure")) / 100;
    if (getMemoryUsed() > maxMemUsage || no_wait)
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        //1 step, removing too old hard links to images (pretty dumb if here)
        auto t = nows();
        utility::erase_if(cache, [t, no_wait, maxMemUsage](const auto & sp)
        {
            auto delay = t - sp.second.first;
            return (getMemoryUsed() > maxMemUsage && sp.second.second.data.unique() && (no_wait || delay > 15)) || delay > longestDelay;
        });
    }

    //2 step, if no hard links left anywhere - clearing weaks too, meaning memory is freed already
    utility::erase_if(wcache, [](const auto & sp)
    {
        bool r = sp.second.data.expired();
        return r;
    });
}

void image_cacher::wipe()
{
    clear_cacher();
}

size_t image_cacher::getMemoryUsed()
{
    return getCurrentRSS();
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
    clear_cacher();
}

#ifdef USING_VIDEO_FS

static const auto& getUserSelectedRawFormat()
{
    struct conv_def_t
    {
        decltype (cv::COLOR_RGB2BGR)  codec;
        uint32_t               src_channels;
    };

    const static std::vector<conv_def_t> rawVideosCodes =
    {
        {cv::COLOR_RGB2BGR,      3},
        {cv::COLOR_GRAY2BGR,     1},

        //fixme: other 3 (except 1) must be checked with real cameras and maybe swapped to match colors (i was guessing their positions)
        {cv::COLOR_BayerRG2BGR,  1},
        {cv::COLOR_BayerGB2BGR,  1}, //GRBG (only this one seems proper by test video)
        {cv::COLOR_BayerGR2BGR,  1},
        {cv::COLOR_BayerBG2BGR,  1},

    };

    auto index = StaticSettingsMap::getGlobalSetts().readInt("Raw_format_video"); //item selected INDEX
    return rawVideosCodes.at(static_cast<size_t>(index));
}

VideoFileReadPtr image_loader::getVideoCapturer(const QString& filePath) const
{
    //function must be called in locked state
    //std::lock_guard<std::recursive_mutex> guard(mutex);

    if (!frameLoaders.second || filePath != frameLoaders.first)
    {
        frameLoaders.second.reset(new VideoFileRead(filePath.toStdString()));
        frameLoaders.first = filePath;
    }
    return frameLoaders.second;
}

void image_loader::closeVideoCapturer() const
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    frameLoaders.second.reset();
}
#endif

bool image_cacher::isProperVfs(const QUrl &url)
{
    //such a condition for now, isValid == true for simple file-names too
    return url.isValid() && url.scheme() == vfs_scheme;
}

bool image_cacher::isUsingCached() const
{
    return StaticSettingsMap::getGlobalSetts().readBool("Bool_cache_frames");
}

QString image_cacher::cachedFileName(const QUrl &url) const
{
    QStringList pathlst = url.path().split("/", QString::SplitBehavior::KeepEmptyParts);
    pathlst.insert(pathlst.size() - 1, VFS_CACHED_FOLDER);
    auto dirp = pathlst.join("/");
    QDir tmp;
    if (tmp.mkpath(dirp))
    {
        auto frame_num = std::max<long>(0, url.fragment().toLong(nullptr, base_frames_to_string_numbering));
        return QString("%1/frame_%2.png").arg(dirp).arg(frame_num);
    }
    return QString();
}

image_cacher::image_t_s image_loader::createImage(const QString &key) const
{
    //must be called in locked state
    QUrl url(key);

    const bool is_url = isProperVfs(url);
    image_t_s tmp;
    //http://stackoverflow.com/questions/20895648/difference-in-make-shared-and-normal-shared-ptr-in-c
    //weak_ptr will keep make_shared result live forever!
    tmp.data  = decltype(tmp.data)(new QImage());

    {
        //ensuring img will close file "key"

        QImage img;

        const auto load_from_disk = [&img, &tmp](const QString & fn)
        {
            img = QImage(fn);
            *tmp.data = img.convertToFormat(QImage::Format_RGB888);
        };

        if (is_url)
        {
            //videofs is expexted to be in form: videofs://file_path.mov#frame_number
#ifdef USING_VIDEO_FS

            if (url.scheme() == vfs_scheme)
            {
                const auto path = url.path();
                //qDebug() << "Thread ID: " << utility::currentThreadId();

                bool uc = isUsingCached();
                QString cfn;
                if (uc)
                {
                    cfn = cachedFileName(url);
                    if (QFile::exists(cfn))
                        load_from_disk(cfn);
                    uc = !tmp.data->isNull();
                }
                if (!uc)
                {
                    auto ptr = getVideoCapturer(path);
                    const bool is_raw = ptr->isRawVideo();

                    bool ok_num;
                    auto frame_num = std::max<long>(0, url.fragment().toLong(&ok_num, base_frames_to_string_numbering));

                    if (ok_num && frame_num < ptr->count())
                    {
                        cv::Mat rgb;
                        if (ptr->read(frame_num, rgb))
                        {
                            if (isUsingCached() && !cfn.isEmpty())
                            {
                                //ok, some optimization, will do save to disk on delete from memory
                                tmp.data = std::shared_ptr<QImage>(new QImage(), [cfn, this](auto p)
                                {
                                    if (p)
                                    {
                                        if (!dctor) //on qt 5.8 if this lambda is called from destructor (by closing app) save makes sigsegv
                                            p->save(cfn); //saving cached image when it is thrown out of RAM
                                        delete p;
                                    }
                                });
                            }
                            if (is_raw)
                            {
                                const auto& t = getUserSelectedRawFormat();
                                auto rc = static_cast<decltype (t.src_channels)>(rgb.channels());
                                if (rc > t.src_channels)
                                {
                                    std::vector<cv::Mat> channels(rc);
                                    cv::split(rgb, channels);
                                    cv::merge(channels.data(), t.src_channels, rgb);
                                }

                                cv::Mat res;
                                cvtColor(rgb, res, t.codec, 3);
                                *tmp.data = utility::bgrrgb::createFrom(res);
                                res.release();
                            }
                            else
                                *tmp.data = utility::bgrrgb::createFrom(rgb);
                        }
                    }
                }
            }
            else
                closeVideoCapturer();
#endif
            //todo: more schemes maybe added here (dont forget to fix initial condition is_url)
        }
        else
        {
#ifdef USING_VIDEO_FS
            closeVideoCapturer();
#endif
            load_from_disk(key);
        }


        if (isNewtoneTelescope())
            *tmp.data = tmp.data->mirrored(true, true);
    }

    //done: this should load exif too
    if (!is_url) //loading exif only from files
        tmp.meta.loadExif(key);
    if (!tmp.data->isNull())
        tmp.meta.precalculate(tmp.data);
    return tmp;
}

void image_loader::wipe()
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
    clear_loader();
    image_cacher::wipe();
}

QStringList image_loader::getVideoFramesLinks(const QString &videoFileName)
{
    const static QChar filler('0');
    QStringList res;
#ifdef USING_VIDEO_FS
    VideoFileReadPtr ptr;
    int64_t fcount = 0;
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        ptr = getVideoCapturer(videoFileName);
        if (ptr)
            fcount = ptr->count();
    }
    for (auto i = 0; i < fcount; ++i)
        res << QString("%1://%2#%3").arg(vfs_scheme).arg(videoFileName).arg(i, 8, base_frames_to_string_numbering, filler);

#else
    Q_UNUSED(videoFileName)
#endif
    return res;
}

QString image_loader::getFileLinkForExternalTools(const QString &originalLink) const
{
    QString result = originalLink;
#ifdef USING_VIDEO_FS
    QUrl url(originalLink);
    if (isProperVfs(url))
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        auto img = createImage(originalLink);

        if (isUsingCached())
        {
            QString cfn = cachedFileName(url);
            if (QFile::exists(cfn))
                return cfn;
        }

        if (!img.tempFile)
        {
            img.tempFile.reset(new QTemporaryFile());
            img.tempFile->setFileTemplate("dumped_frame_tmp_XXXXXX.png");
            if (img.tempFile->open())
            {
                img.data->save(img.tempFile.get(), "png");
                img.tempFile->flush();
                img.tempFile->close();
            }
        }

        //ensuring temp file will not be deleted for some time, even if cached image is cleared out by heavy GC (happens when user clicks during loading of movie)
        auto fcopy = img.tempFile;
        QTimer::singleShot(60000, [fcopy]() {;});

        result = img.tempFile->fileName();
    }
#endif
    return result;
}

image_loader::~image_loader()
{
    dctor = true;
    clear_loader();
}

void image_loader::clear_loader()
{
    std::lock_guard<std::recursive_mutex> guard(mutex);
#ifdef USING_VIDEO_FS
    closeVideoCapturer();
#endif
}

image_cacher::image_t_s image_preview_loader::createImage(const QString &key) const
{
    //this class caches previews, not full objects, so we access OTHER object with full images
    image_t_s src;
    IMAGE_LOADER.findImage(key, src);
    //keeping aspect ratio
    auto width = static_cast<int>(static_cast<int64_t>(previewSize * src.data->width() / static_cast<double>(src.data->height())));
    src.data  = decltype (src.data)(new QImage(src.data->scaled(width, previewSize, Qt::KeepAspectRatio)));
    return src;
}

#define INIT(VAR) exiv2_helpers::exiv_rationals::varInit<decltype (VAR)>()

meta_t::meta_t():
    wasLoaded(false),
    iso(INIT(iso)),
    exposure(INIT(exposure)),
    aperture(INIT(aperture)),
    optZoom(1),
    precalcs()
{
}

#undef INIT

QString meta_t::getStringValue() const //should prepare human readable value
{
    QString extra = QString("Blureness: %1")
                    .arg(precalcs.blureness, 0, 'f', 4);
    if (!wasLoaded)
        return extra + QObject::tr("\nNo EXIF tags.");

    using namespace exiv2_helpers::exiv_rationals;
    return QString(QObject::tr("%0\nISO: %1\nExposure: %2 s\nAperture: %3 mm\nOptical Zoom: x%4"))
           .arg(extra)
           .arg(iso)
           .arg(toDouble(exposure), 0, 'f', 4)
           .arg(toDouble(aperture), 0, 'f', 2)
           .arg(optZoom, 0, 'f', 2)
           ;
}

void meta_t::loadExif(const QString &fileName)
{
    using namespace exiv2_helpers;
    if (QFile::exists(fileName))
    {
        auto image = Exiv2::ImageFactory::open(fileName.toStdString());
        if (image.get() != nullptr)
        {
            image->readMetadata();
            const auto test_all_metas = [&image](const std::vector<keys_t>& e_x_i, auto & result)
            {
                if (!getValueOneOf(image->exifData(),    e_x_i.at(0), result))
                    if (!getValueOneOf(image->xmpData(), e_x_i.at(1), result))
                        getValueOneOf(image->iptcData(), e_x_i.at(2), result);
            };

            //todo: add keys to read values from xmp/iptc http://www.exiv2.org/tags.html
            test_all_metas(
            {
                {"Exif.Photo.ISOSpeedRatings", "Exif.Image.ISOSpeedRatings"}, //exif keys
                {}, //xmp keys
                {}, //iptc keys
            }, iso);

            //fixme: not really sure, if it is rational or double, from my camera it comes as double in .Photo. field
            //possibly .Image. field will be rational
            test_all_metas(
            {
                {"Exif.Photo.ExposureTime", "Exif.Image.ExposureTime"}, //exif keys
                {}, //xmp keys
                {}, //iptc keys
            }, exposure);

            test_all_metas(
            {
                {"Exif.Photo.ApertureValue", "Exif.Image.ApertureValue", "Exif.Image.FNumber"}, //exif keys
                {"Xmp.exif.ApertureValue", "Xmp.exif.FNumber"}, //xmp keys
                {}, //iptc keys
            }, aperture);
            Exiv2::Rational actualFocus;
            test_all_metas(
            {
                {"Exif.Photo.FocalLength", "Exif.Image.FocalLength"}, //exif keys
                {"Xmp.exif.FocalLength"}, //xmp keys
                {}, //iptc keys
            }, actualFocus);
            RationalArray lensSpec;
            test_all_metas(
            {
                {"Exif.Photo.LensSpecification", "Exif.Image.LensSpecification"}, //exif keys
                {}, //xmp keys
                {}, //iptc keys
            }, lensSpec);

            if (!lensSpec.size())
            {
                std::vector<uint16_t> sf;
                test_all_metas(
                {
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

            wasLoaded = true;
        }
    }
}

void meta_t::precalculate(const image_buffer_ptr &img)
{
#if defined(USING_VIDEO_FS) || defined (USING_OPENCV)
    using namespace utility::opencv;
    auto mat = createMat(*img, true);
    auto res = algos::get_Blureness(*mat);
    precalcs.blureness = res.at<decltype (precalcs.blureness)>(0);
    res.release();

#else
#warning opencv is disabled, precalculations will be too as well.
#endif
}
