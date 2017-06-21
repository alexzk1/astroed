#line below controls if lua will do GC on each allocation
#QMAKE_CXXFLAGS += -DHARDMEMTESTS

HEADERS += \
    $$PWD/lapi.h \
    $$PWD/lauxlib.h \
    $$PWD/lcode.h \
    $$PWD/lctype.h \
    $$PWD/ldebug.h \
    $$PWD/ldo.h \
    $$PWD/lfunc.h \
    $$PWD/lgc.h \
    $$PWD/llex.h \
    $$PWD/llimits.h \
    $$PWD/lmem.h \
    $$PWD/lobject.h \
    $$PWD/lopcodes.h \
    $$PWD/lparser.h \
    $$PWD/lprefix.h \
    $$PWD/lstate.h \
    $$PWD/lstring.h \
    $$PWD/ltable.h \
    $$PWD/ltm.h \
    $$PWD/lua.h \
    $$PWD/lua.hpp \
    $$PWD/luaconf.h \
    $$PWD/lualib.h \
    $$PWD/lundump.h \
    $$PWD/lvm.h \
    $$PWD/lzio.h \
    $$PWD/customlua.h \
    $$PWD/unidata.h \
    $$PWD/lualock.h \
    $$PWD/lua_allocator.h


LUA_SOURCES += \
    $$PWD/lapi.c.cpp \
    $$PWD/lauxlib.c.cpp \
    $$PWD/lbaselib.c.cpp \
    $$PWD/lbitlib.c.cpp \
    $$PWD/lcode.c.cpp \
    $$PWD/lcorolib.c.cpp \
    $$PWD/lctype.c.cpp \
    $$PWD/ldblib.c.cpp \
    $$PWD/ldebug.c.cpp \
    $$PWD/ldo.c.cpp \
    $$PWD/ldump.c.cpp \
    $$PWD/lfunc.c.cpp \
    $$PWD/lgc.c.cpp \
    $$PWD/linit.c.cpp \
    $$PWD/liolib.c.cpp \
    $$PWD/llex.c.cpp \
    $$PWD/lmathlib.c.cpp \
    $$PWD/lmem.c.cpp \
    $$PWD/loadlib.c.cpp \
    $$PWD/lobject.c.cpp \
    $$PWD/lopcodes.c.cpp \
    $$PWD/loslib.c.cpp \
    $$PWD/lparser.c.cpp \
    $$PWD/lstate.c.cpp \
    $$PWD/lstring.c.cpp \
    $$PWD/lstrlib.c.cpp \
    $$PWD/ltable.c.cpp \
    $$PWD/ltablib.c.cpp \
    $$PWD/ltm.c.cpp \
    $$PWD/lundump.c.cpp \
    $$PWD/lvm.c.cpp \
    $$PWD/lzio.c.cpp \
    $$PWD/lutf8lib.c.cpp \
    $$PWD/lutf8lib_cust.cpp

#I have moved lua sources from C to C++, this breaks some rules, for example, union cast is UB
#https://stackoverflow.com/questions/27683777/how-to-specify-compiler-flag-to-a-single-source-file-with-qmake
lua_compiler.name = lua_compiler
lua_compiler.input = LUA_SOURCES
lua_compiler.dependency_type = TYPE_C
lua_compiler.variable_out = OBJECTS
lua_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_IN_BASE}$${first(QMAKE_EXT_OBJ)}
lua_compiler.commands = $${QMAKE_CXX} $(CXXFLAGS) -fno-strict-aliasing -Wno-error=strict-aliasing $(INCPATH) -c ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += lua_compiler

#CONFIG(debug) {
#unix:!macx {
#   HEADERS += $$PWD/socket/auxiliar.h \
#    $$PWD/socket/buffer.h \
#    $$PWD/socket/compat.h \
#    $$PWD/socket/except.h \
#    $$PWD/socket/inet.h \
#    $$PWD/socket/io.h \
#    $$PWD/socket/luasocket.h \
#    $$PWD/socket/mime.h \
#    $$PWD/socket/options.h \
#    $$PWD/socket/pierror.h \
#    $$PWD/socket/select.h \
#    $$PWD/socket/socket.h \
#    $$PWD/socket/tcp.h \
#    $$PWD/socket/timeout.h \
#    $$PWD/socket/udp.h \
#    $$PWD/socket/unix.h \
#    $$PWD/socket/usocket.h

#    SOURCES += $$PWD/socket/auxiliar.c.cpp \
#    $$PWD/socket/buffer.c.cpp \
#    $$PWD/socket/compat.c.cpp \
#    $$PWD/socket/except.c.cpp \
#    $$PWD/socket/inet.c.cpp \
#    $$PWD/socket/io.c.cpp \
#    $$PWD/socket/luasocket.c.cpp \
#    $$PWD/socket/mime.c.cpp \
#    $$PWD/socket/options.c.cpp \
#    $$PWD/socket/select.c.cpp \
#    $$PWD/socket/serial.c.cpp \
#    $$PWD/socket/tcp.c.cpp \
#    $$PWD/socket/timeout.c.cpp \
#    $$PWD/socket/udp.c.cpp \
#    $$PWD/socket/unix.c.cpp \
#    $$PWD/socket/usocket.c.cpp

#    DEFINES += USE_SOCKETS_LINUX
#    DEFINES += USE_SOCKETS
#}
#}
