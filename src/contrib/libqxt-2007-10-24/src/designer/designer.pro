TARGET          =  QxtDesignerPlugins
DEPENDPATH      += .
INCLUDEPATH     += . ../core ../gui
win32: CONFIG   += dll
QT               = core gui
QXT              = core gui
INCLUDEPATH     += .
TEMPLATE         = lib
MOC_DIR          = .moc
OBJECTS_DIR      = .obj
CONFIG          += designer plugin qxtbuild
include(../../config.pri)


target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS+=target


HEADERS += qxtcheckcomboboxplugin.h \
           qxtdesignerplugin.h \
           qxtdesignerplugins.h \
           qxtgroupboxplugin.h \
           qxtlabelplugin.h \
           qxtlistwidgetplugin.h \
           qxtprogresslabelplugin.h \
           qxtpushbuttonplugin.h \
           qxtspansliderplugin.h \
           qxtstarsplugin.h \
           qxtstringspinboxplugin.h \
           qxttablewidgetplugin.h \
           qxttreewidgetplugin.h

SOURCES += qxtcheckcomboboxplugin.cpp \
           qxtdesignerplugin.cpp \
           qxtdesignerplugins.cpp \
           qxtgroupboxplugin.cpp \
           qxtlabelplugin.cpp \
           qxtlistwidgetplugin.cpp \
           qxtprogresslabelplugin.cpp \
           qxtpushbuttonplugin.cpp \
           qxtspansliderplugin.cpp \
           qxtstarsplugin.cpp \
           qxtstringspinboxplugin.cpp \
           qxttablewidgetplugin.cpp \
           qxttreewidgetplugin.cpp

RESOURCES += resources.qrc


CONFIG(debug, debug|release) {
        unix:  TARGET = $$join(TARGET,,,.debug)
        mac:   TARGET = $$join(TARGET,,,_debug)
        win32: TARGET = $$join(TARGET,,d)
}





