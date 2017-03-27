opencv {
  DEFINES *= USING_VIDEO_FS

  #guess I use opencv3, that's why linking -lopencv_videoio, on older maybe something else
  LIBS *= -lopencv_core -lopencv_videoio
  message('OpenCV allowed, using videos as fs')
}

HEADERS += \
    $$PWD/rgb24_data.h \
    $$PWD/image_loader.h \
    $$PWD/opencv_vid.h

SOURCES += \
    $$PWD/rgb24_data.cpp \
    $$PWD/image_loader.cpp

#lib to access exif/iptc/etc: http://www.exiv2.org
LIBS +=  -lexiv2
