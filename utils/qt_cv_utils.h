#ifndef QT_CV_UTILS_H
#define QT_CV_UTILS_H

#include "palgorithm.h"

#ifdef USING_VIDEO_FS
#include <opencv2/opencv.hpp>
#include <opencv2/core/version.hpp>
#include <QImage>

#if CV_MAJOR_VERSION == 2
#error It seems we have wrong opencv version. Source was developped on 3.x
#endif

#endif

#include "cont_utils.h"
#include "fatal_err.h"
#include <functional>
#include <vector>

namespace utility
{
#if defined(USING_VIDEO_FS) || defined (USING_OPENCV)
    namespace opencv
    {
        inline void forEachChannel(cv::Mat &src, const std::function<void (cv::Mat &)> &functor)
        {
            std::vector<cv::Mat> channels(static_cast<size_t>(std::max(1, src.channels())));
            cv::split(src, channels);
            ALG_NS::for_each(channels.begin(), channels.end(), functor);
            src.release();
            cv::merge(channels.data(), channels.size(), src);
            ALG_NS::for_each(channels.begin(), channels.end(), [](auto& c){
                c.release();
            });
        }
    };
#endif
    namespace bgrrgb
    {
        using default_color_type = uint8_t; //how many bits per color

        //pixel iterator, which keeps counting in term of "pixel", which is tripplet of 3 colors
        template <typename ColorT, typename ColorType = ColorT*>
        class PixelIterator : public std::iterator<std::random_access_iterator_tag, ColorType>
        {
        private:
            ColorType buffer;
            size_t tripplets_amount;
            mutable size_t current;

            PixelIterator(ColorT* buffer, size_t tripplets_amount): //expects size of buffer to be 3 * tripplets_amount of ColotT elements exactly
                buffer(buffer),
                tripplets_amount(tripplets_amount),
                current(0)
            {
            }

        public:
            PixelIterator():
                buffer(nullptr),
                tripplets_amount(0),
                current(0)
            {
            }
            static PixelIterator start(ColorT* buffer, size_t tripplets_amount)
            {
                return PixelIterator(buffer, tripplets_amount);
            }

            static PixelIterator end(ColorT* buffer, size_t tripplets_amount)
            {
                PixelIterator r(buffer, tripplets_amount);
                r.current = tripplets_amount;
                return r;
            }

            const static PixelIterator start(const ColorT* buffer, const size_t tripplets_amount)
            {
                return PixelIterator(const_cast<ColorT*>(buffer), tripplets_amount);
            }

            const static PixelIterator end(const ColorT* buffer, const size_t tripplets_amount)
            {
                PixelIterator r(const_cast<ColorT*>(buffer), tripplets_amount);
                r.current = tripplets_amount;
                return r;
            }

            PixelIterator(const PixelIterator& ) = default;
            PixelIterator& operator = (const PixelIterator&) = default;
            ~PixelIterator() = default;

            ColorType operator*() const
            {

                if (current < tripplets_amount)
                    return buffer + current * 3;
                return nullptr;
            }


            ColorType operator[](size_t n) const
            {
                if (current + n < tripplets_amount)
                    return buffer + (current + n) * 3;
                return nullptr;
            }

            PixelIterator& operator++()
            {
                // actual increment takes place here
                ++current;
                return *this;
            }

            PixelIterator operator++(int)
            {
                PixelIterator tmp(*this); // copy
                operator++(); // pre-increment
                return tmp;   // return old value
            }

            PixelIterator& operator--()
            {
                if (current)
                    --current;
                return *this;
            }

            PixelIterator operator--(int)
            {
                PixelIterator tmp(*this);
                operator--();
                return tmp;
            }


            PixelIterator& operator+=(size_t val)
            {
                current += val;
                return *this;
            }

            PixelIterator& operator-=(size_t val)
            {
                if (current - val >0)
                    current -= val;
                else
                    current = 0;
                return *this;
            }

            PixelIterator operator+(size_t val) const
            {
                PixelIterator tmp(*this);
                tmp.current = current + val;
                return tmp;
            }

            PixelIterator operator-(size_t val) const
            {
                PixelIterator tmp(*this);
                tmp.current = current - val;
                return tmp;
            }


            size_t operator+(const PixelIterator& val) const
            {
                return current + val.current;
            }

            size_t operator-(const PixelIterator& val) const
            {
                return current - val.current;
            }

            bool operator < (const PixelIterator& c) const
            {
                return current < c.current;
            }

            bool operator <= (const PixelIterator& c) const
            {
                return current <= c.current;
            }

            bool operator > (const PixelIterator& c) const
            {
                return current > c.current;
            }

            bool operator >= (const PixelIterator& c) const
            {
                return current >= c.current;
            }

            bool operator == (const PixelIterator& c) const
            {
                return buffer == c.buffer && tripplets_amount == c.tripplets_amount && current == c.current;
            }

            bool operator !=(const PixelIterator& c) const
            {
                return !(*this == c);
            }
        };

        //expecting, that buffer is 3 * pixels_amount, allows conversion in both directions (invariant)
        template <typename ColorT = default_color_type> //allows to have more than 8 bytes per color
        inline void convertRGBBGR(ColorT *buffer, size_t pixels_amount)
        {
            using iter_t = PixelIterator<ColorT>;
            auto  istart = iter_t::start(buffer, pixels_amount);
            auto  iend   = iter_t::end  (buffer, pixels_amount);
            ALG_NS::for_each(istart, iend, [](auto p)
            {
                utility::swapPointed(p + 0, p + 2);
            });
        }
#if defined(USING_VIDEO_FS) || defined (USING_OPENCV)
        inline void rgbConvert(QImage& img)
        {
            if (img.format() != QImage::Format_RGB888)
                FATAL_RISE("Unexpected QImage pixel format");

            //QImage has method to do conversion, but not sure if it can use all CPUs as we do here with openmp
            utility::bgrrgb::convertRGBBGR(static_cast<uint8_t*>(img.bits()), static_cast<size_t>(img.width() * img.height()));
        }

        inline void rgbConvert(cv::Mat& img)
        {
            //fixme: implement per row conversion (each row is cont)
            if (!img.isContinuous())
                FATAL_RISE("Cannot convert non-continious matrix");

            utility::bgrrgb::convertRGBBGR(static_cast<uint8_t*>(img.data), static_cast<size_t>(img.cols * img.rows));
        }

        inline QImage copy(const cv::Mat& rgb)
        {
            //fixme: implement per row copy (each row is cont)
            if (!rgb.isContinuous())
                FATAL_RISE("Cannot copy non-continious matrix");

            return QImage(static_cast<uint8_t*>(rgb.data), rgb.cols, rgb.rows, QImage::Format_RGB888).copy();
        }

        inline QImage createFrom(const cv::Mat& rgb_p)
        {
            QImage tmp;
            auto make_it = [&tmp](const cv::Mat& rgb)
            {
                if (rgb.type() == CV_8UC3)
                {
                    tmp = copy(rgb);
                    rgbConvert(tmp);
                }
                if (rgb.type() == CV_8UC1)
                {
                    tmp = QImage(static_cast<uint8_t*>(rgb.data), rgb.cols, rgb.rows, QImage::Format_Grayscale8).copy();
                }
            };

            if (rgb_p.type() == CV_64FC1 || rgb_p.type() == CV_64FC3)
            {
                cv::Mat tm(rgb_p.clone());
                utility::opencv::forEachChannel(tm, [](cv::Mat& c)
                {
                    cv::convertScaleAbs(c, c, 255);
                });
                make_it(tm);
                tm.release();
            }
            else
                make_it(rgb_p);
            return tmp;
        }

#endif
    }
}

#endif // QT_CV_UTILS_H
