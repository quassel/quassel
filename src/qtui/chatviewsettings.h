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

#ifndef CHATVIEWSETTINGS_H
#define CHATVIEWSETTINGS_H

#include "qtuisettings.h"

class ChatScene;
class ChatView;

class ChatViewSettings : public QtUiSettings
{
public:
    Q_ENUMS(OperationMode)
public:
    enum OperationMode {
        InvalidMode = 0,
        OptIn = 1,
        OptOut = 2
    };
    Q_DECLARE_FLAGS(operationModes, OperationMode);

    ChatViewSettings(const QString &id = "__default__");
    ChatViewSettings(ChatScene *scene);
    ChatViewSettings(ChatView *view);

    inline bool showWebPreview() { return localValue("ShowWebPreview", true).toBool(); }
    inline void enableWebPreview(bool enabled) { setLocalValue("ShowWebPreview", enabled); }

    /**
     * Gets the format string for chat log timestamps
     *
     * @returns String representing timestamp format, e.g. "[hh:mm:ss]" or " hh:mm:ss"
     */
    inline QString timestampFormatString() { return localValue("TimestampFormat", " hh:mm:ss").toString(); }
    // Include a space in the default TimestampFormat to give the timestamp a small bit of padding
    // between the border of the chat buffer window and the numbers.  Helps with readability.
    /**
     * Sets the format string for chat log timestamps
     *
     * @param[in] format String representing timestamp format, e.g. "[hh:mm:ss]" or " hh:mm:ss"
     */
    inline void setTimestampFormatString(const QString &format) { setLocalValue("TimestampFormat", format); }

    /**
     * Gets if brackets are shown around sender names
     *
     * @returns True if sender brackets enabled, otherwise false
     */
    inline bool showSenderBrackets() { return localValue("ShowSenderBrackets", true).toBool(); }
    /**
     * Sets whether brackets are shown around around sender names.
     *
     * @param[in] enabled True if enabling sender brackets, otherwise false
     */
    inline void enableSenderBrackets(bool enabled) { setLocalValue("ShowSenderBrackets", enabled); }

    inline QString webSearchUrlFormatString() { return localValue("WebSearchUrlFormat", "https://www.google.com/search?q=%s").toString(); }
    inline void setWebSearchUrlFormatString(const QString &format) { setLocalValue("WebSearchUrlFormat", format); }
};


Q_DECLARE_METATYPE(ChatViewSettings::OperationMode)
#endif //CHATVIEWSETTINGS_H
