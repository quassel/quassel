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

#ifndef CHATVIEWSEARCHCONTROLLER_H
#define CHATVIEWSEARCHCONTROLLER_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QString>
#include <QTimeLine>

#include "chatscene.h"
#include "message.h"

class QGraphicsItem;
class ChatLine;
class SearchHighlightItem;

class ChatViewSearchController : public QObject
{
    Q_OBJECT

public:
    ChatViewSearchController(QObject *parent = 0);

    inline const QString &searchString() const { return _searchString; }

    void setScene(ChatScene *scene);

public slots:
    void setSearchString(const QString &searchString);
    void setCaseSensitive(bool caseSensitive);
    void setSearchSenders(bool searchSenders);
    void setSearchMsgs(bool searchMsgs);
    void setSearchOnlyRegularMsgs(bool searchOnlyRegularMsgs);

    void highlightNext();
    void highlightPrev();

private slots:
    void sceneDestroyed();
    void updateHighlights(bool reuse = false);

    void repositionHighlights();
    void repositionHighlights(ChatLine *line);

signals:
    void newCurrentHighlight(QGraphicsItem *highlightItem);

private:
    QString _searchString;
    ChatScene *_scene;
    QList<SearchHighlightItem *> _highlightItems;
    int _currentHighlight;

    bool _caseSensitive;
    bool _searchSenders;
    bool _searchMsgs;
    bool _searchOnlyRegularMsgs;

    inline Qt::CaseSensitivity caseSensitive() const { return _caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive; }

    inline bool checkType(Message::Type type) const { return type & (Message::Plain | Message::Notice | Message::Action); }

    void checkMessagesForHighlight(int start = 0, int end = -1);
    void highlightLine(ChatLine *line);
    void updateHighlights(ChatLine *line);
};


// Highlight Items
#include <QGraphicsItem>

class SearchHighlightItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
#if QT_VERSION >= 0x040600
    Q_INTERFACES(QGraphicsItem)
#endif

public :
        SearchHighlightItem(QRectF wordRect, QGraphicsItem *parent = 0);
    virtual inline QRectF boundingRect() const { return _boundingRect; }
    void updateGeometry(qreal width, qreal height);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    enum { Type = ChatScene::SearchHighlightType };
    virtual inline int type() const { return Type; }

    void setHighlighted(bool highlighted);

    static bool firstInLine(QGraphicsItem *item1, QGraphicsItem *item2);

private slots:
    void updateHighlight(qreal value);

private:
    QRectF _boundingRect;
    bool _highlighted;
    int _alpha;
    QTimeLine _timeLine;
};


#endif //CHATVIEWSEARCHCONTROLLER_H
