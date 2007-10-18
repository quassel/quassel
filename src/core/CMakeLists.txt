SET(core_SRCS core.cpp coresession.cpp server.cpp basichandler.cpp ircserverhandler.cpp userinputhandler.cpp ctcphandler.cpp sqlitestorage.cpp storage.cpp)
SET(core_HDRS )
SET(core_MOCS core.h coresession.h server.h basichandler.h ircserverhandler.h userinputhandler.h ctcphandler.h sqlitestorage.h storage.h)

QT4_WRAP_CPP(_MOC ${core_MOCS})
ADD_LIBRARY(core ${core_SRCS} ${_MOC})
TARGET_LINK_LIBRARIES(core common)
