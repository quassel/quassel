SET(network_SRCS server.cpp)
SET(network_HDRS )
SET(network_MOCS server.h)

QT4_WRAP_CPP(_MOC ${network_MOCS})
ADD_LIBRARY(network ${_MOC} ${network_SRCS} ${network_HDRS})
