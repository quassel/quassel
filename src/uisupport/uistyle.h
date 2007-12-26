/***************************************************************************
 *   Copyright (C) 2005-07 by the Quassel IRC Team                         *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _UISTYLE_H_
#define _UISTYLE_H_

#include <QTextCharFormat>
#include <QTextLayout>
#include <QUrl>

#include "message.h"
#include "settings.h"

class UiStyle {

  public:
    UiStyle(const QString &settingsKey);
    virtual ~UiStyle();

    /** This enumerates the possible formats a text element may have. */
    enum FormatType {
      None, Bold, Italic, Underline, Reverse,                                        // Standard formats
      PlainMsg, NoticeMsg, ServerMsg, ErrorMsg, JoinMsg, PartMsg, QuitMsg, KickMsg,  // Internal message formats
      RenameMsg, ModeMsg, ActionMsg,                                                 // ...cnt'd
      Timestamp, Sender, Nick, Hostmask, ChannelName, ModeFlags, Url,                // individual elements
      FgCol00, FgCol01, FgCol02, FgCol03, FgCol04, FgCol05, FgCol06, FgCol07,        // Color codes
      FgCol08, FgCol09, FgCol10, FgCol11, FgCol12, FgCol13, FgCol14, FgCol15,
      BgCol00, BgCol01, BgCol02, BgCol03, BgCol04, BgCol05, BgCol06, BgCol07,
      BgCol08, BgCol09, BgCol10, BgCol11, BgCol12, BgCol13, BgCol14, BgCol15,
      NumFormatTypes, Invalid   // Do not add anything after this
    };

    struct UrlInfo {
      int start, end;
      QUrl url;
    };

    struct StyledText {
      QString text;
      QList<QTextLayout::FormatRange> formats;
      QList<UrlInfo> urls;
    };

    StyledText styleString(QString);

    void setFormat(FormatType, QTextCharFormat, Settings::Mode mode/* = Settings::Custom*/);
    QTextCharFormat format(FormatType, Settings::Mode mode = Settings::Custom) const;

    FormatType formatType(const QString &code) const;
    QString formatCode(FormatType) const;

  protected:


  private:
    QTextCharFormat mergedFormat(QList<FormatType>);

    QVector<QTextCharFormat> _defaultFormats;
    QVector<QTextCharFormat> _customFormats;
    QHash<QString, FormatType> _formatCodes;

    QString _settingsKey;

};

#endif
