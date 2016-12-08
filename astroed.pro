#-------------------------------------------------
#
# Project created by QtCreator 2016-11-11T02:20:22
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = astroed
TEMPLATE = app

QMAKE_CXXFLAGS +=  -std=c++14 -Wall -frtti -fexceptions -Werror=return-type -Werror=overloaded-virtual
QMAKE_CXXFLAGS +=  -Wctor-dtor-privacy -Werror=delete-non-virtual-dtor -Werror=strict-aliasing -fstrict-aliasing


macx:QMAKE_LFLAGS+=-Wl,-map,mapfile
CONFIG += c++14

#using new C++ libs for macos http://blog.michael.kuron-germany.de/2013/02/using-c11-on-mac-os-x-10-8/
#that may not work with C++14 though, Apple is slow
macx: QMAKE_CXXFLAGS += -stdlib=libc++
macx: QMAKE_LFLAGS += -lc++
macx: QMAKE_CXXFLAGS += -mmacosx-version-min=10.10
macx: QMAKE_MACOSX_DEPLOYMENT_TARGET=10.10


CONFIG(debug, debug|release) {
     message( "Building the DEBUG Version" )
     QMAKE_CXXFLAGS += -O0 -g
     DEFINES += _DEBUG
}
else {
    DEFINES += NDEBUG
    message( "Building the RELEASE Version" )
    QMAKE_CXXFLAGS += -O2
}

!macx:LIBS +=  -lrt

include($$PWD/lua/lua_vm.pri)
include($$PWD/utils/utils.pri)
include($$PWD/memimage/memimage.pri)
include($$PWD/logic/bl.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
    previewsmodel.cpp \
    customtableview.cpp

HEADERS  += mainwindow.h \
    previewsmodel.h \
    previewsmodeldata.h \
    customtableview.h \
    custom_roles.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
