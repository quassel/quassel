SET(common_SRCS global.cpp logger.cpp util.cpp message.cpp settings.cpp)
SET(common_HDRS util.h message.h settings.h)
SET(common_MOCS global.h logger.h quasselui.h)

QT4_WRAP_CPP(_MOC ${common_MOCS})
ADD_LIBRARY(common ${_MOC} ${common_SRCS} ${common_HDRS})
