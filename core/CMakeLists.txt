SET(core_SRCS core.cpp coreproxy.cpp)
SET(core_HDRS )
SET(core_MOCS core.h coreproxy.h)

QT4_WRAP_CPP(_MOC ${core_MOCS})
ADD_LIBRARY(core ${_MOC} ${core_SRCS} ${core_HDRS})
