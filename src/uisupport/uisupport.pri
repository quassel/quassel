DEPMOD = common client
QT_MOD = core gui network

SRCS += bufferview.cpp bufferviewfilter.cpp inputline.cpp nickview.cpp tabcompleter.cpp uisettings.cpp uistyle.cpp
HDRS += bufferview.h bufferviewfilter.h inputline.h nickview.h tabcompleter.h uisettings.h uistyle.h

FORMNAMES = 

for(ui, FORMNAMES) {
  FRMS += ui/$${ui}
}
