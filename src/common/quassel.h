/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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
#include <QLocale>
#include <QString>

#include "abstractcliparser.h"

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

  virtual ~Quassel();

  static void setupBuildInfo(const QString &generated);
  static inline const BuildInfo & buildInfo();
  static inline RunMode runMode();

  static QString configDirPath();

  //! Returns a list of data directory paths
  /** There are several locations for applications to install their data files in. On Unix,
  *  a common location is /usr/share; others include $PREFIX/share and additional directories
  *  specified in the env variable XDG_DATA_DIRS.
  *  \return A list of directory paths to look for data files in
  */
  static QStringList dataDirPaths();

  //! Searches for a data file in the possible data directories
  /** Data files can reside in $DATA_DIR/apps/quassel, where $DATA_DIR is one of the directories
  *  returned by \sa dataDirPaths().
  *  \Note With KDE integration enabled, files are searched (only) in KDE's appdata dirs.
  *  \return The full path to the data file if found; a null QString else
  */
  static QString findDataFilePath(const QString &filename);

  static QString translationDirPath();

  //! Returns a list of directories we look for scripts in
  /** We look for a subdirectory named "scripts" in the configdir and in all datadir paths.
  *   \return A list of directory paths containing executable scripts for /exec
  */
  static QStringList scriptDirPaths();

  static void loadTranslation(const QLocale &locale);

  static inline void setCliParser(AbstractCliParser *cliParser);
  static inline AbstractCliParser *cliParser();
  static inline QString optionValue(const QString &option);
  static inline bool isOptionSet(const QString &option);

  static const QString &coreDumpFileName();

  static bool DEBUG;

  static void logFatalMessage(const char *msg);

protected:
  Quassel();
  virtual bool init();
  virtual void quit();

  inline void setRunMode(RunMode mode);
  inline void setDataDirPaths(const QStringList &paths);
  QStringList findDataDirPaths() const;
  inline void disableCrashhandler();

private:
  void registerMetaTypes();

  static void handleSignal(int signal);
  static void logBacktrace(const QString &filename);

  static Quassel *_instance;
  static BuildInfo _buildInfo;
  static AbstractCliParser *_cliParser;
  static RunMode _runMode;
  static bool _initialized;
  static bool _handleCrashes;

  static QString _coreDumpFileName;
  static QString _configDirPath;
  static QStringList _dataDirPaths;
  static QString _translationDirPath;
};

const Quassel::BuildInfo & Quassel::buildInfo() { return _buildInfo; }
Quassel::RunMode Quassel::runMode() { return _runMode; }
void Quassel::setRunMode(Quassel::RunMode mode) { _runMode = mode; }
void Quassel::setDataDirPaths(const QStringList &paths) { _dataDirPaths = paths; }
void Quassel::disableCrashhandler() { _handleCrashes = false; }

void Quassel::setCliParser(AbstractCliParser *parser) { _cliParser = parser; }
AbstractCliParser *Quassel::cliParser() { return _cliParser; }
QString Quassel::optionValue(const QString &key) { return cliParser()->value(key); }
bool Quassel::isOptionSet(const QString &key) { return cliParser()->isSet(key); }

#endif
