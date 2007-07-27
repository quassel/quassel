SET(common_SRCS global.cpp logger.cpp message.cpp settings.cpp util.cpp ircuser.cpp)
SET(common_HDRS message.h settings.h util.h ircuser.h)
SET(common_MOCS global.h logger.h quasselui.h)

QT4_WRAP_CPP(_MOC ${common_MOCS})
ADD_LIBRARY(common ${common_SRCS} ${_MOC})
