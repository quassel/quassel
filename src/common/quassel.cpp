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

#if defined(HAVE_EXECINFO) && !defined(Q_OS_MAC)
#  define BUILD_CRASHHANDLER
#  include <execinfo.h>
#  include <dlfcn.h>
#  include <cxxabi.h>
#endif

Quassel::BuildInfo Quassel::_buildInfo;
CliParser *Quassel::_cliParser = 0;
Quassel::RunMode Quassel::_runMode;
bool Quassel::_initialized = false;
bool Quassel::DEBUG = false;
QString Quassel::_coreDumpFileName;

Quassel::Quassel() {
  Q_INIT_RESOURCE(i18n);

  // We catch SIGTERM and SIGINT (caused by Ctrl+C) to graceful shutdown Quassel.
  signal(SIGTERM, handleSignal);
  signal(SIGINT, handleSignal);

#ifdef BUILD_CRASHHANDLER
  signal(SIGABRT, handleSignal);
  signal(SIGBUS, handleSignal);
  signal(SIGSEGV, handleSignal);
#endif // #if defined(HAVE_EXECINFO) && !defined(Q_OS_MAC)

  _cliParser = new CliParser();

  // put shared client&core arguments here
  cliParser()->addSwitch("debug",'d', tr("Enable debug output"));
  cliParser()->addSwitch("help",'h', tr("Display this help and exit"));
}

Quassel::~Quassel() {
  delete _cliParser;
}

bool Quassel::init() {
  if(_initialized)
    return true;  // allow multiple invocations because of MonolithicApplication

  _initialized = true;
  qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

  registerMetaTypes();
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

void Quassel::setupBuildInfo(const QString &generated) {
  _buildInfo.applicationName = "Quassel IRC";
  _buildInfo.coreApplicationName = "Quassel Core";
  _buildInfo.clientApplicationName = "Quassel Client";
  _buildInfo.organizationName = "Quassel Project";
  _buildInfo.organizationDomain = "quassel-irc.org";

  QStringList gen = generated.split(',');
  Q_ASSERT(gen.count() == 10);
  _buildInfo.baseVersion = gen[0];
  _buildInfo.generatedVersion = gen[1];
  _buildInfo.isSourceDirty = !gen[2].isEmpty();
  _buildInfo.commitHash = gen[3];
  _buildInfo.commitDate = gen[4].toUInt();
  _buildInfo.protocolVersion = gen[5].toUInt();
  _buildInfo.clientNeedsProtocol = gen[6].toUInt();
  _buildInfo.coreNeedsProtocol = gen[7].toUInt();
  _buildInfo.buildDate = QString("%1 %2").arg(gen[8], gen[9]);
  // create a nice version string
  if(_buildInfo.generatedVersion.isEmpty()) {
    if(!_buildInfo.commitHash.isEmpty()) {
      // dist version
      _buildInfo.plainVersionString = QString("v%1 (dist-%2)")
                                        .arg(_buildInfo.baseVersion)
                                        .arg(_buildInfo.commitHash.left(7));
                                        _buildInfo.fancyVersionString
                                           = QString("v%1 (dist-<a href=\"http://git.quassel-irc.org/?p=quassel.git;a=commit;h=%3\">%2</a>)")
                                        .arg(_buildInfo.baseVersion)
                                        .arg(_buildInfo.commitHash.left(7))
                                        .arg(_buildInfo.commitHash);
    } else {
    // we only have a base version :(
      _buildInfo.plainVersionString = QString("v%1 (unknown rev)").arg(_buildInfo.baseVersion);
    }
  } else {
    // analyze what we got from git-describe
    QRegExp rx("(.*)-(\\d+)-g([0-9a-f]+)$");
    if(rx.exactMatch(_buildInfo.generatedVersion)) {
      QString distance = rx.cap(2) == "0" ? QString() : QString(" [+%1]").arg(rx.cap(2));
      _buildInfo.plainVersionString = QString("v%1%2 (git-%3%4)")
                                        .arg(rx.cap(1), distance, rx.cap(3))
                                        .arg(_buildInfo.isSourceDirty ? "*" : "");
      if(!_buildInfo.commitHash.isEmpty()) {
        _buildInfo.fancyVersionString = QString("v%1%2 (git-<a href=\"http://git.quassel-irc.org/?p=quassel.git;a=commit;h=%5\">%3</a>%4)")
                                          .arg(rx.cap(1), distance, rx.cap(3))
                                          .arg(_buildInfo.isSourceDirty ? "*" : "")
                                          .arg(_buildInfo.commitHash);
      }
    } else {
      _buildInfo.plainVersionString = QString("v%1 (invalid rev)").arg(_buildInfo.baseVersion);
    }
  }
  if(_buildInfo.fancyVersionString.isEmpty())
    _buildInfo.fancyVersionString = _buildInfo.plainVersionString;
}

//! Signal handler for graceful shutdown.
void Quassel::handleSignal(int sig) {
  switch(sig) {
    case SIGTERM:
    case SIGINT:
      qWarning("%s", qPrintable(QString("Caught signal %1 - exiting.").arg(sig)));
      QCoreApplication::quit();
      break;

#ifdef BUILD_CRASHHANDLER
    case SIGABRT:
    case SIGBUS:
    case SIGSEGV:
      handleCrash();
#endif
      break;
    default:
      break;
  }
}

void Quassel::logFatalMessage(const char *msg) {
#ifndef Q_OS_MAC
  QFile dumpFile(coreDumpFileName());
  dumpFile.open(QIODevice::WriteOnly);
  QTextStream dumpStream(&dumpFile);
#else
  QTextStream dumpStream(stderr);
#endif
  
  dumpStream << "Fatal: " << msg << '\n';
  dumpStream.flush();

  qInstallMsgHandler(0);
  abort();
}

void Quassel::handleCrash() {
#ifdef BUILD_CRASHHANDLER
  void* callstack[128];
  int i, frames = backtrace(callstack, 128);

  QFile dumpFile(coreDumpFileName());
  dumpFile.open(QIODevice::Append);
  QTextStream dumpStream(&dumpFile);

  dumpStream << "Quassel IRC: " << _buildInfo.baseVersion << ' ' << _buildInfo.commitHash << '\n';
    
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
#endif /* BUILD_CRASHHANDLER */
}

const QString &Quassel::coreDumpFileName() {
  if(_coreDumpFileName.isEmpty())
    _coreDumpFileName = QString("Quassel-Crash-%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmm"));

  return _coreDumpFileName;
}
