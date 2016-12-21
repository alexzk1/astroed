HEADERS += \
    $$PWD/rgb24_data.h \
    $$PWD/image_loader.h

SOURCES += \
    $$PWD/rgb24_data.cpp \
    $$PWD/image_loader.cpp

#lib to access exif/iptc/etc: http://www.exiv2.org
LIBS +=  -lexiv2
