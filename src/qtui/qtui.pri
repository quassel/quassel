DEPMOD = uisupport common client
QT_MOD = core gui network

SRCS += bufferwidget.cpp chatline-old.cpp chatwidget.cpp coreconnectdlg.cpp configwizard.cpp debugconsole.cpp inputwidget.cpp \
        mainwin.cpp nicklistwidget.cpp qtui.cpp qtuistyle.cpp settingsdlg.cpp settingspagedlg.cpp \
        topicwidget.cpp verticaldock.cpp

HDRS += bufferwidget.h chatline-old.h chatwidget.h configwizard.h debugconsole.h inputwidget.h \
        coreconnectdlg.h mainwin.h nicklistwidget.h qtui.h qtuistyle.h settingsdlg.h settingspagedlg.h \
        topicwidget.h verticaldock.h

FORMNAMES = mainwin.ui coreaccounteditdlg.ui coreconnectdlg.ui bufferviewwidget.ui bufferwidget.ui nicklistwidget.ui settingsdlg.ui \
            settingspagedlg.ui topicwidget.ui debugconsole.ui inputwidget.ui

for(ui, FORMNAMES) {
  FRMS += ui/$${ui}
}

# Include settingspages
include(settingspages/settingspages.pri)
for(page, SETTINGSPAGES) {
  SRCS += settingspages/$${page}settingspage.cpp
  HDRS += settingspages/$${page}settingspage.h
  FRMS += settingspages/$${page}settingspage.ui
}

# Include additional files
for(src, SP_SRCS) {
  SRCS += settingspages/$${src}
}
for(hdr, SP_HDRS) {
  HDRS += settingspages/$${hdr}
}
for(frm, SP_FRMS) {
  FRMS += settingspages/$${frm}
}
