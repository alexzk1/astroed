#ifndef OPENCV_UTILS_H
#define OPENCV_UTILS_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <stdint.h>
#include <functional>
#include "utils/qt_cv_utils.h"

class QImage;
namespace utility
{
    namespace opencv
    {
        using contours_t  = std::vector<std::vector<cv::Point>>;
        using hierarchy_t = std::vector<cv::Vec4i>;

        cv::Mat createMat(const QImage &src); //copies QImage to new Mat
        const cv::Mat wrapQImage(const QImage &src); //wraps buffer of image without copying, colors order remains same as QImage (RGB)
        contours_t detectExternContours(const QImage& src, int treshold1 = 100, int treshold2 = 200);
        void forEachChannel(cv::Mat& src, const std::function<void(cv::Mat& channel)>& functor);

        //this one algo seems cool to filter noise with low iterations
        //https://drive.google.com/file/d/0B7CO_0Z4IxeRWVdXSjZFRzF3Y0k/view
        void lucy_richardson_deconv_grayscale(cv::Mat& img, int num_iterations, double sigmaG);
        void lucy_richardson_deconv(cv::Mat& img, int num_iterations = 5, double sigmaG = 6.0);
    };
};

#endif // OPENCV_UTILS_H
