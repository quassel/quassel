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

#include "coreapplication.h"

#include "core.h"
#include "logger.h"

CoreApplicationInternal::CoreApplicationInternal()
    : _coreCreated(false)
{
}


CoreApplicationInternal::~CoreApplicationInternal()
{
    if (_coreCreated) {
        Core::saveState();
        Core::destroy();
    }
}


bool CoreApplicationInternal::init()
{
    /* FIXME
    This is an initial check if logfile is writable since the warning would spam stdout if done
    in current Logger implementation. Can be dropped whenever the logfile is only opened once.
    */
    QFile logFile;
    if (!Quassel::optionValue("logfile").isEmpty()) {
        logFile.setFileName(Quassel::optionValue("logfile"));
        if (!logFile.open(QIODevice::Append | QIODevice::Text))
            qWarning("Warning: Couldn't open logfile '%s' - will log to stdout instead", qPrintable(logFile.fileName()));
        else logFile.close();
    }

    Core::instance(); // create and init the core
    _coreCreated = true;

    Quassel::registerReloadHandler([]() {
        // Currently, only reloading SSL certificates is supported
        return Core::reloadCerts();
    });

    if (!Quassel::isOptionSet("norestore"))
        Core::restoreState();

    return true;
}


/*****************************************************************************/

CoreApplication::CoreApplication(int &argc, char **argv)
    : QCoreApplication(argc, argv)
{
#ifdef Q_OS_MAC
    Quassel::disableCrashHandler();
#endif /* Q_OS_MAC */

    Quassel::setRunMode(Quassel::CoreOnly);
    _internal = new CoreApplicationInternal();
}


CoreApplication::~CoreApplication()
{
    delete _internal;
    Quassel::destroy();
}


bool CoreApplication::init()
{
    if (Quassel::init() && _internal->init()) {
#if QT_VERSION < 0x050000
        qInstallMsgHandler(Logger::logMessage);
#else
        qInstallMessageHandler(Logger::logMessage);
#endif
        return true;
    }
    return false;
}
