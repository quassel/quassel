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
#include <QKeySequence>
#include <QX11Info>
#include <X11/Xlib.h>

QxtApplication::QxtApplication(Display* display, Qt::HANDLE visual, Qt::HANDLE colormap)
        : QApplication(display, visual, colormap)
{}

QxtApplication::QxtApplication(Display* display, int& argc, char** argv, Qt::HANDLE visual, Qt::HANDLE colormap)
        : QApplication(display, argc, argv, visual, colormap)
{}

bool QxtApplication::x11EventFilter(XEvent* event)
{
    foreach (QxtNativeEventFilter* filter, qxt_d().nativeFilters)
    {
        if (filter && filter->x11EventFilter(event))
            return true;
    }

    if (event->type == KeyPress)
    {
        XKeyEvent* key = (XKeyEvent*) event;
        qxt_d().activateHotKey(key->state, key->keycode);
    }
    return QApplication::x11EventFilter(event);
}

uint QxtApplicationPrivate::nativeModifiers(Qt::KeyboardModifiers modifiers) const
{
    // ShiftMask, LockMask, ControlMask, Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, and Mod5Mask
    uint native = 0;
    if (modifiers & Qt::ShiftModifier)
        native |= ShiftMask;
    if (modifiers & Qt::ControlModifier)
        native |= ControlMask;
    if (modifiers & Qt::AltModifier)
        native |= Mod1Mask;
    // TODO: resolve these?
    //if (modifiers & Qt::MetaModifier)
    //if (modifiers & Qt::KeypadModifier)
    //if (modifiers & Qt::GroupSwitchModifier)
    return native;
}

uint QxtApplicationPrivate::nativeKeycode(Qt::Key key) const
{
    Display* display = QX11Info::display();
    return XKeysymToKeycode(display, XStringToKeysym(QKeySequence(key).toString().toLatin1().data()));
}

bool QxtApplicationPrivate::registerHotKey(uint modifiers, uint keycode, QWidget* receiver)
{
    Q_UNUSED(receiver);
    Display* display = QX11Info::display();
    Window window = QX11Info::appRootWindow();
    Bool owner = True;
    int pointer = GrabModeAsync;
    int keyboard = GrabModeAsync;
    return XGrabKey(display, keycode, modifiers, window, owner, pointer, keyboard);
}

bool QxtApplicationPrivate::unregisterHotKey(uint modifiers, uint keycode, QWidget* receiver)
{
    Q_UNUSED(receiver);
    Display* display = QX11Info::display();
    Window window = QX11Info::appRootWindow();
    return XUngrabKey(display, keycode, modifiers, window);
}
