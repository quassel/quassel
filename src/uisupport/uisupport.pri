DEPMOD = common client
QT_MOD = core gui network

SRCS += bufferview.cpp bufferviewfilter.cpp uistyle.cpp
HDRS += bufferview.h bufferviewfilter.h uistyle.h

FORMNAMES = 

for(ui, FORMNAMES) {
  FRMS += ui/$${ui}
}
