DEPMOD = common contrib/qxt
QT_MOD = core network gui   # gui is needed just for QColor... FIXME!
SRCS += buffer.cpp buffertreemodel.cpp client.cpp clientsettings.cpp treemodel.cpp
HDRS += buffer.h buffertreemodel.h client.h clientsettings.h treemodel.h
