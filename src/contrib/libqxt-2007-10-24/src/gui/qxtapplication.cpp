/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtGui module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/
#include "qxtapplication.h"
#include "qxtapplication_p.h"
#include <QWidget>

/*!
    \class QxtApplication QxtApplication
    \ingroup QxtGui
    \brief An extended QApplication with support for hotkeys aka "global shortcuts".

    QxtApplication introduces hotkeys which trigger even if the application is not
    active. This makes it easy to implement applications that react to certain
    shortcuts still if some other application is active or if the application is
    for example minimized to the system tray.

    QxtApplication also lets you install native event filters. This makes it
    possible to access platform specific native events without subclassing
    QApplication.
 */

/*!
    \fn QxtApplication::instance()

    Returns a pointer to the instance of the application object.

    A convenience macro \b qxtApp is also available.
 */

QxtApplication::QxtApplication(int& argc, char** argv)
        : QApplication(argc, argv)
{}

QxtApplication::QxtApplication(int& argc, char** argv, bool GUIenabled)
        : QApplication(argc, argv, GUIenabled)
{}

QxtApplication::QxtApplication(int& argc, char** argv, Type type)
        : QApplication(argc, argv, type)
{}

QxtApplication::~QxtApplication()
{}

/*!
    Installs a native event \a filter.

    A native event filter is an object that receives all native events before they reach
    the application object. The filter can either stop the native event or forward it to
    the application object. The filter receives native events via its platform specific
    native event filter function. The native event filter function must return \b true
    if the event should be filtered, (i.e. stopped); otherwise it must return \b false.

    If multiple native event filters are installed, the filter that was installed last
    is activated first.

    \sa removeNativeEventFilter()
*/
void QxtApplication::installNativeEventFilter(QxtNativeEventFilter* filter)
{
    if (!filter)
        return;

    qxt_d().nativeFilters.removeAll(filter);
    qxt_d().nativeFilters.prepend(filter);
}

/*!
    Removes a native event \a filter. The request is ignored if such a native
    event filter has not been installed.

    \sa installNativeEventFilter()
*/
void QxtApplication::removeNativeEventFilter(QxtNativeEventFilter* filter)
{
    qxt_d().nativeFilters.removeAll(filter);
}

/*!
    Adds a hotkey using \a modifiers and \a key. The \a member
    of \a receiver is invoked upon hotkey trigger.

    \return \b true if hotkey registration succeed, \b false otherwise.

    Example usage:
    \code
    QxtLabel* label = new QxtLabel("Hello world!");
    qxtApp->addHotKey(Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_S, label, "show");
    \endcode
*/
bool QxtApplication::addHotKey(Qt::KeyboardModifiers modifiers, Qt::Key key, QWidget* receiver, const char* member)
{
    Q_ASSERT(receiver);
    Q_ASSERT(member);
    uint mods = qxt_d().nativeModifiers(modifiers);
    uint keycode = qxt_d().nativeKeycode(key);
    if (keycode)
    {
        qxt_d().hotkeys.insert(qMakePair(mods, keycode), qMakePair(receiver, member));
        return qxt_d().registerHotKey(mods, keycode, receiver);
    }
    return false;
}

/*!
    Removes the hotkey using \a modifiers and \a key mapped to
    \a member of \a receiver.

    \return \b true if hotkey unregistration succeed, \b false otherwise.
*/
bool QxtApplication::removeHotKey(Qt::KeyboardModifiers modifiers, Qt::Key key, QWidget* receiver, const char* member)
{
    Q_ASSERT(receiver);
    Q_UNUSED(member);
    uint mods = qxt_d().nativeModifiers(modifiers);
    uint keycode = qxt_d().nativeKeycode(key);
    if (keycode)
    {
        qxt_d().hotkeys.remove(qMakePair(mods, keycode));
        return qxt_d().unregisterHotKey(mods, keycode, receiver);
    }
    return false;
}

void QxtApplicationPrivate::activateHotKey(uint modifiers, uint keycode) const
{
    Receivers receivers = hotkeys.values(qMakePair(modifiers, keycode));
    foreach (Receiver receiver, receivers)
    {
        // QMetaObject::invokeMethod() has appropriate null checks
        QMetaObject::invokeMethod(receiver.first, receiver.second);
    }
}
