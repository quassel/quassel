DEPMOD = common
QT_MOD = core network sql script

SRCS = core.cpp corebacklogmanager.cpp corebufferviewconfig.cpp corebufferviewmanager.cpp coresession.cpp coresettings.cpp networkconnection.cpp sqlitestorage.cpp abstractsqlstorage.cpp storage.cpp basichandler.cpp \
       ircserverhandler.cpp userinputhandler.cpp ctcphandler.cpp coreusersettings.cpp sessionthread.cpp
HDRS = core.h corebacklogmanager.h corebufferviewconfig.h corebufferviewmanager.h coresession.h coresettings.h networkconnection.h sqlitestorage.h abstractsqlstorage.h storage.h basichandler.h \
       ircserverhandler.h userinputhandler.h ctcphandler.h coreusersettings.h sessionthread.h

contains(QT_CONFIG, openssl) | contains(QT_CONFIG, openssl-linked) {
    SRCS += sslserver.cpp
    HDRS += sslserver.h
}
