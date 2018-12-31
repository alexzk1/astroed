#include <QImage>
#include <iostream>

#include "utils/palgorithm.h"
#include "opencv_utils.h"

#ifdef USING_OPENCV_GUI
    #include <opencv/highgui.h>
#endif
const static auto   laplasianBlurKernelSize = 3;//odd, bigger value = slower, but less noise counting

using namespace utility::opencv;

algos::contours_t utility::opencv::algos::detectExternContours(const QImage &src, int treshold1, int treshold2)
{
    using namespace cv;
    const auto tmpColored = createMat(src);
    Mat src_gray;

    // Convert image to gray and blur it
    cvtColor(*tmpColored, src_gray, cv::COLOR_RGB2GRAY );
    blur( src_gray, src_gray, Size(3, 3) );

    Mat canny_output;
    // Detect edges using canny, 3 is "Sobel" operator appertureSize
    Canny( src_gray, canny_output, treshold1, treshold2, 3, true); //http://docs.opencv.org/2.4/modules/imgproc/doc/feature_detection.html#void Canny(InputArray image, OutputArray edges, double threshold1, double threshold2, int apertureSize, bool L2gradient)

    contours_t  contours;
    hierarchy_t hierarchy;
    // Find contours
    findContours( canny_output, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, Point(0, 0));

    src_gray.release();
    canny_output.release();

    return contours;
}

MatPtr utility::opencv::createMat(const QImage &src, bool grey)
{
    //assuming src is RGB888 ...
    if (src.format() != QImage::Format_RGB888)
    {
        std::cerr << "Wrong pixel format: " << src.format() << std::endl;
        std::cerr.flush();
        FATAL_RISE("Unexpected QImage pixel format");
    }
    using namespace utility::bgrrgb;
    cv::Mat tmp = wrapQImage(src);
    MatPtr res = MatPtr(new cv::Mat(), [](auto p)
    {
        if (p)
        {
            p->release();
            delete p;
        }
    });

    if (grey)
        cvtColor(tmp, *res, cv::COLOR_RGB2GRAY, 1);
    else
    {
        tmp.copyTo(*res);
        rgbConvert(*res);
    }

    forEachChannel(*res, [](cv::Mat & c)
    {
        c.convertTo(c, CV_64F, 1 / 255.); // convert to double
    });

    tmp.release();
    return res;
}

const cv::Mat utility::opencv::wrapQImage(const QImage &src)
{
    if (src.format() != QImage::Format_RGB888)
        FATAL_RISE("Unexpected QImage pixel format");

    return cv::Mat(src.height(), src.width(), CV_8UC3, const_cast<uchar*>(src.bits()));
}

const static double EPSILON = 2.2204e-16;
void utility::opencv::algos::lucy_richardson_deconv_grayscale(cv::Mat &img, int num_iterations, double sigmaG)
{
    using namespace cv;

    //    img.convertTo(img, cv::64F);// convert to double
    //    normalize(img, img, 0, 1, NORM_MINMAX);

    // Lucy-Richardson Deconvolution Function
    // input-1 img: NxM matrix image
    // input-2 num_iterations: number of iterations
    // input-3 sigma: sigma of point spread function (PSF)
    // output result: deconvolution result

    // Window size of PSF
    int winSize = static_cast<decltype(winSize)>(8 * sigmaG + 1);

    // Initializations
    Mat Y(img.clone());
    Mat J1(img.clone());
    Mat J2(img.clone());
    Mat wI(img.clone());
    Mat imR(img.clone());
    Mat reBlurred(img.clone());

    Mat T1(img.rows, img.cols, CV_64F, 0.0), T2(img.rows, img.cols, CV_64F, 0.0), tmpMat1, tmpMat2;

    // Lucy-Rich. Deconvolution CORE

    double lambda = 0;
    for (int j = 0; j < num_iterations; j++)
    {
        if (j > 1)
        {
            // calculation of lambda
            multiply(T1, T2, tmpMat1);
            multiply(T2, T2, tmpMat2);
            lambda = sum(tmpMat1)[0] / (sum( tmpMat2)[0] + EPSILON);
            // calculation of lambda
        }
        Y.release();
        Y = J1 + lambda * (J1 - J2);
        Y.setTo(0, Y < 0);
        // 1)
        GaussianBlur( Y, reBlurred, Size(winSize, winSize), sigmaG, sigmaG ); //applying Gaussian filter
        reBlurred.setTo(EPSILON, reBlurred <= 0);

        // 2)
        divide(wI, reBlurred, imR);
        imR = imR + EPSILON;

        // 3)
        GaussianBlur( imR, imR, Size(winSize, winSize), sigmaG, sigmaG ); //applying Gaussian filter

        // 4)
        J2.release();
        J2 = J1.clone();
        multiply(Y, imR, J1);

        T2.release();
        T2 = T1.clone();
        T1 = J1 - Y;
    }

    // output
    img = J1;
    //    normalize(J1.clone(), img, 0, 1, NORM_MINMAX);
    //    convertScaleAbs(img, img, 256);
    T1.release();
    T2.release();
    tmpMat1.release();
    tmpMat2.release();
    Y.release();
    J1.release();
    J2.release();
    wI.release();
    imR.release();
    reBlurred.release();
}

void utility::opencv::algos::lucy_richardson_deconv(cv::Mat& img, int num_iterations, double sigmaG)
{
    forEachChannel(img, [&num_iterations, &sigmaG](cv::Mat & c)
    {
        lucy_richardson_deconv_grayscale(c, num_iterations, sigmaG);
    });
}

//from https://stackoverflow.com/questions/40713929/weiner-deconvolution-using-opencv
void utility::opencv::algos::ForwardFFT(const cv::Mat &Src, fft_planes_t &FImg)
{
    using namespace cv;
    int M = getOptimalDFTSize(Src.rows);
    int N = getOptimalDFTSize(Src.cols);
    Mat padded;
    copyMakeBorder(Src, padded, 0, M - Src.rows, 0, N - Src.cols, BORDER_CONSTANT, Scalar::all(0));
    Mat planes[] = { Mat_<double>(padded), Mat::zeros(padded.size(), CV_64FC1) };
    Mat complexImg;
    merge(planes, 2, complexImg);
    dft(complexImg, complexImg);
    split(complexImg, planes);
    // crop result
    planes[0] = planes[0](Rect(0, 0, Src.cols, Src.rows));
    planes[1] = planes[1](Rect(0, 0, Src.cols, Src.rows));
    FImg[0] = planes[0].clone();
    FImg[1] = planes[1].clone();
    padded.release();
    complexImg.release();
}

//from https://stackoverflow.com/questions/40713929/weiner-deconvolution-using-opencv
void utility::opencv::algos::InverseFFT(const fft_planes_t &FImg, cv::Mat &Dst)
{
    using namespace cv;
    Mat complexImg;
    merge(FImg.data(), 2, complexImg);
    dft(complexImg, complexImg, DFT_INVERSE + DFT_SCALE);
    fft_planes_t tmp;
    split(complexImg, tmp.data());
    Dst = tmp[0];
    complexImg.release();
}

cv::Mat utility::opencv::algos::get_FWHM(const cv::Mat &src_greyscale)
{
    //https://en.wikipedia.org/wiki/Full_width_at_half_maximum
    const auto static coef = 2 * sqrt(2 * log(2));
    cv::Mat mean, stddev;
    cv::meanStdDev(src_greyscale, mean, stddev);
    stddev *= coef;

    //https://sourceforge.net/projects/astrofocuser/?source=typ_redirect
    stddev.forEach<double>([](double & v, const int*)
    {
        v = 2 * sqrt(v / M_PI);
    });
    mean.release();
    return stddev;
}


cv::Mat utility::opencv::algos::get_Blureness(const cv::Mat &src_greyscale)
{
    cv::Mat dest;
    cv::Laplacian(src_greyscale, dest, src_greyscale.type(), laplasianBlurKernelSize);
    return get_FWHM(dest);
}
