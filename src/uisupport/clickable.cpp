/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include <QDesktopServices>
#include <QModelIndex>
#include <QUrl>

#include "buffermodel.h"
#include "clickable.h"
#include "client.h"

void Clickable::activate(NetworkId networkId, const QString &text) const
{
    if (!isValid())
        return;

    QString str = text.mid(start(), length());

    switch (type()) {
    case Clickable::Url:
        if (!str.contains("://"))
            str = "http://" + str;
        QDesktopServices::openUrl(QUrl::fromEncoded(str.toUtf8(), QUrl::TolerantMode));
        break;
    case Clickable::Channel:
        Client::bufferModel()->switchToOrJoinBuffer(networkId, str);
        break;
    default:
        break;
    }
}


// NOTE: This method is not threadsafe and not reentrant!
//       (RegExps are not constant while matching, and they are static here for efficiency)
ClickableList ClickableList::fromString(const QString &str)
{
    // For matching URLs
    static QString scheme("(?:(?:mailto:|(?:[+.-]?\\w)+://)|www(?=\\.\\S+\\.))");
    static QString authority("(?:(?:[,.;@:]?[-\\w]+)+\\.?|\\[[0-9a-f:.]+\\])(?::\\d+)?");
    static QString urlChars("(?:[,.;:]*[\\w~@/?&=+$()!%#*{}\\[\\]\\|'^-])");
    static QString urlEnd("(?:>|[,.;:\"]*\\s|\\b|$)");

    static QRegExp regExp[] = {
        // URL
        // QRegExp(QString("((?:https?://|s?ftp://|irc://|mailto:|www\\.)%1+|%1+\\.[a-z]{2,4}(?:?=/%1+|\\b))%2").arg(urlChars, urlEnd)),
        QRegExp(QString("\\b(%1%2(?:/%3*)?)%4").arg(scheme, authority, urlChars, urlEnd), Qt::CaseInsensitive),

        // Channel name
        // We don't match for channel names starting with + or &, because that gives us a lot of false positives.
        QRegExp("((?:#|![A-Z0-9]{5})[^,:\\s]+(?::[^,:\\s]+)?)\\b", Qt::CaseInsensitive)

        // TODO: Nicks, we'll need a filtering for only matching known nicknames further down if we do this
    };

    static const int regExpCount = 2; // number of regexps in the array above

    qint16 matches[] = { 0, 0, 0 };
    qint16 matchEnd[] = { 0, 0, 0 };

    ClickableList result;
    //QString str = data(ChatLineModel::DisplayRole).toString();

    qint16 idx = 0;
    qint16 minidx;
    int type = -1;

    do {
        type = -1;
        minidx = str.length();
        for (int i = 0; i < regExpCount; i++) {
            if (matches[i] < 0 || matchEnd[i] > str.length()) continue;
            if (idx >= matchEnd[i]) {
                matches[i] = regExp[i].indexIn(str, qMax(matchEnd[i], idx));
                if (matches[i] >= 0) matchEnd[i] = matches[i] + regExp[i].cap(1).length();
            }
            if (matches[i] >= 0 && matches[i] < minidx) {
                minidx = matches[i];
                type = i;
            }
        }
        if (type >= 0) {
            idx = matchEnd[type];
            QString match = str.mid(matches[type], matchEnd[type] - matches[type]);
            if (type == Clickable::Url && str.at(idx-1) == ')') { // special case: closing paren only matches if we had an open one
                if (!match.contains('(')) {
                    matchEnd[type]--;
                    match.chop(1);
                }
            }
            if (type == Clickable::Channel) {
                // don't make clickable if it could be a #number
                if (QRegExp("^#\\d+$").exactMatch(match))
                    continue;
            }
            result.append(Clickable((Clickable::Type)type, matches[type], matchEnd[type] - matches[type]));
        }
    }
    while (type >= 0);
    return result;
}


Clickable ClickableList::atCursorPos(int idx)
{
    foreach(const Clickable &click, *this) {
        if (idx >= click.start() && idx < click.start() + click.length())
            return click;
    }
    return Clickable();
}
