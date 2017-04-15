#qmake will be launched on any changes in project - who cares - will recreate resources for us
system(cd $$PWD; perl makeqrc.pl > luaqrc.qrc)
RESOURCES += $$PWD/luaqrc.qrc

HEADERS += \
    $$PWD/luaqrcpackager.h \
    $$PWD/luaaevm.h

SOURCES += \
    $$PWD/luaqrcpackager.cpp \
    $$PWD/luaaevm.cpp

