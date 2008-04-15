/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef _CHATLINE_OLD_H_
#define _CHATLINE_OLD_H_

#include <QtGui>

#include "util.h"
#include "uistyle.h"
#include "quasselui.h"

//FIXME: chatline doku
//!\brief Containing the layout and providing the rendering of a single message.
/** A ChatLineOld takes a Message object,
 * formats it (by turning the various message types into a human-readable form and afterwards pumping it through
 * our Style engine), and stores it as a number of QTextLayouts representing the three fields of a chat line
 * (timestamp, sender and text). These layouts already include any rendering information such as font,
 * color, or selected characters. By calling layout(), they can be quickly layouted to fit a given set of field widths.
 * Afterwards, they can quickly be painted whenever necessary.
 *
 * By separating the complex and slow task of interpreting and formatting Message objects (which happens exactly once
 * per message) from the actual layouting and painting, we gain a lot of speed compared to the standard Qt rendering
 * functions.
 */
class ChatLineOld : public QObject, public AbstractUiMsg {
  Q_OBJECT

  public:
    ChatLineOld(Message message);
    virtual ~ChatLineOld();

    qreal layout(qreal tsWidth, qreal nickWidth, qreal textWidth);
    qreal height() const { return hght; }
    int posToCursor(QPointF pos);
    void draw(QPainter *p, const QPointF &pos);

    enum SelectionMode { None, Partial, Full };
    void setSelection(SelectionMode, int start = 0, int end = 0);

    QDateTime timestamp() const;
    QString sender() const;
    QString text() const;
    MsgId msgId() const;
    BufferInfo bufferInfo() const;

    bool isUrl(int pos) const;
    QUrl getUrl(int pos) const;

  public slots:

  private:
    qreal hght;
    Message msg;
    qreal tsWidth, senderWidth, textWidth;
    UiStyle::StyledText styledTimeStamp, styledSender, styledText;

    struct FormatRange {
      int start;
      int length;
      int height;
      QTextCharFormat format;
    };
    struct Word {
      int start;
      int length;
      int trailing;
      int height;
    };
    struct LineLayout {
      int y;
      int height;
      int start;
      int length;
    };
    QVector<int> charPos;
    QVector<int> charWidths;
    QVector<int> charHeights;
    QVector<int> charUrlIdx;
    QList<FormatRange> tsFormat, senderFormat, textFormat;
    QList<Word> words;
    QList<LineLayout> lineLayouts;
    int minHeight;

    bool isHighlight;
    SelectionMode selectionMode;
    int selectionStart, selectionEnd;
    void formatMsg(Message);
    void precomputeLine();
    QList<FormatRange> calcFormatRanges(const UiStyle::StyledText &);
    QList<FormatRange> calcFormatRanges(const UiStyle::StyledText &, const QTextLayout::FormatRange &additional);

    QColor _highlightColor;
};

#endif
