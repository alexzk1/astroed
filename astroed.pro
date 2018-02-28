#-------------------------------------------------
#
# Project created by QtCreator 2016-11-11T02:20:22
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = astroed
TEMPLATE = app

#lets optimize for CPU on linux
unix:!macx:QMAKE_CXXFLAGS +=  -march=native

QMAKE_CXXFLAGS +=  -std=c++14 -Wall -frtti -fexceptions -Werror=return-type -Werror=overloaded-virtual
QMAKE_CXXFLAGS +=  -Wctor-dtor-privacy -Werror=delete-non-virtual-dtor -fstrict-aliasing
QMAKE_CXXFLAGS +=  -Werror=strict-aliasing -Wstrict-aliasing=2

macx:QMAKE_LFLAGS+=-Wl,-map,mapfile
CONFIG += c++14
CONFIG += opencv
CONFIG += openmp

#using new C++ libs for macos http://blog.michael.kuron-germany.de/2013/02/using-c11-on-mac-os-x-10-8/
#that may not work with C++14 though, Apple is slow
macx: QMAKE_CXXFLAGS += -stdlib=libc++
macx: QMAKE_LFLAGS += -lc++
macx: QMAKE_CXXFLAGS += -mmacosx-version-min=10.10
macx: QMAKE_MACOSX_DEPLOYMENT_TARGET=10.10


openmp {
DEFINES *= USING_OPENMP
QMAKE_CXXFLAGS *= -fopenmp
QMAKE_LFLAGS   *= -fopenmp
#uncomment to try global parallels
#DEFINES *= _GLIBCXX_PARALLEL
}

CONFIG(debug) {
     message( "Building the DEBUG Version" )
     QMAKE_CXXFLAGS += -O0 -g
     DEFINES += _DEBUG
     QMAKE_CXXFLAGS += -fsanitize=undefined -fsanitize=vptr
     LIBS += -lubsan
}
else {
    DEFINES += NDEBUG
    message( "Building the RELEASE Version" )
    QMAKE_CXXFLAGS += -O3
}

!macx: LIBS +=  -lrt

include($$PWD/config_ui/config_ui.pri)
include($$PWD/lua/lua_vm.pri)
include($$PWD/utils/utils.pri)
include($$PWD/memimage/memimage.pri)
include($$PWD/singleapp/singleapplication.pri)
include($$PWD/editor/codeeditor.pri)

opencv: include($$PWD/opencv_utils/opencv_utils.pri)

DEFINES += QAPPLICATION_CLASS=QApplication

SOURCES += main.cpp\
        mainwindow.cpp \
    previewsmodel.cpp \
    customtableview.cpp \
    scrollareapannable.cpp \
    fixedcombowithshortcut.cpp \
    sliderdrop.cpp \
    labeledslider.cpp

HEADERS  += mainwindow.h \
    previewsmodel.h \
    previewsmodeldata.h \
    customtableview.h \
    custom_roles.h \
    scrollareapannable.h \
    fixedcombowithshortcut.h \
    clickablelabel.h \
    sliderdrop.h \
    labeledslider.h


FORMS    += mainwindow.ui \
    labeledslider.ui

RESOURCES += \
    resources.qrc
