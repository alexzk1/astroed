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
    class image_loader : public utility::ItCanBeOnlyOne<image_loader>
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
    public:
        image_loader();
        image_buffer_ptr getImage(const QString& fileName);
        void gc();

        size_t getMemoryUsed() const;
    };
}

#ifndef IMAGE_LOADER
#define IMAGE_LOADER utility::globalInstance<imaging::image_loader>()
#endif
