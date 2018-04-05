#ifndef OPENCV_VID_H
#define OPENCV_VID_H

#ifdef _DEBUG
#include <QDebug>
#endif

#ifdef USING_VIDEO_FS
#include <memory>
#include <string>
#include <atomic>
#include <opencv2/opencv.hpp>
#include "opencv_utils/opencv_utils.h"
#include "utils/runners.h"
#include "utils/conditional_wait.h"
#include "utils/inst_once.h"
using VideoCapturePtr  = std::shared_ptr<cv::VideoCapture>;
using VideoCapturePtrW = std::weak_ptr<cv::VideoCapture>;

class VideoFileRead
{
private:
    const std::string fileName;
    utility::runner_t thr;
    std::atomic<long> framesCount;
    std::atomic<bool> isRaw;

    confirmed_pass cw_ready_params;
    confirmed_pass cw_ready_buf;
    std::atomic<long> pFrame;
    cv::Mat rBuf;
    std::atomic<bool> rOk;
public:
    VideoFileRead(const std::string& fileName):
        fileName(fileName),
        thr(nullptr),
        framesCount(-1),
        isRaw(false),
        cw_ready_params(),
        cw_ready_buf(),
        pFrame(-1),
        rBuf(),
        rOk(false)
    {
    }

    ~VideoFileRead()
    {
        thr.reset();
#ifdef _DEBUG
        qDebug() << "VideFileRead thread ended";
#endif
    }

    //ensuring VideoCapture will be created and accessed in the same fixed thread (otherwise it leaks memory)
    //...and that is NOT working yet - memory leaks

    bool read(long frame, cv::Mat& res)
    {
        using namespace utility;


        if (!thr)
        {
            thr = startNewRunner([this](const auto should_stop)
            {
                cv::VideoCapture vid(fileName, cv::CAP_ANY);

                framesCount = static_cast<long>(vid.get(CV_CAP_PROP_FRAME_COUNT));
                uint32_t val = static_cast<decltype (val)>(vid.get(CV_CAP_PROP_FOURCC));
                isRaw = (val == 0); //fixme: it returns 0 for my raw sample, BUT it is possible that some more fouriercc can be returned
                vid.set(CV_CAP_PROP_BUFFERSIZE, 1);

                while (!(*should_stop))
                {
                    if (!cw_ready_params.tryWaitConfirm(200))
                        continue;

                    long frame = pFrame;

                    if (!(*should_stop) && frame > -1)
                    {
                        if (static_cast<long>(vid.get(CV_CAP_PROP_POS_FRAMES)) != frame)
                            vid.set(CV_CAP_PROP_POS_FRAMES, frame);
                        rOk = vid.read(rBuf);
                    }
                    cw_ready_buf.confirm();
                }
                vid.release();
                cw_ready_buf.confirm(); //ensuring main waiter will go on
            });
        }

        pFrame = frame;
        rOk = false;

        cw_ready_params.confirm();
        cw_ready_buf.waitConfirm();

        if (rOk)
            rBuf.copyTo(res);

        return rOk;
    }

    long count()
    {
        initStats();
        return framesCount;
    }

    bool isRawVideo()
    {
        initStats();
        return isRaw;
    }

private:
    void initStats()
    {
        if (framesCount < 0)
        {
            cv::Mat tmp;
            read(-1, tmp);
        }
    }
};

using VideoFileReadPtr = std::shared_ptr<VideoFileRead>;
#endif

#endif // OPENCV_VID_H
