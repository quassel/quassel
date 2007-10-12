SET(common_SRCS global.cpp logger.cpp message.cpp settings.cpp signalproxy.cpp util.cpp ircuser.cpp)
SET(common_HDRS global.h message.h util.h)
SET(common_MOCS logger.h ircuser.h settings.h signalproxy.h)

QT4_WRAP_CPP(_MOC ${common_MOCS})
ADD_LIBRARY(common ${common_SRCS} ${_MOC})
TARGET_LINK_LIBRARIES(common qxt)
