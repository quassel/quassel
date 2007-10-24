DEPMOD = common
QT_MOD = core gui

SRCS += uistyle.cpp
HDRS += uistyle.h

FORMNAMES = 

for(ui, FORMNAMES) {
  FRMS += ui/$${ui}
}
