SET(qxt_SRCS qxtmetaobject.cpp qxtnull.cpp qxtrpcpeer.cpp)
SET(qxt_HDRS qxtboundcfunction.h qxtboundfunctionbase.h qxtboundfunction.h qxtglobal.h qxtpimpl.h qxtmetaobject.h qxtnullable.h qxtnull.h)
SET(qxt_MOCS qxtboundfunction.h qxtrpcpeer.h)

QT4_WRAP_CPP(_MOC ${qxt_MOCS})
ADD_LIBRARY(qxt ${qxt_SRCS} ${_MOC})
