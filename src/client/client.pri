DEPMOD = common
QT_MOD = core network gui   # gui is needed just for QColor... FIXME!
SRCS += buffer.cpp networkmodel.cpp client.cpp clientsettings.cpp mappedselectionmodel.cpp modelpropertymapper.cpp \
        nickmodel.cpp selectionmodelsynchronizer.cpp treemodel.cpp
HDRS += buffer.h networkmodel.h client.h clientsettings.h quasselui.h mappedselectionmodel.h modelpropertymapper.h \
        nickmodel.h selectionmodelsynchronizer.h treemodel.h
