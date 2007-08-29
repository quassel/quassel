SET(client_SRCS buffer.cpp buffertreemodel.cpp client.cpp clientsettings.cpp treemodel.cpp)
SET(client_HDRS )
SET(client_MOCS buffer.h buffertreemodel.h client.h clientsettings.h treemodel.h)

QT4_WRAP_CPP(_MOC ${client_MOCS})

ADD_LIBRARY(client ${client_SRCS} ${_MOC})
TARGET_LINK_LIBRARIES(client common)