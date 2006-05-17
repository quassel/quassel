SET(main_SRCS logger.cpp quassel.cpp)
SET(main_HDRS quassel.h)
SET(main_MOCS logger.h)

QT4_WRAP_CPP(_MOC ${main_MOCS})
ADD_LIBRARY(main ${_MOC} ${main_SRCS} ${main_HDRS})
