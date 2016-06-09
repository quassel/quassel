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

#ifndef MONOAPPLICATION_H_
#define MONOAPPLICATION_H_

#include "qtuiapplication.h"

// Timeout for core session cleanup
#include <QTimer>

class CoreApplicationInternal;

class MonolithicApplication : public QtUiApplication
{
    Q_OBJECT
public:
    MonolithicApplication(int &, char **);
    ~MonolithicApplication();

    bool init();

protected:
    /**
     * Requests an orderly shutdown of the application, cleaning up active sessions as needed.
     *
     * @see Quassel::beginQuittingApp()
     */
    virtual void beginQuittingApp();

private slots:
    void startInternalCore();

    /**
     * Requests an orderly shutdown of the application, cleaning up active sessions as needed.
     *
     * Implementation as a slot allows connecting signals from QtUiApplication.
     *
     * @see MonolithicApplication::beginQuittingApp()
     */
    void shutdownInternalCore();

    /**
     * Signifies all sessions have been disconnected and cleaned up, or timeout has reached.
     */
    void coreSessionsFinish();

private:
    CoreApplicationInternal *_internal;
    bool _internalInitDone;
};


#endif
