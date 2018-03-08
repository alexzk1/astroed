#ifndef OPENCV_VID_H
#define OPENCV_VID_H

#ifdef USING_VIDEO_FS
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>

using VideoCapturePtr  = std::shared_ptr<cv::VideoCapture>;
using VideoCapturePtrW = std::weak_ptr<cv::VideoCapture>;

class VideoFileRead
{
private:
    std::shared_ptr<cv::Mat> buf;
    VideoCapturePtr ptr;
public:
    VideoFileRead(const std::string& fileName):
        buf(new cv::Mat()), ptr(nullptr)
    {
        const static auto backend = cv::VideoCaptureAPIs::CAP_ANY;
        ptr.reset(new cv::VideoCapture(fileName, backend), [](cv::VideoCapture* p)
        {
            if (p)
            {
                p->release();
                delete p;
            }
        });

        if (!ptr->isOpened())
            ptr->open(fileName);
    }

    ~VideoFileRead()
    {
        ptr.reset(); //ensuring VideoCapture is deleted before buf
        buf.reset();
    }

    bool read(cv::Mat& res)
    {
        if (ptr->read(*buf))
        {
            buf->copyTo(res);
            return true;
        }
        return false;
    }

    long count() const
    {
        return static_cast<long>(ptr->get(CV_CAP_PROP_FRAME_COUNT));
    }

    void rewindIf(long frame_num) const
    {
        if (static_cast<decltype(frame_num)>(ptr->get(CV_CAP_PROP_POS_FRAMES)) != frame_num)
            ptr->set(CV_CAP_PROP_POS_FRAMES, frame_num);
    }

    bool isRawVideo()
    {
        uint32_t val = static_cast<decltype (val)>(ptr->get(CV_CAP_PROP_FOURCC));
        //qDebug() << "Codec: " << std::string(prop_format.codec, 4).c_str() << prop_format.val;
        return val == 0; //fixme: it returns 0 for my raw sample, BUT it is possible that some more fouriercc can be returned
    }

};

using VideoFileReadPtr = std::shared_ptr<VideoFileRead>;
#endif

#endif // OPENCV_VID_H
