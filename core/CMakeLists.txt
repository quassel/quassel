SET(core_SRCS logger.cpp quassel.cpp proxy.cpp core.cpp)
SET(core_HDRS quassel.h proxy.h)
SET(core_MOCS logger.h core.h)

QT4_WRAP_CPP(_MOC ${core_MOCS})
ADD_LIBRARY(core ${_MOC} ${core_SRCS} ${core_HDRS})
