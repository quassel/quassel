                                                            
                                                                    
                                                                    
                                                                               







#keep the space lines above. nsis needs them, as it can only override bytes

isEmpty(QXTbase){
    unix:!macx: QXTbase = /usr/local/Qxt
    win32:      QXTbase = C:/libqxt
    macx :
}

isEmpty(QXTinclude): QXTinclude = $${QXTbase}/include/Qxt/
isEmpty(QXTbin):     QXTbin     = $${QXTbase}/bin

macx {
    isEmpty(QXTlib):     QXTlib = /Library/Frameworks
}

!macx {
    isEmpty(QXTlib):     QXTlib = $${QXTbase}/lib
}

INCLUDEPATH     += $${QXTinclude}
unix:!macx:LIBS += -Wl,-rpath,$${QXTlib}
macx:LIBS       += -F$${QXTlib}
LIBS            += -L$${QXTlib}


contains(QXT, crypto) {
    INCLUDEPATH       += $${QXTinclude}/QxtCrypto
    macx: INCLUDEPATH += $${QXTlib}/QxtCrypto.framework/HEADERS/
    macx:        LIBS += -framework QxtCrypto
    unix:!macx:  LIBS += -lQxtCrypto
    win32:       LIBS += -lQxtCrypto0
    QXT += core
}
contains(QXT, curses) {
    INCLUDEPATH       += $${QXTinclude}/QxtCurses
    macx: INCLUDEPATH += $${QXTlib}/QxtCurses.framework/HEADERS/
    macx:        LIBS += -framework QxtCurses
    unix:!macx:  LIBS += -lQxtCurses
    win32:       LIBS += -lQxtCurses0
    QXT += core
}
contains(QXT, web) {
    INCLUDEPATH       += $${QXTinclude}/QxtWeb
    macx: INCLUDEPATH += $${QXTlib}/QxtWeb.framework/HEADERS/
    macx:        LIBS += -framework QxtWeb
    unix:!macx:  LIBS += -lQxtWeb
    win32:       LIBS += -lQxtWeb0
    QXT += core network
    QT  += network 
}
contains(QXT, gui) {
    INCLUDEPATH       += $${QXTinclude}/QxtGui
    macx: INCLUDEPATH += $${QXTlib}/QxtGui.framework/HEADERS/
    macx:        LIBS += -framework QxtGui
    unix:!macx:  LIBS += -lQxtGui
    win32:       LIBS += -lQxtGui0
    QXT += core
    QT  += gui
}
contains(QXT, network) {
    INCLUDEPATH       += $${QXTinclude}/QxtNetwork
    macx: INCLUDEPATH += $${QXTlib}/QxtNetwork.framework/HEADERS/
    macx:        LIBS += -framework QxtNetwork
    unix:!macx:  LIBS += -lQxtNetwork
    win32:       LIBS += -lQxtNetwork0
    QXT += core
    QT  += network
}
contains(QXT, sql) {
    INCLUDEPATH       += $${QXTinclude}/QxtSql
    macx: INCLUDEPATH += $${QXTlib}/QxtSql.framework/HEADERS/
    macx:        LIBS += -framework QxtSql
    unix:!macx:  LIBS += -lQxtSql
    win32:       LIBS += -lQxtSql0
    QXT += core
    QT  += sql
}
contains(QXT, media) {
    INCLUDEPATH       += $${QXTinclude}/QxtMedia
    macx: INCLUDEPATH += $${QXTlib}/QxtMedia.framework/HEADERS/
    macx:        LIBS += -framework QxtMedia
    unix:!macx:  LIBS += -lQxtMedia
    win32:       LIBS += -lQxtMedia0
    QXT += core
}
contains(QXT, core) {
    INCLUDEPATH       += $${QXTinclude}/QxtCore
    macx: INCLUDEPATH += $${QXTlib}/QxtCore.framework/HEADERS/
    macx:        LIBS += -framework QxtCore
    unix:!macx:  LIBS += -lQxtCore
    win32:       LIBS += -lQxtCore0
}


