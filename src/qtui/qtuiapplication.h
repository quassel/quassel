/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
    bool _aboutToQuit;
};


#endif
