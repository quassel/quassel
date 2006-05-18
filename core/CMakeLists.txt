SET(core_SRCS logger.cpp quassel.cpp)
SET(core_HDRS quassel.h)
SET(core_MOCS logger.h)

QT4_WRAP_CPP(_MOC ${core_MOCS})
ADD_LIBRARY(core ${_MOC} ${core_SRCS} ${core_HDRS})
