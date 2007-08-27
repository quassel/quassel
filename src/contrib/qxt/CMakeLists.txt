SET(qxt_SRCS qxtrpcpeer.cpp)
SET(qxt_HDRS qxtglobal.h qxtpimpl.h)
SET(qxt_MOCS qxtrpcpeer.h)

QT4_WRAP_CPP(_MOC ${qxt_MOCS})
ADD_LIBRARY(qxt ${qxt_SRCS} ${_MOC})
