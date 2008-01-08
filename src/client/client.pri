DEPMOD = common
QT_MOD = core network gui   # gui is needed just for QColor... FIXME!
SRCS += buffer.cpp treemodel.cpp networkmodel.cpp buffermodel.cpp client.cpp clientsettings.cpp mappedselectionmodel.cpp modelpropertymapper.cpp \
        nickmodel.cpp selectionmodelsynchronizer.cpp
HDRS += buffer.h treemodel.h networkmodel.h buffermodel.h client.h clientsettings.h quasselui.h mappedselectionmodel.h modelpropertymapper.h \
        nickmodel.h selectionmodelsynchronizer.h
