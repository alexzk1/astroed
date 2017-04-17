#ifndef OPENCV_UTILS_H
#define OPENCV_UTILS_H

#include <opencv2/opencv.hpp>
#include <QImage>
#include <vector>
#include <stdint.h>

namespace utility
{
    namespace opencv
    {
        using contours_t  = std::vector<std::vector<cv::Point>>;
        using hierarchy_t = std::vector<cv::Vec4i>;

        cv::Mat createMat(const QImage &src); //copies QImage to new Mat
        const cv::Mat wrapQImage(const QImage &src); //wraps buffer of image without copying, colors order remains same as QImage (RGB)
        contours_t detectExternContours(const QImage& src, int treshold1 = 100, int treshold2 = 200);
    };
};

#endif // OPENCV_UTILS_H
