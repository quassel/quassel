SET(core_SRCS core.cpp coreproxy.cpp coresession.cpp server.cpp serverinfo.cpp sqlitestorage.cpp storage.cpp)
SET(core_HDRS )
SET(core_MOCS core.h coreproxy.h coresession.h server.h serverinfo.h sqlitestorage.h storage.h)

QT4_WRAP_CPP(_MOC ${core_MOCS})
ADD_LIBRARY(core ${core_SRCS} ${_MOC})
TARGET_LINK_LIBRARIES(core common)
