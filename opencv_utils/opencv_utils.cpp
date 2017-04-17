#include "opencv_utils.h"
#include "utils/qt_cv_utils.h"

using namespace utility::opencv;

contours_t utility::opencv::detectExternContours(const QImage &src, int treshold1, int treshold2)
{
    using namespace cv;
    const auto tmpColored(wrapQImage(src));
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
    using iter_t = PixelIterator<uint8_t>;

    cv::Mat res;

    auto sptr = static_cast<const uint8_t*>(src.bits());
    const auto pixels_amount = src.width() * src.height();

    const auto  istart = iter_t::start(sptr, static_cast<const size_t>(pixels_amount));
    const auto  iend   = iter_t::end  (sptr, static_cast<const size_t>(pixels_amount));

    res.create(src.width(), src.height(), CV_8UC3);

    auto dptr = res.ptr(0);

    ALG_NS::for_each(istart, iend, [dptr, sptr](const auto p)
    {
        auto addr_dist = p - sptr;
        auto dest = dptr + addr_dist;
        memcpy(dest, p, 3);
        utility::swapPointed(dest + 0, dest + 2);
    });

    return res;
}

const cv::Mat utility::opencv::wrapQImage(const QImage &src)
{
    if (src.format() != QImage::Format_RGB888)
        FATAL_RISE("Unexpected QImage pixel format");

    return cv::Mat(src.width(), src.height(), CV_8UC3, const_cast<uchar*>(src.bits()));
}
