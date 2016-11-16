#include "image_loader.h"
#include <QImage>
#include "utils/erase_if.h"
#include <chrono>

using namespace imaging;

extern size_t getMemorySize();
const static size_t sysMemory   = getMemorySize();
const static size_t maxMemUsage = sysMemory / 3; //mem pressure until it will start "gc" loops
const static auto& dumb = IMAGE_LOADER; //ensuring single instance is created on program init

static int64_t nows()
{
    using namespace std::chrono;
    return duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
};

image_loader::image_loader(): utility::ItCanBeOnlyOne<image_loader>(),
    cache(),
    wcache(),
    mutex(),
    lastSize(0)
{

}

image_buffer_ptr image_loader::getImage(const QString &fileName)
{
    image_buffer_ptr res;
    std::lock_guard<std::recursive_mutex> guard(mutex);

    if (wcache.count(fileName))
    {
        res = wcache.at(fileName).second.lock();
    }

    if (!res)
    {
        //todo: need some kinda of "virtual filesystem" to dig into avi/mp4 and pick frames there
        QImage img(fileName);
        res = std::make_shared<QImage>();
        *res = img.convertToFormat(QImage::Format_RGB888);

        auto sz = static_cast<size_t>(res->byteCount());
        //idea is to keep images hard locked in RAM until we have sufficient memory, then we unlock it
        //but it will still be in RAM until processing algorithms use it

        wcache[fileName] = std::make_pair(sz, res);
        lastSize += sz;
    }

    cache[fileName] = std::make_pair(nows(), res); //even if object exists in cache, just updating time
    gc();

    return res;
}

void image_loader::gc()
{
    //kinda "gc" loop - removing weak pointers which were deleted already
    if (lastSize > maxMemUsage)
    {
        std::lock_guard<std::recursive_mutex> guard(mutex);
        //1 step, removing too old hard links to images (pretty dumb if here)
        auto t = nows();
        utility::erase_if(cache, [this, t](const auto& sp)
        {
            auto delay = t - sp.second.first;
            return (sp.second.second.unique() && delay > 20) || delay > 120;
        });

        //2 step, if no hard links left anywhere - clearing weaks too, meaning memory is freed already
        utility::erase_if(wcache, [this](const auto& sp)
        {
            bool r = sp.second.second.expired();
            if (r)
                lastSize -= sp.second.first;
            return r;
        });
    }
}

size_t image_loader::getMemoryUsed() const
{
    return lastSize;
}
