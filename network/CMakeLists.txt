SET(network_SRCS server.cpp buffer.cpp message.cpp)
SET(network_HDRS message.h)
SET(network_MOCS server.h buffer.h)

QT4_WRAP_CPP(_MOC ${network_MOCS})
ADD_LIBRARY(network ${_MOC} ${network_SRCS} ${network_HDRS})
