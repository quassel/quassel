// SPDX-FileCopyrightText: 2005-2025 Quassel Project <devel@quassel-irc.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "macosutils.h"

#ifdef Q_OS_MAC
#include <QtGlobal>
#include <Foundation/Foundation.h>
#include <AppKit/NSEvent.h> // For modern key constants

quint32 getMacCommandUnicode() {
    return 0x2318; // ⌘ Command key Unicode
}

quint32 getMacControlUnicode() {
    return 0x2303; // ⌃ Control key Unicode  
}

quint32 getMacOptionUnicode() {
    return 0x2325; // ⌥ Option key Unicode
}

quint32 getMacShiftUnicode() {
    return 0x21E7; // ⇧ Shift key Unicode
}
#endif
