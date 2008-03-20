CONFIG += warn_on uic resources qt
# CONFIG += incremental link_prl nostrip qt_no_framework

release {
  CONFIG *= release strip
} else {
  CONFIG *= debug
}

win32 { 
  CONFIG = release warn_on uic resources qt windows static
}
