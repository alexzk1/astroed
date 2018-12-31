DEFINES *= USING_OPENCV
LIBS *= -lopencv_core -lopencv_imgproc

INCLUDEPATH += /usr/include/opencv4

HEADERS += \
    $$PWD/opencv_utils.h

SOURCES += \
    $$PWD/opencv_utils.cpp

opencvgui {
   LIBS *= -lopencv_highgui
   DEFINES *= USING_OPENCV_GUI
}
