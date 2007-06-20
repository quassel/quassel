SET(client_SRCS buffer.cpp buffertreemodel.cpp client.cpp clientproxy.cpp treemodel.cpp)
SET(client_HDRS )
SET(client_MOCS buffer.h buffertreemodel.h client.h clientproxy.h treemodel.h)

QT4_WRAP_CPP(_MOC ${client_MOCS})
ADD_LIBRARY(client ${_MOC} ${client_SRCS} ${client_HDRS})
