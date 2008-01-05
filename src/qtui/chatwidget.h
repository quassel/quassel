/***************************************************************************
 *   Copyright (C) 2005/06 by the Quassel Project                          *
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

#ifndef _CHATWIDGET_H_
#define _CHATWIDGET_H_

#include <QtGui>

class ChatLine;
class AbstractUiMsg;

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

    void prependMsg(AbstractUiMsg *);
    void appendMsg(AbstractUiMsg *);

    void prependChatLine(ChatLine *);
    void appendChatLine(ChatLine *);
    void prependChatLines(QList<ChatLine *>);
    void appendChatLines(QList<ChatLine *>);
    void setContents(QList<ChatLine *>);

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

#endif
