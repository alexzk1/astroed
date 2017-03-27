#ifndef OPENCV_VID_H
#define OPENCV_VID_H

#ifdef USING_VIDEO_FS
#include <memory>
#include <opencv2/opencv.hpp>

using VideoCapturePtr  = std::shared_ptr<cv::VideoCapture>;
using VideoCapturePtrW = std::weak_ptr<cv::VideoCapture>;
#endif

#endif // OPENCV_VID_H
