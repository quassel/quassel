// SPDX-FileCopyrightText: 2005-2025 Quassel Project <devel@quassel-irc.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "macosutils.h"

#ifdef Q_OS_MAC
#include <QtGlobal>
#include <Foundation/Foundation.h>
#include <Carbon/Carbon.h> // For kCommandUnicode, kControlUnicode, etc.

quint32 getMacCommandUnicode() {
    return kCommandUnicode;
}

quint32 getMacControlUnicode() {
    return kControlUnicode;
}

quint32 getMacOptionUnicode() {
    return kOptionUnicode;
}

quint32 getMacShiftUnicode() {
    return kShiftUnicode;
}
#endif
