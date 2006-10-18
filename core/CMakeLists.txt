SET(core_SRCS logger.cpp quassel.cpp core.cpp)
SET(core_HDRS )
SET(core_MOCS logger.h core.h quassel.h)

QT4_WRAP_CPP(_MOC ${core_MOCS})
ADD_LIBRARY(core ${_MOC} ${core_SRCS} ${core_HDRS})
