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
#ifndef QXTAPPLICATION_H
#define QXTAPPLICATION_H

#include <QApplication>
#include "qxtglobal.h"
#include "qxtpimpl.h"

class QxtApplicationPrivate;
class QxtNativeEventFilter;

#define qxtApp (QxtApplication::instance())

class QXT_GUI_EXPORT QxtApplication : public QApplication
{
    Q_OBJECT
    QXT_DECLARE_PRIVATE(QxtApplication);

public:
    QxtApplication(int& argc, char** argv);
    QxtApplication(int& argc, char** argv, bool GUIenabled);
    QxtApplication(int& argc, char** argv, Type type);
#if defined(Q_WS_X11)
    QxtApplication(Display* display, Qt::HANDLE visual = 0, Qt::HANDLE colormap = 0);
    QxtApplication(Display* display, int& argc, char** argv, Qt::HANDLE visual = 0, Qt::HANDLE colormap = 0);
#endif // Q_WS_X11
    virtual ~QxtApplication();

    void installNativeEventFilter(QxtNativeEventFilter* filter);
    void removeNativeEventFilter(QxtNativeEventFilter* filter);

#ifndef QXT_DOXYGEN_RUN
#if defined(Q_WS_X11)
    virtual bool x11EventFilter(XEvent* event);
#elif defined(Q_WS_WIN)
    virtual bool winEventFilter(MSG* msg, long* result);
#elif defined(Q_WS_MAC)
    virtual bool macEventFilter(EventHandlerCallRef caller, EventRef event);
#endif // Q_WS_*
#endif // QXT_DOXYGEN_RUN

    bool addHotKey(Qt::KeyboardModifiers modifiers, Qt::Key key, QWidget* receiver, const char* member);
    bool removeHotKey(Qt::KeyboardModifiers modifiers, Qt::Key key, QWidget* receiver, const char* member);

    inline static QxtApplication* instance()
    {
        return static_cast<QxtApplication*>(QApplication::instance());
    }
};

#endif // QXTAPPLICATION_H
