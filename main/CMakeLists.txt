SET(main_SRCS quassel.cpp logger.cpp)
SET(main_HDRS )
SET(main_MOCS quassel.h logger.h)

QT4_WRAP_CPP(_MOC ${main_MOCS})
ADD_LIBRARY(main ${_MOC} ${main_SRCS} ${main_HDRS})
