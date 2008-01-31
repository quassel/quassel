CONFIG += debug warn_on uic resources qt
# CONFIG += incremental link_prl nostrip qt_no_framework

win32{ 
CONFIG = release warn_on uic resources qt windows static
}
