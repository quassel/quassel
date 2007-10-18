SET(common_SRCS global.cpp logger.cpp message.cpp settings.cpp signalproxy.cpp util.cpp synchronizer.cpp networkinfo.cpp ircuser.cpp ircchannel.cpp)
SET(common_HDRS global.h message.h util.h)
SET(common_MOCS logger.h synchronizer.h networkinfo.h ircuser.h ircchannel.h settings.h signalproxy.h)

QT4_WRAP_CPP(_MOC ${common_MOCS})
ADD_LIBRARY(common ${common_SRCS} ${_MOC})
TARGET_LINK_LIBRARIES(common qxt)
