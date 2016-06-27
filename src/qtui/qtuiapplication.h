/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef QTUIAPPLICATION_H_
#define QTUIAPPLICATION_H_

#ifdef HAVE_KDE4
#  include <KApplication>
#else
#  include <QApplication>
#endif

#include <QSessionManager>

#include "quassel.h"
#include "uisettings.h"
#include "qtuisettings.h"

class QtUi;

#ifdef HAVE_KDE4
class QtUiApplication : public KApplication, public Quassel
{
#else
class QtUiApplication : public QApplication, public Quassel
{
#endif

    Q_OBJECT

public:
    QtUiApplication(int &, char **);
    ~QtUiApplication();
    virtual bool init();

    void resumeSessionIfPossible();
    inline bool isAboutToQuit() const { return _aboutToQuit; }

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    void commitData(QSessionManager &manager) override;
    void saveState(QSessionManager &manager) override;
#else
    void commitData(QSessionManager &manager);
    void saveState(QSessionManager &manager);
#endif

protected:
    virtual void quit();

private:
    /**
     * Migrate settings if neccessary and possible
     *
     * If unsuccessful (major version changed, minor version upgrade failed), returning false, the
     * settings are in an unknown state and the client should quit.
     *
     * @return True if settings successfully migrated, otherwise false
     */
    bool migrateSettings();

    /**
     * Migrate from one minor settings version to the next
     *
     * Settings can only be migrated one version at a time.  Start from the current version, calling
     * this function for each intermediate version up until the latest version.
     *
     * @param[in] settings    Current settings instance
     * @param[in] newVersion  Next target version for migration, at most 1 from the current version
     * @return True if minor revision of settings successfully migrated, otherwise false
     */
    bool applySettingsMigration(QtUiSettings settings, const uint newVersion);

    bool _aboutToQuit;
};


#endif
