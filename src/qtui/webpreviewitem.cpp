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

#include "webpreviewitem.h"

#ifdef HAVE_WEBKIT

#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QWebView>
#include <QWebSettings>

WebPreviewItem::WebPreviewItem(const QUrl &url)
    : QGraphicsItem(0), // needs to be a top level item as we otherwise cannot guarantee that it's on top of other chatlines
    _boundingRect(0, 0, 400, 300)
{
    qreal frameWidth = 5;

    QWebView *webView = new QWebView;
    webView->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
    webView->load(url);
    webView->resize(1000, 750);
    QGraphicsProxyWidget *proxyItem = new QGraphicsProxyWidget(this);
    proxyItem->setWidget(webView);
    proxyItem->setAcceptHoverEvents(false);

    qreal xScale = (_boundingRect.width() - 2 * frameWidth) / webView->width();
    qreal yScale = (_boundingRect.height() - 2 * frameWidth) / webView->height();
    proxyItem->scale(xScale, yScale);
    proxyItem->setPos(frameWidth, frameWidth);

    setZValue(30);
}


void WebPreviewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option); Q_UNUSED(widget);
    painter->setClipRect(boundingRect());
    painter->setPen(QPen(Qt::black, 5));
    painter->setBrush(Qt::black);
    painter->setRenderHints(QPainter::Antialiasing);
    painter->drawRoundedRect(boundingRect(), 10, 10);
}


#endif //#ifdef HAVE_WEBKIT
