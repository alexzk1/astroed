HEADERS += \
    $$PWD/luaeditor.h \
    $$PWD/lualexer.h \
    $$PWD/lexer_types.h

SOURCES += \
    $$PWD/luaeditor.cpp \
    $$PWD/lualexer.cpp

LIBS += -lqscintilla2_qt5
