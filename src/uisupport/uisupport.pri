DEPMOD = common client
QT_MOD = core gui network

SRCS += bufferview.cpp bufferviewfilter.cpp inputline.cpp nickview.cpp tabcompleter.cpp uistyle.cpp
HDRS += bufferview.h bufferviewfilter.h inputline.h nickview.h tabcompleter.h uistyle.h

FORMNAMES = 

for(ui, FORMNAMES) {
  FRMS += ui/$${ui}
}
