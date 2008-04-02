DEPMOD = client common uisupport
QT_MOD = core gui network

SRCS += aboutdlg.cpp bufferwidget.cpp chatitem.cpp chatline.cpp chatline-old.cpp chatscene.cpp chatview.cpp chatwidget.cpp \
        coreconfigwizard.cpp coreconnectdlg.cpp configwizard.cpp debugconsole.cpp inputwidget.cpp \
        mainwin.cpp nicklistwidget.cpp qtui.cpp qtuisettings.cpp qtuistyle.cpp settingsdlg.cpp settingspagedlg.cpp \
        topicbutton.cpp topicwidget.cpp verticaldock.cpp jumpkeyhandler.cpp

HDRS += aboutdlg.h bufferwidget.h chatitem.h chatline.h chatline-old.h chatscene.h chatview.h chatwidget.h \
        coreconfigwizard.h configwizard.h debugconsole.h inputwidget.h \
        coreconnectdlg.h mainwin.h nicklistwidget.h qtui.h qtuisettings.h qtuistyle.h settingsdlg.h settingspagedlg.h \
        topicbutton.h topicwidget.h verticaldock.h jumpkeyhandler.h

FORMNAMES = aboutdlg.ui mainwin.ui coreaccounteditdlg.ui coreconnectdlg.ui bufferviewwidget.ui bufferwidget.ui nicklistwidget.ui settingsdlg.ui \
            settingspagedlg.ui topicwidget.ui debugconsole.ui inputwidget.ui \
            coreconfigwizardintropage.ui coreconfigwizardadminuserpage.ui coreconfigwizardstorageselectionpage.ui coreconfigwizardsyncpage.ui

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
