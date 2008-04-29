CONFIG += warn_on uic resources qt silent
# CONFIG += incremental link_prl nostrip qt_no_framework

release {
  CONFIG *= release strip
} else {
  CONFIG *= debug
}

win32 { 
  CONFIG = warn_on uic resources qt silent windows
  static { 
    CONFIG += release static
  }
}

mac:Tiger {
 QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
 CONFIG += x86 ppc
}

sputdev {
  DEFINES *= SPUTDEV
}
