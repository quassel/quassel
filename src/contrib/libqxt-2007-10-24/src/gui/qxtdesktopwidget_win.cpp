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
#include "qxtdesktopwidget.h"
#include <qt_windows.h>

static WindowList qxt_Windows;

BOOL CALLBACK qxt_EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    Q_UNUSED(lParam);
    if (::IsWindowVisible(hwnd))
        qxt_Windows += hwnd;
    return TRUE;
}

WindowList QxtDesktopWidget::windows()
{
    qxt_Windows.clear();
    HDESK hdesk = ::OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    ::EnumDesktopWindows(hdesk, qxt_EnumWindowsProc, 0);
    ::CloseDesktop(hdesk);
    return qxt_Windows;
}

WId QxtDesktopWidget::activeWindow()
{
    return ::GetForegroundWindow();
}

WId QxtDesktopWidget::findWindow(const QString& title)
{
    std::wstring str = title.toStdWString();
    return ::FindWindow(NULL, str.c_str());
}

WId QxtDesktopWidget::windowAt(const QPoint& pos)
{
    POINT pt;
    pt.x = pos.x();
    pt.y = pos.y();
    return ::WindowFromPoint(pt);
}

QString QxtDesktopWidget::windowTitle(WId window)
{
    QString title;
    int len = ::GetWindowTextLength(window);
    if (len >= 0)
    {
        wchar_t* buf = new wchar_t[len+1];
        len = ::GetWindowText(window, buf, len+1);
        title = QString::fromStdWString(std::wstring(buf, len));
        delete[] buf;
    }
    return title;
}

QRect QxtDesktopWidget::windowGeometry(WId window)
{
    RECT rc;
    QRect rect;
    if (::GetWindowRect(window, &rc))
    {
        rect.setTop(rc.top);
        rect.setBottom(rc.bottom);
        rect.setLeft(rc.left);
        rect.setRight(rc.right);
    }
    return rect;
}
