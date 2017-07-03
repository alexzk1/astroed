#ifndef OPENCV_UTILS_H
#define OPENCV_UTILS_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <array>
#include <stdint.h>
#include <functional>
#include <memory>
#include "utils/qt_cv_utils.h"

class QImage;
namespace utility
{
    namespace opencv
    {
        using MatPtr = std::shared_ptr<cv::Mat>;
        const cv::Mat wrapQImage(const QImage &src); //wraps buffer of image without copying, colors order remains same as QImage (RGB, 8 bit)
        MatPtr createMat(const QImage &src, bool grey = false); //copies QImage to new Mat and makes it CV_64F

        namespace algos
        {
            using fft_planes_t = std::array<cv::Mat, 2>;
            using contours_t   = std::vector<std::vector<cv::Point>>;
            using hierarchy_t  = std::vector<cv::Vec4i>;

            contours_t detectExternContours(const QImage& src, int treshold1 = 100, int treshold2 = 200);

            //this one algo seems cool to filter noise with low iterations
            //https://drive.google.com/file/d/0B7CO_0Z4IxeRWVdXSjZFRzF3Y0k/view
            void lucy_richardson_deconv_grayscale(cv::Mat& img, int num_iterations, double sigmaG);
            void lucy_richardson_deconv(cv::Mat& img, int num_iterations = 5, double sigmaG = 6.0);


            //----------------------------------------------------------
            // Compute Re and Im planes of FFT from Image
            //----------------------------------------------------------
            void ForwardFFT(const cv::Mat &Src, fft_planes_t& FImg);
            //----------------------------------------------------------
            // Compute image from Re and Im parts of FFT
            //----------------------------------------------------------
            void InverseFFT(const fft_planes_t& FImg, cv::Mat &Dst);

            cv::Mat get_FWHM(const cv::Mat& src);
        }
    };
};

#endif // OPENCV_UTILS_H
