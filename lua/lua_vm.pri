QMAKE_CXXFLAGS += -std=c++11 -fexceptions
INCLUDEPATH += $$PWD/..

include($$PWD/luasrc/lua.pri)

HEADERS += \
    $$PWD/lua_params.h \
    $$PWD/variant_convert.h \
    $$PWD/lua_gen.h
