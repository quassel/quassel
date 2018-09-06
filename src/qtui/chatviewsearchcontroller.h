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

#ifndef CHATVIEWSEARCHCONTROLLER_H
#define CHATVIEWSEARCHCONTROLLER_H

#include <QGraphicsItem>
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
    ChatViewSearchController(QObject *parent = nullptr);

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
    ChatScene *_scene{nullptr};
    QList<SearchHighlightItem *> _highlightItems;
    int _currentHighlight{0};

    bool _caseSensitive{false};
    bool _searchSenders{false};
    bool _searchMsgs{true};
    bool _searchOnlyRegularMsgs{true};

    inline Qt::CaseSensitivity caseSensitive() const { return _caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive; }

    inline bool checkType(Message::Type type) const { return type & (Message::Plain | Message::Notice | Message::Action); }

    void checkMessagesForHighlight(int start = 0, int end = -1);
    void highlightLine(ChatLine *line);
    void updateHighlights(ChatLine *line);
};


// Highlight Items
class SearchHighlightItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    SearchHighlightItem(QRectF wordRect, QGraphicsItem *parent = nullptr);
    inline QRectF boundingRect() const override { return _boundingRect; }
    void updateGeometry(qreal width, qreal height);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    enum { Type = ChatScene::SearchHighlightType };
    inline int type() const override { return Type; }

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
