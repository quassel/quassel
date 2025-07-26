// SPDX-FileCopyrightText: 2005-2025 Quassel Project <devel@quassel-irc.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MACOSUTILS_H
#define MACOSUTILS_H

#include <QtGlobal>

#ifdef Q_OS_MAC

quint32 getMacCommandUnicode();
quint32 getMacControlUnicode();
quint32 getMacOptionUnicode();
quint32 getMacShiftUnicode();

#else

// Define dummy values for non-macOS platforms
inline quint32 getMacCommandUnicode()
{
    return 0;
}
inline quint32 getMacControlUnicode()
{
    return 0;
}
inline quint32 getMacOptionUnicode()
{
    return 0;
}
inline quint32 getMacShiftUnicode()
{
    return 0;
}

#endif

#endif  // MACOSUTILS_H
