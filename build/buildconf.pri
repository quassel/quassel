# This file contains global build settings. Note that you can add stuff to CONFIG
# by using qmake -config stuff 
# Notable examples:
# 
# -config debug (or release or debug_and_release)
# -config verbose (to enable verbose compiling)

CONFIG += warn_on uic resources qt silent

verbose {
  CONFIG -= silent
}

win32 { 
  static {
    CONFIG = release warn_on uic resources qt windows static
  } else {
    CONFIG = warn_on uic resources qt silent windows
  }
}

mac:Tiger {
 QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
 CONFIG += x86 ppc
}

sputdev {
  DEFINES *= SPUTDEV
}
