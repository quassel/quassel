/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "quassel.h"

#include <signal.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QObject>
#include <QMetaType>

#include "message.h"
#include "identity.h"
#include "network.h"
#include "bufferinfo.h"
#include "types.h"
#include "syncableobject.h"

#if defined(HAVE_EXECINFO) and not defined(Q_OS_MAC)
#  include <execinfo.h>
#  include <dlfcn.h>
#  include <cxxabi.h>
#endif

Quassel::BuildInfo Quassel::_buildInfo;
CliParser *Quassel::_cliParser = 0;
Quassel::RunMode Quassel::_runMode;
bool Quassel::_initialized = false;
bool Quassel::DEBUG = false;

Quassel::Quassel() {
  // We catch SIGTERM and SIGINT (caused by Ctrl+C) to graceful shutdown Quassel.
  signal(SIGTERM, handleSignal);
  signal(SIGINT, handleSignal);

#if defined(HAVE_EXECINFO) and not defined(Q_OS_MAC)
  signal(SIGABRT, handleSignal);
  signal(SIGBUS, handleSignal);
  signal(SIGSEGV, handleSignal);
#endif // #if defined(HAVE_EXECINFO) and not defined(Q_OS_MAC)

  _cliParser = new CliParser();

  // put shared client&core arguments here
  cliParser()->addSwitch("debug",'d', tr("Enable debug output"));
  cliParser()->addSwitch("help",'h', tr("Display this help and exit"));
}

Quassel::~Quassel() {
  delete _cliParser;
}

bool Quassel::init() {
  if(_initialized) return true;  // allow multiple invocations because of MonolithicApplication

  qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

  registerMetaTypes();
  setupBuildInfo();
  Global::setupVersion();
  setupTranslations();

  QCoreApplication::setApplicationName(buildInfo().applicationName);
  QCoreApplication::setOrganizationName(buildInfo().organizationName);
  QCoreApplication::setOrganizationDomain(buildInfo().organizationDomain);

  Network::setDefaultCodecForServer("ISO-8859-1");
  Network::setDefaultCodecForEncoding("UTF-8");
  Network::setDefaultCodecForDecoding("ISO-8859-15");

  if(!cliParser()->parse(QCoreApplication::arguments()) || isOptionSet("help")) {
    cliParser()->usage();
    return false;
  }
  DEBUG = isOptionSet("debug");
  return true;
}

//! Register our custom types with Qt's Meta Object System.
/**  This makes them available for QVariant and in signals/slots, among other things.
*
*/
void Quassel::registerMetaTypes() {
  // Complex types
  qRegisterMetaType<QVariant>("QVariant");
  qRegisterMetaType<Message>("Message");
  qRegisterMetaType<BufferInfo>("BufferInfo");
  qRegisterMetaType<NetworkInfo>("NetworkInfo");
  qRegisterMetaType<Identity>("Identity");
  qRegisterMetaType<Network::ConnectionState>("Network::ConnectionState");

  qRegisterMetaTypeStreamOperators<QVariant>("QVariant");
  qRegisterMetaTypeStreamOperators<Message>("Message");
  qRegisterMetaTypeStreamOperators<BufferInfo>("BufferInfo");
  qRegisterMetaTypeStreamOperators<NetworkInfo>("NetworkInfo");
  qRegisterMetaTypeStreamOperators<Identity>("Identity");
  qRegisterMetaTypeStreamOperators<qint8>("Network::ConnectionState");

  qRegisterMetaType<IdentityId>("IdentityId");
  qRegisterMetaType<BufferId>("BufferId");
  qRegisterMetaType<NetworkId>("NetworkId");
  qRegisterMetaType<UserId>("UserId");
  qRegisterMetaType<AccountId>("AccountId");
  qRegisterMetaType<MsgId>("MsgId");

  qRegisterMetaTypeStreamOperators<IdentityId>("IdentityId");
  qRegisterMetaTypeStreamOperators<BufferId>("BufferId");
  qRegisterMetaTypeStreamOperators<NetworkId>("NetworkId");
  qRegisterMetaTypeStreamOperators<UserId>("UserId");
  qRegisterMetaTypeStreamOperators<AccountId>("AccountId");
  qRegisterMetaTypeStreamOperators<MsgId>("MsgId");
}

void Quassel::setupTranslations() {
  // Set up i18n support
  QLocale locale = QLocale::system();

  QTranslator *qtTranslator = new QTranslator(qApp);
  qtTranslator->setObjectName("QtTr");
  qtTranslator->load(QString(":i18n/qt_%1").arg(locale.name()));
  qApp->installTranslator(qtTranslator);

  QTranslator *quasselTranslator = new QTranslator(qApp);
  quasselTranslator->setObjectName("QuasselTr");
  quasselTranslator->load(QString(":i18n/quassel_%1").arg(locale.name()));
  qApp->installTranslator(quasselTranslator);
}

void Quassel::setupBuildInfo() {
  _buildInfo.applicationName = "Quassel IRC";
  _buildInfo.coreApplicationName = "Quassel Core";
  _buildInfo.clientApplicationName = "Quassel Client";
  _buildInfo.organizationName = "Quassel Project";
  _buildInfo.organizationDomain = "quassel-irc.org";
/*
#  include "version.inc"
#  include "version.gen"

  if(quasselGeneratedVersion.isEmpty()) {
    if(quasselCommit.isEmpty())
      quasselVersion = QString("v%1 (unknown rev)").arg(quasselBaseVersion);
    else
      quasselVersion = QString("v%1 (dist-%2, %3)").arg(quasselBaseVersion).arg(quasselCommit.left(7))
      .arg(QDateTime::fromTime_t(quasselArchiveDate).toLocalTime().toString("yyyy-MM-dd"));
  } else {
    QStringList parts = quasselGeneratedVersion.split(':');
    quasselVersion = QString("v%1").arg(parts[0]);
    if(parts.count() >= 2) quasselVersion.append(QString(" (%1)").arg(parts[1]));
  }
  quasselBuildDate = __DATE__;
  quasselBuildTime = __TIME__;
  */
}

//! Signal handler for graceful shutdown.
void Quassel::handleSignal(int sig) {
  switch(sig) {
    case SIGTERM:
    case SIGINT:
      qWarning("%s", qPrintable(QString("Caught signal %1 - exiting.").arg(sig)));
      QCoreApplication::quit();
      break;

    case SIGABRT:
    case SIGBUS:
    case SIGSEGV:
      handleCrash();
      break;
    default:
      break;
  }
}

void Quassel::handleCrash() {
#if defined(HAVE_EXECINFO) and not defined(Q_OS_MAC)
  void* callstack[128];
  int i, frames = backtrace(callstack, 128);

  QFile dumpFile(QString("Quassel-Crash-%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmm.log")));
  dumpFile.open(QIODevice::WriteOnly);
  QTextStream dumpStream(&dumpFile);

  for (i = 0; i < frames; ++i) {
    Dl_info info;
    dladdr (callstack[i], &info);
    // as a reference:
    //     typedef struct
    //     {
      //       __const char *dli_fname;   /* File name of defining object.  */
    //       void *dli_fbase;           /* Load address of that object.  */
    //       __const char *dli_sname;   /* Name of nearest symbol.  */
    //       void *dli_saddr;           /* Exact value of nearest symbol.  */
    //     } Dl_info;

    #if __LP64__
    int addrSize = 16;
    #else
    int addrSize = 8;
    #endif

    QString funcName;
    if(info.dli_sname) {
      char *func = abi::__cxa_demangle(info.dli_sname, 0, 0, 0);
      if(func) {
        funcName = QString(func);
        free(func);
      } else {
        funcName = QString(info.dli_sname);
      }
    } else {
      funcName = QString("0x%1").arg((long)info.dli_saddr, addrSize, QLatin1Char('0'));
    }

    // prettificating the filename
    QString fileName("???");
    if(info.dli_fname) {
      fileName = QString(info.dli_fname);
      int slashPos = fileName.lastIndexOf('/');
      if(slashPos != -1)
        fileName = fileName.mid(slashPos + 1);
      if(fileName.count() < 20)
        fileName += QString(20 - fileName.count(), ' ');
    }

    QString debugLine = QString("#%1 %2 0x%3 %4").arg(i, 3, 10)
    .arg(fileName)
    .arg((long)(callstack[i]), addrSize, 16, QLatin1Char('0'))
    .arg(funcName);

    dumpStream << debugLine << "\n";
    qDebug() << qPrintable(debugLine);
  }
  dumpFile.close();
  exit(27);
#endif // #if defined(HAVE_EXECINFO) and not defined(Q_OS_MAC)
}

// FIXME temporary

void Global::setupVersion() {

  #  include "version.inc"
  #  include "version.gen"

  if(quasselGeneratedVersion.isEmpty()) {
    if(quasselCommit.isEmpty())
      quasselVersion = QString("v%1 (unknown rev)").arg(quasselBaseVersion);
    else
      quasselVersion = QString("v%1 (dist-%2, %3)").arg(quasselBaseVersion).arg(quasselCommit.left(7))
      .arg(QDateTime::fromTime_t(quasselArchiveDate).toLocalTime().toString("yyyy-MM-dd"));
  } else {
    QStringList parts = quasselGeneratedVersion.split(':');
    quasselVersion = QString("v%1").arg(parts[0]);
    if(parts.count() >= 2) quasselVersion.append(QString(" (%1)").arg(parts[1]));
  }
  quasselBuildDate = __DATE__;
  quasselBuildTime = __TIME__;
}

QString Global::quasselVersion;
QString Global::quasselBaseVersion;
QString Global::quasselGeneratedVersion;
QString Global::quasselBuildDate;
QString Global::quasselBuildTime;
QString Global::quasselCommit;
uint Global::quasselArchiveDate;
uint Global::protocolVersion;
uint Global::clientNeedsProtocol;
uint Global::coreNeedsProtocol;
