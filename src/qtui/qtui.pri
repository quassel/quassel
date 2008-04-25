DEPMOD = client common uisupport
QT_MOD = core gui network

SRCS += aboutdlg.cpp bufferwidget.cpp chatline-old.cpp chatwidget.cpp \
        coreconfigwizard.cpp coreconnectdlg.cpp configwizard.cpp debugconsole.cpp inputwidget.cpp \
        mainwin.cpp nicklistwidget.cpp qtui.cpp qtuisettings.cpp qtuistyle.cpp settingsdlg.cpp settingspagedlg.cpp \
        titlesetter.cpp topicbutton.cpp topicwidget.cpp verticaldock.cpp jumpkeyhandler.cpp

HDRS += aboutdlg.h bufferwidget.h chatline-old.h chatwidget.h \
        coreconfigwizard.h configwizard.h debugconsole.h inputwidget.h \
        coreconnectdlg.h mainwin.h nicklistwidget.h qtui.h qtuisettings.h qtuistyle.h settingsdlg.h settingspagedlg.h \
        titlesetter.h topicbutton.h topicwidget.h verticaldock.h jumpkeyhandler.h

# new chatline model stuff
sputdev {
  SRCS += chatitem.cpp chatline.cpp chatlinemodel.cpp chatscene.cpp chatview.cpp
  HDRS += chatitem.h chatline.h chatlinemodel.h chatscene.h chatview.h
  SRCS -= chatline-old.cpp chatwidget.cpp
  HDRS -= chatline-old.h chatwidget.h
}

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
