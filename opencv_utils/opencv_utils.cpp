#include <QImage>

#include "utils/palgorithm.h"
#include "opencv_utils.h"

#ifdef USING_OPENCV_GUI
#include <opencv/highgui.h>
#endif




using namespace utility::opencv;

contours_t utility::opencv::detectExternContours(const QImage &src, int treshold1, int treshold2)
{
    using namespace cv;
    const auto tmpColored(createMat(src));
    Mat src_gray;

    // Convert image to gray and blur it
    cvtColor( tmpColored, src_gray, CV_RGB2GRAY );
    blur( src_gray, src_gray, Size(3,3) );

    Mat canny_output;
    // Detect edges using canny, 3 is "Sobel" operator appertureSize
    Canny( src_gray, canny_output, treshold1, treshold2, 3, true); //http://docs.opencv.org/2.4/modules/imgproc/doc/feature_detection.html#void Canny(InputArray image, OutputArray edges, double threshold1, double threshold2, int apertureSize, bool L2gradient)

    contours_t  contours;
    hierarchy_t hierarchy;
    // Find contours
    findContours( canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
    return contours;
}

cv::Mat utility::opencv::createMat(const QImage &src)
{
    //assuming src is RGB888 ...
    if (src.format() != QImage::Format_RGB888)
        FATAL_RISE("Unexpected QImage pixel format");
    using namespace utility::bgrrgb;
    cv::Mat tmp = wrapQImage(src);
    auto res = tmp.clone();
    rgbConvert(res);
    return res;
}

const cv::Mat utility::opencv::wrapQImage(const QImage &src)
{
    if (src.format() != QImage::Format_RGB888)
        FATAL_RISE("Unexpected QImage pixel format");

    return cv::Mat(src.height(), src.width(), CV_8UC3, const_cast<uchar*>(src.bits()));
}


const static double EPSILON=2.2204e-16;
void utility::opencv::lucy_richardson_deconv_grayscale(cv::Mat &img, int num_iterations, double sigmaG)
{
    using namespace cv;

    img.convertTo(img, CV_64F);// convert to double
    normalize(img, img, 0, 1, NORM_MINMAX);

    // Lucy-Richardson Deconvolution Function
    // input-1 img: NxM matrix image
    // input-2 num_iterations: number of iterations
    // input-3 sigma: sigma of point spread function (PSF)
    // output result: deconvolution result

    // Window size of PSF
    int winSize = static_cast<decltype(winSize)>(8 * sigmaG + 1);

    // Initializations
    Mat Y = img.clone();
    Mat J1 = img.clone();
    Mat J2 = img.clone();
    Mat wI = img.clone();
    Mat imR = img.clone();
    Mat reBlurred = img.clone();

    Mat T1, T2, tmpMat1, tmpMat2;
    T1 = Mat(img.rows,img.cols, CV_64F, 0.0);
    T2 = Mat(img.rows,img.cols, CV_64F, 0.0);

    // Lucy-Rich. Deconvolution CORE

    double lambda = 0;
    for(int j = 0; j < num_iterations; j++)
    {
        if (j>1)
        {
            // calculation of lambda
            multiply(T1, T2, tmpMat1);
            multiply(T2, T2, tmpMat2);
            lambda=sum(tmpMat1)[0] / (sum( tmpMat2)[0]+EPSILON);
            // calculation of lambda
        }

        Y = J1 + lambda * (J1-J2);
        Y.setTo(0, Y < 0);
        // 1)
        GaussianBlur( Y, reBlurred, Size(winSize,winSize), sigmaG, sigmaG );//applying Gaussian filter
        reBlurred.setTo(EPSILON , reBlurred <= 0);

        // 2)
        divide(wI, reBlurred, imR);
        imR = imR + EPSILON;

        // 3)
        GaussianBlur( imR, imR, Size(winSize,winSize), sigmaG, sigmaG );//applying Gaussian filter

        // 4)
        J2 = J1.clone();
        multiply(Y, imR, J1);

        T2 = T1.clone();
        T1 = J1 - Y;
    }

    // output
    normalize(J1.clone(), img, 0, 1, NORM_MINMAX);
    convertScaleAbs(img, img, 256);
}

void utility::opencv::forEachChannel(cv::Mat &src, const std::function<void (cv::Mat &)> &functor)
{
    std::vector<cv::Mat> channels(src.channels());
    cv::split(src, channels);
    ALG_NS::for_each(channels.begin(), channels.end(), functor);
    cv::merge(channels.data(), channels.size(), src);
}

void utility::opencv::lucy_richardson_deconv(cv::Mat& img, int num_iterations, double sigmaG)
{
    forEachChannel(img, [&num_iterations, &sigmaG](cv::Mat& c)
    {
       lucy_richardson_deconv_grayscale(c, num_iterations, sigmaG);
    });
}
