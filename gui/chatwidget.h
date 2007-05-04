/***************************************************************************
 *   Copyright (C) 2005/06 by The Quassel Team                             *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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

#ifndef _CHATWIDGET_H_
#define _CHATWIDGET_H_

#include "style.h"
#include "message.h"
#include "buffer.h"
#include <QtCore>
#include <QtGui>

class ChatLine;

//!\brief Scroll area showing part of the chat messages for a given buffer.
/** The contents of the scroll area, i.e. a widget of type ChatWidgetContents,
 * needs to be provided by calling init(). We don't create this widget ourselves, because
 * while a ChatWidget will be destroyed and recreated quite often (for example when switching
 * buffers), there ususally is no need to re-render its content every time (which can be time-consuming).
 * Before a ChatWidget is destroyed, it gives up its ownership of its contents, referring responsibility
 * back to where it came from.
 *
 * Because we use this as a custom widget in Qt Designer, we cannot use a constructor that takes custom
 * parameters. Instead, it is mandatory to call init() before using this widget.
 */
class ChatWidget : public QAbstractScrollArea {
  Q_OBJECT

  public:
    ChatWidget(QWidget *parent = 0);
    ~ChatWidget();
    void init(QString net, QString buf);

    virtual QSize sizeHint() const;

  public slots:
    void clear();
    void prependChatLines(QList<ChatLine *>);
    void appendMsg(Message);
    void appendMsgList(QList<Message> *);

  protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void paintEvent(QPaintEvent * event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);

  private slots:
    void layout();
    void scrollBarAction(int);
    void scrollBarValChanged(int);
    void ensureVisible(int line);
    void handleScrollTimer();

  private:
    QString networkName, bufferName;
    enum SelectionMode { NoSelection, TextSelected, LinesSelected };
    enum MouseMode { Normal, Pressed, DragTsSep, DragTextSep, MarkText, MarkLines };
    enum MousePos { None, OverTsSep, OverTextSep, OverUrl };
    MouseMode mouseMode;
    MousePos mousePos;
    QPoint dragStartPos;
    MouseMode dragStartMode;
    int dragStartLine;
    int dragStartCursor;
    int curCursor;
    int curLine;
    SelectionMode selectionMode;
    int selectionStart, selectionEnd, selectionLine;

    int bottomLine, bottomLineOffset;

    QList<ChatLine *> lines;
    QList<qreal> ycoords;
    QTimer *scrollTimer;
    QPoint pointerPosition;

    int senderX;
    int textX;
    int tsWidth;
    int senderWidth;
    int textWidth;
    int tsGrabPos;     ///< X-Position for changing the timestamp width
    int senderGrabPos;
    void computePositions();

    int width;
    qreal height;
    qreal y;

    void adjustScrollBar();

    int yToLineIdx(qreal y);
    void clearSelection();
    QString selectionToString();
    void handleMouseMoveEvent(const QPoint &pos);

};

//FIXME: chatline doku
//!\brief Containing the layout and providing the rendering of a single message.
/** A ChatLine takes a Message object,
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
class ChatLine : public QObject {
  Q_OBJECT

  public:
    ChatLine(Message message, QString networkName, QString bufferName);
    ~ChatLine();

    qreal layout(qreal tsWidth, qreal nickWidth, qreal textWidth);
    qreal height() { return hght; }
    int posToCursor(QPointF pos);
    void draw(QPainter *p, const QPointF &pos);

    enum SelectionMode { None, Partial, Full };
    void setSelection(SelectionMode, int start = 0, int end = 0);
    QDateTime getTimeStamp();
    QString getSender();
    QString getText();

    bool isUrl(int pos);
    QUrl getUrl(int pos);

  public slots:

  private:
    qreal hght;
    Message msg;
    QString networkName, bufferName;
    qreal tsWidth, senderWidth, textWidth;
    Style::FormattedString tsFormatted, senderFormatted, textFormatted;

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

    SelectionMode selectionMode;
    int selectionStart, selectionEnd;
    void formatMsg(Message);
    void precomputeLine();
    QList<FormatRange> calcFormatRanges(const Style::FormattedString &, QTextLayout::FormatRange additional = QTextLayout::FormatRange());
};

struct LayoutTask {
  QList<Message> messages;
  Buffer *buffer;
  QString net, buf;
  QList<ChatLine *> lines;
};

Q_DECLARE_METATYPE(LayoutTask);

class LayoutThread : public QThread {
  Q_OBJECT

  public:
    LayoutThread();
    virtual ~LayoutThread();
    virtual void run();

  public:
    void processTask(LayoutTask task);

  signals:
    void taskProcessed(LayoutTask task);

  private:
    QList<LayoutTask> queue;
    QMutex mutex;
    QWaitCondition condition;
    bool abort;

};

#endif
