SET(network_SRCS message.cpp builtin_cmds.cpp builtin_handlers.cpp server.cpp)
SET(network_HDRS message.h cmdcodes.h)
SET(network_MOCS server.h)

QT4_WRAP_CPP(_MOC ${network_MOCS})
ADD_LIBRARY(network ${_MOC} ${network_SRCS} ${network_HDRS})
