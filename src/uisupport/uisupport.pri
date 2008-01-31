DEPMOD = common client
QT_MOD = core gui network

SRCS += bufferview.cpp bufferviewfilter.cpp nickviewfilter.cpp inputline.cpp nickview.cpp settingspage.cpp tabcompleter.cpp uisettings.cpp uistyle.cpp uistylesettings.cpp
HDRS += bufferview.h bufferviewfilter.h nickviewfilter.h inputline.h nickview.h settingspage.h tabcompleter.h uisettings.h uistyle.h uistylesettings.h

FORMNAMES = 

for(ui, FORMNAMES) {
  FRMS += ui/$${ui}
}
