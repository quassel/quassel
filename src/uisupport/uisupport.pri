DEPMOD = common client
QT_MOD = core gui network

SRCS += abstractbuffercontainer.cpp abstractitemview.cpp bufferview.cpp bufferviewfilter.cpp clearablelineedit.cpp colorbutton.cpp \
        nickviewfilter.cpp inputline.cpp nickview.cpp settingspage.cpp tabcompleter.cpp uisettings.cpp uistyle.cpp uistylesettings.cpp
HDRS += abstractbuffercontainer.h abstractitemview.h bufferview.h bufferviewfilter.h clearablelineedit.h colorbutton.h \
        nickviewfilter.h inputline.h nickview.h settingspage.h tabcompleter.h uisettings.h uistyle.h uistylesettings.h

FORMNAMES = 

for(ui, FORMNAMES) {
  FRMS += ui/$${ui}
}
