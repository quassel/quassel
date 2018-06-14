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
#include "qtuistyle.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

QtUiStyle::QtUiStyle(QObject *parent) : UiStyle(parent)
{
    ChatViewSettings s;
    s.notify("UseCustomTimestampFormat", this, SLOT(updateUseCustomTimestampFormat()));
    updateUseCustomTimestampFormat();
    s.notify("TimestampFormat", this, SLOT(updateTimestampFormatString()));
    updateTimestampFormatString();
    s.notify("SenderPrefixMode", this, SLOT(updateSenderPrefixDisplay()));
    updateSenderPrefixDisplay();
    s.notify("ShowSenderBrackets", this, SLOT(updateShowSenderBrackets()));
    updateShowSenderBrackets();

    // If no style sheet exists, generate it on first run.
    initializeSettingsQss();
}


QtUiStyle::~QtUiStyle() {}

void QtUiStyle::updateUseCustomTimestampFormat()
{
    ChatViewSettings s;
    setUseCustomTimestampFormat(s.useCustomTimestampFormat());
}

void QtUiStyle::updateTimestampFormatString()
{
    ChatViewSettings s;
    setTimestampFormatString(s.timestampFormatString());
}

void QtUiStyle::updateSenderPrefixDisplay()
{
    ChatViewSettings s;
    setSenderPrefixDisplay(s.SenderPrefixDisplay());
}

void QtUiStyle::updateShowSenderBrackets()
{
    ChatViewSettings s;
    enableSenderBrackets(s.showSenderBrackets());
}


void QtUiStyle::initializeSettingsQss()
{
    QFileInfo settingsQss(Quassel::configDirPath() + "settings.qss");
    // Only initialize if it doesn't already exist
    if (settingsQss.exists())
        return;

    // Generate and load the new stylesheet
    generateSettingsQss();
    reload();
}

void QtUiStyle::generateSettingsQss() const
{
    QFile settingsQss(Quassel::configDirPath() + "settings.qss");

    if (!settingsQss.open(QFile::WriteOnly|QFile::Truncate)) {
        qWarning() << "Could not open" << settingsQss.fileName() << "for writing!";
        return;
    }
    QTextStream out(&settingsQss);

    out << "// Style settings made in Quassel's configuration dialog\n"
        << "// This file is automatically generated, do not edit\n";

    // ChatView
    ///////////
    QtUiStyleSettings fs("Fonts");
    if (fs.value("UseCustomChatViewFont").toBool())
        out << "\n// ChatView Font\n"
            << "ChatLine { " << fontDescription(fs.value("ChatView").value<QFont>()) << "; }\n";

    QtUiStyleSettings s("Colors");
    if (s.value("UseChatViewColors").toBool()) {
        out << "\n// Custom ChatView Colors\n"

        // markerline is special in that it always used to use a gradient, so we keep this behavior even with the new implementation
        << "Palette { marker-line: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " << color("MarkerLine", s) << ", stop: 0.1 transparent); }\n"
        << "ChatView { background: " << color("ChatViewBackground", s) << "; }\n\n"
        << "ChatLine[label=\"highlight\"] {\n"
        << "  foreground: " << color("Highlight", s) << ";\n"
        << "  background: " << color("HighlightBackground", s) << ";\n"
        << "}\n\n"
        << "ChatLine::timestamp { foreground: " << color("Timestamp", s) << "; }\n\n"

        << msgTypeQss("plain", "ChannelMsg", s)
        << msgTypeQss("notice", "ServerMsg", s)
        << msgTypeQss("action", "ActionMsg", s)
        << msgTypeQss("nick", "CommandMsg", s)
        << msgTypeQss("mode", "CommandMsg", s)
        << msgTypeQss("join", "CommandMsg", s)
        << msgTypeQss("part", "CommandMsg", s)
        << msgTypeQss("quit", "CommandMsg", s)
        << msgTypeQss("kick", "CommandMsg", s)
        << msgTypeQss("kill", "CommandMsg", s)
        << msgTypeQss("server", "ServerMsg", s)
        << msgTypeQss("info", "ServerMsg", s)
        << msgTypeQss("error", "ErrorMsg", s)
        << msgTypeQss("daychange", "ServerMsg", s)
        << msgTypeQss("topic", "CommandMsg", s)
        << msgTypeQss("netsplit-join", "CommandMsg", s)
        << msgTypeQss("netsplit-quit", "CommandMsg", s)
        << msgTypeQss("invite", "CommandMsg", s)
        << "\n";
    }

    if (s.value("UseSenderColors", true).toBool()) {
        out << "\n// Sender Colors\n";
        // Generate a color palette for easy reuse elsewhere
        // NOTE: A color palette is not a complete replacement for specifying the colors below, as
        // specifying the colors one-by-one instead of with QtUi::style()->brush(...) makes it easy
        // to toggle the specific coloring of sender/nick at the cost of regenerating this file.
        // See UiStyle::ColorRole
        out << senderPaletteQss(s);

        out << "ChatLine::sender#plain[sender=\"self\"] { foreground: palette(sender-color-self); }\n\n";

        // Matches qssparser.cpp for UiStyle::PlainMsg
        for (int i = 0; i < defaultSenderColors.count(); i++)
            out << senderQss(i, "plain");

        // Only color the nicks in CTCP ACTIONs if sender colors are enabled
        if (s.value("UseSenderActionColors", true).toBool()) {
            // For action messages, color the 'sender' column -and- the nick itself
            out << "\n// Sender Nickname Colors for action messages\n"
                << "ChatLine::sender#action[sender=\"self\"] { foreground: palette(sender-color-self); }\n"
                << "ChatLine::nick#action[sender=\"self\"] { foreground: palette(sender-color-self); }\n\n";

            // Matches qssparser.cpp for UiStyle::ActionMsg
            for (int i = 0; i < defaultSenderColors.count(); i++)
                out << senderQss(i, "action", true);
        }

        // Only color the nicks in CTCP ACTIONs if sender colors are enabled
        if (s.value("UseNickGeneralColors", true).toBool()) {
            // For action messages, color the 'sender' column -and- the nick itself
            out << "\n// Nickname colors for all messages\n"
                << "ChatLine::nick[sender=\"self\"] { foreground: palette(sender-color-self); }\n\n";

            // Matches qssparser.cpp for any style of message (UiStyle::...)
            for (int i = 0; i < defaultSenderColors.count(); i++)
                out << nickQss(i);
        }

    }

    // ItemViews
    ////////////

    UiStyleSettings uiFonts("Fonts");
    if (uiFonts.value("UseCustomItemViewFont").toBool()) {
        QString fontDesc = fontDescription(uiFonts.value("ItemView").value<QFont>());
        out << "\n// ItemView Font\n"
            << "ChatListItem { " << fontDesc << "; }\n"
            << "NickListItem { " << fontDesc << "; }\n\n";
    }

    UiStyleSettings uiColors("Colors");
    if (uiColors.value("UseBufferViewColors").toBool()) {
        out << "\n// BufferView Colors\n"
            << "ChatListItem { foreground: " << color("DefaultBuffer", uiColors) << "; }\n"
            << chatListItemQss("inactive", "InactiveBuffer", uiColors)
            << chatListItemQss("channel-event", "ActiveBuffer", uiColors)
            << chatListItemQss("unread-message", "UnreadBuffer", uiColors)
            << chatListItemQss("highlighted", "HighlightedBuffer", uiColors);
    }

    if (uiColors.value("UseNickViewColors").toBool()) {
        out << "\n// NickView Colors\n"
            << "NickListItem[type=\"category\"] { foreground: " << color("DefaultBuffer", uiColors) << "; }\n"
            << "NickListItem[type=\"user\"] { foreground: " << color("OnlineNick", uiColors) << "; }\n"
            << "NickListItem[type=\"user\", state=\"away\"] { foreground: " << color("AwayNick", uiColors) << "; }\n";
    }

    settingsQss.close();
}


QString QtUiStyle::color(const QString &key, UiSettings &settings, const QColor &defaultColor) const
{
    return settings.value(key, defaultColor).value<QColor>().name();
}


QString QtUiStyle::fontDescription(const QFont &font) const
{
    QString desc = "font: ";
    if (font.italic())
        desc += "italic ";
    if (font.bold())
        desc += "bold ";
    if (!font.italic() && !font.bold())
        desc += "normal ";
    desc += QString("%1pt \"%2\"").arg(font.pointSize()).arg(font.family());
    return desc;
}


QString QtUiStyle::msgTypeQss(const QString &msgType, const QString &key, UiSettings &settings) const
{
    return QString("ChatLine#%1 { foreground: %2; }\n").arg(msgType, color(key, settings));
}


QString QtUiStyle::senderPaletteQss(UiSettings &settings) const
{
    QString result;
    result += "Palette {\n";

    // Generate entries for sender-color-self
    result += QString("    sender-color-self: %1;\n")
              .arg(color("SenderSelf", settings, defaultSenderColorSelf));

    // Generate entries for sender-color-HASH
    for (int i = 0; i < defaultSenderColors.count(); i++) {
        QString dez = QString::number(i);
        if (dez.length() == 1) dez.prepend('0');
        result += QString("    sender-color-0%1: %2;\n")
                .arg(QString::number(i, 16), color("Sender"+dez, settings, defaultSenderColors[i]));
    }
    result += "}\n\n";
    return result;
}


QString QtUiStyle::senderQss(int i, const QString &messageType, bool includeNick) const
{
    QString dez = QString::number(i);
    if (dez.length() == 1) dez.prepend('0');

    if (includeNick) {
        // Include the nickname in the color rules
        return QString("ChatLine::sender#%1[sender=\"0%2\"] { foreground: palette(sender-color-0%2); }\n"
                       "ChatLine::nick#%1[sender=\"0%2\"]   { foreground: palette(sender-color-0%2); }\n")
                .arg(messageType, QString::number(i, 16));
    } else {
        return QString("ChatLine::sender#%1[sender=\"0%2\"] { foreground: palette(sender-color-0%2); }\n")
                .arg(messageType, QString::number(i, 16));
    }
}


QString QtUiStyle::nickQss(int i) const
{
    QString dez = QString::number(i);
    if (dez.length() == 1) dez.prepend('0');

    return QString("ChatLine::nick[sender=\"0%1\"]   { foreground: palette(sender-color-0%1); }\n")
            .arg(QString::number(i, 16));
}


QString QtUiStyle::chatListItemQss(const QString &state, const QString &key, UiSettings &settings) const
{
    return QString("ChatListItem[state=\"%1\"] { foreground: %2; }\n").arg(state, color(key, settings));
}
