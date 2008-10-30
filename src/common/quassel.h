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

#ifndef QUASSEL_H_
#define QUASSEL_H_

#include <QCoreApplication>
#include <QString>

#include "cliparser.h"

class Quassel {
  Q_DECLARE_TR_FUNCTIONS(Quassel)

public:
  enum RunMode {
    Monolithic,
    ClientOnly,
    CoreOnly
  };

  struct BuildInfo {
    QString fancyVersionString; // clickable rev
    QString plainVersionString; // no <a> tag

    QString baseVersion;
    QString generatedVersion;
    QString commitHash;
    uint commitDate;
    QString buildDate;
    bool isSourceDirty;
    uint protocolVersion;
    uint clientNeedsProtocol;
    uint coreNeedsProtocol;

    QString applicationName;
    QString coreApplicationName;
    QString clientApplicationName;
    QString organizationName;
    QString organizationDomain;
  };

  void setupBuildInfo(const QString &generated);

  virtual ~Quassel();

  static inline const BuildInfo & buildInfo();
  static inline RunMode runMode();

  static inline CliParser *cliParser();
  static inline QString optionValue(const QString &option);
  static inline bool isOptionSet(const QString &option);

  static const QString &coreDumpFileName();

  static bool DEBUG;

  static void logFatalMessage(const char *msg);

protected:
  Quassel();
  virtual bool init();

  inline void setRunMode(RunMode mode);

private:
  void setupTranslations();
  void registerMetaTypes();

  static void handleSignal(int signal);
  static void handleCrash();

  static BuildInfo _buildInfo;
  static CliParser *_cliParser;
  static RunMode _runMode;
  static bool _initialized;

  static QString _coreDumpFileName;
};

const Quassel::BuildInfo & Quassel::buildInfo() { return _buildInfo; }
Quassel::RunMode Quassel::runMode() { return _runMode; }
void Quassel::setRunMode(Quassel::RunMode mode) { _runMode = mode; }

CliParser *Quassel::cliParser() { return _cliParser; }
QString Quassel::optionValue(const QString &key) { return cliParser()->value(key); }
bool Quassel::isOptionSet(const QString &key) { return cliParser()->isSet(key); }

#endif
