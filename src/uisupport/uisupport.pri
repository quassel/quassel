DEPMOD = common client
QT_MOD = core gui network

SRCS += bufferview.cpp bufferviewfilter.cpp nickview.cpp uistyle.cpp
HDRS += bufferview.h bufferviewfilter.h nickview.h uistyle.h

FORMNAMES = 

for(ui, FORMNAMES) {
  FRMS += ui/$${ui}
}
