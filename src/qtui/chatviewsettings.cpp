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

#include "chatviewsettings.h"

#include "chatscene.h"
#include "chatview.h"

ChatViewSettings::ChatViewSettings(const QString &id)
    : QtUiSettings(QString("ChatView/%1").arg(id))
{
}


ChatViewSettings::ChatViewSettings(ChatScene *scene)
    : QtUiSettings(QString("ChatView/%1").arg(scene->idString()))
{
}


ChatViewSettings::ChatViewSettings(ChatView *view)
    : QtUiSettings(QString("ChatView/%1").arg(view->scene()->idString()))
{
}


bool ChatViewSettings::showWebPreview() const
{
    return localValue("ShowWebPreview", false).toBool();
}


void ChatViewSettings::enableWebPreview(bool enabled)
{
    setLocalValue("ShowWebPreview", enabled);
}


bool ChatViewSettings::useCustomTimestampFormat() const
{
    return localValue("UseCustomTimestampFormat", false).toBool();
}


void ChatViewSettings::setUseCustomTimestampFormat(bool enabled)
{
    setLocalValue("UseCustomTimestampFormat", enabled);
}


QString ChatViewSettings::timestampFormatString() const
{
    // Include a space in the default TimestampFormat to give the timestamp a small bit of padding
    // between the border of the chat buffer window and the numbers.  Helps with readability.
    return localValue("TimestampFormat", " hh:mm:ss").toString();
}


void ChatViewSettings::setTimestampFormatString(const QString &format)
{
    setLocalValue("TimestampFormat", format);
}


UiStyle::SenderPrefixMode ChatViewSettings::senderPrefixDisplay() const
{
    return static_cast<UiStyle::SenderPrefixMode>(
                localValue("SenderPrefixMode",
                           QVariant::fromValue<UiStyle::SenderPrefixMode>(
                               UiStyle::SenderPrefixMode::HighestMode)).toInt());
    // Cast the QVariant to an integer, then cast that to the enum class.
    // .canConvert<UiStyle::SenderPrefixMode>() returned true, but
    // .value<UiStyle::SenderPrefixMode>(); always gave the default value 0.
    //
    // There's probably a cleaner way of doing this.  I couldn't find it within 4 hours, so...
}


bool ChatViewSettings::showSenderBrackets() const
{
    return localValue("ShowSenderBrackets", false).toBool();
}


void ChatViewSettings::enableSenderBrackets(bool enabled)
{
    setLocalValue("ShowSenderBrackets", enabled);
}


QString ChatViewSettings::webSearchUrlFormatString() const
{
    return localValue("WebSearchUrlFormat", "https://www.google.com/search?q=%s").toString();
}


void ChatViewSettings::setWebSearchUrlFormatString(const QString &format)
{
    setLocalValue("WebSearchUrlFormat", format);
}
