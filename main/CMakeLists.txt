SET(main_SRCS global.cpp logger.cpp util.cpp)
SET(main_HDRS util.h)
SET(main_MOCS global.h logger.h)

QT4_WRAP_CPP(_MOC ${main_MOCS})
ADD_LIBRARY(main ${_MOC} ${main_SRCS} ${main_HDRS})
