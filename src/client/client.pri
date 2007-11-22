DEPMOD = common
QT_MOD = core network gui   # gui is needed just for QColor... FIXME!
SRCS += buffer.cpp buffertreemodel.cpp client.cpp clientsettings.cpp mappedselectionmodel.cpp modelpropertymapper.cpp \
        nicktreemodel.cpp selectionmodelsynchronizer.cpp treemodel.cpp
HDRS += buffer.h buffertreemodel.h client.h clientsettings.h quasselui.h mappedselectionmodel.h modelpropertymapper.h \
        nicktreemodel.h selectionmodelsynchronizer.h treemodel.h
