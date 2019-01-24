/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#pragma once

#include "qtuisettings.h"
#include "uistyle.h"

class ChatScene;
class ChatView;

class ChatViewSettings : public QtUiSettings
{
public:
    Q_ENUMS(OperationMode)
public:
    enum OperationMode
    {
        InvalidMode = 0,
        OptIn = 1,
        OptOut = 2
    };
    Q_DECLARE_FLAGS(OperationModes, OperationMode)

    ChatViewSettings(const QString& id = "__default__");
    ChatViewSettings(ChatScene* scene);
    ChatViewSettings(ChatView* view);

    bool showWebPreview() const;
    void enableWebPreview(bool enabled);

    /**
     * Gets if a custom timestamp format is used.
     *
     * @returns True if custom timestamp format used, otherwise false
     */
    bool useCustomTimestampFormat() const;
    /**
     * Sets whether a custom timestamp format is used.
     *
     * @param[in] enabled True if custom timestamp format used, otherwise false
     */
    void setUseCustomTimestampFormat(bool enabled);

    /**
     * Gets the format string for chat log timestamps.
     *
     * @returns String representing timestamp format, e.g. "[hh:mm:ss]" or " hh:mm:ss"
     */
    QString timestampFormatString() const;
    // Include a space in the default TimestampFormat to give the timestamp a small bit of padding
    // between the border of the chat buffer window and the numbers.  Helps with readability.
    /**
     * Sets the format string for chat log timestamps
     *
     * @param[in] format String representing timestamp format, e.g. "[hh:mm:ss]" or " hh:mm:ss"
     */
    void setTimestampFormatString(const QString& format);

    /**
     * Gets how prefix modes are shown before sender names
     *
     * @returns SenderPrefixMode of what format to use for showing sender prefix modes
     */
    UiStyle::SenderPrefixMode senderPrefixDisplay() const;

    /**
     * Gets if brackets are shown around sender names
     *
     * @returns True if sender brackets enabled, otherwise false
     */
    bool showSenderBrackets() const;
    /**
     * Sets whether brackets are shown around around sender names.
     *
     * @param[in] enabled True if enabling sender brackets, otherwise false
     */
    void enableSenderBrackets(bool enabled);

    QString webSearchUrlFormatString() const;
    void setWebSearchUrlFormatString(const QString& format);
};

Q_DECLARE_METATYPE(ChatViewSettings::OperationMode)
