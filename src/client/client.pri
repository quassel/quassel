DEPMOD = common
QT_MOD = core network gui

SRCS += buffer.cpp buffersettings.cpp clientbacklogmanager.cpp treemodel.cpp networkmodel.cpp buffermodel.cpp \
        client.cpp clientsettings.cpp clientsyncer.cpp mappedselectionmodel.cpp selectionmodelsynchronizer.cpp
HDRS += buffer.h buffersettings.h clientbacklogmanager.h treemodel.h networkmodel.h buffermodel.h \
        client.h clientsettings.h clientsyncer.h quasselui.h mappedselectionmodel.h selectionmodelsynchronizer.h

sputdev {
  SRCS += messagefilter.cpp messagemodel.cpp
  HDRS += messagefilter.h messagemodel.h
}

