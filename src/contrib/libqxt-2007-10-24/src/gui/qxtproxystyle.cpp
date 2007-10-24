/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtGui module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/
#include <QStyleFactory>
#include "qxtproxystyle.h"

/*!
    \class QxtProxyStyle QxtProxyStyle
    \ingroup QxtGui
    \brief A proxy style.

    A technique called "proxy style" is a common way for creating
    cross-platform custom styles. Developers often want to do slight
    adjustments in some specific values returned by QStyle. A proxy
    style is the solution to avoid subclassing any platform specific
    style (eg. QPlastiqueStyle, QWindowsXPStyle, or QMacStyle) and
    to retain the native look on all supported platforms.

    The subject has been discussed in Qt Quarterly 9:
    http://doc.trolltech.com/qq/qq09-q-and-a.html#style (just notice
    that there are a few noteworthy spelling mistakes in the article).

    Proxy styles are becoming obsolete thanks to style sheets
    introduced in Qt 4.2. However, style sheets still is a new
    concept and only a portion of features are supported yet. Both -
    style sheets and proxy styles - have their pros and cons.

    \section usage Usage

    Implement the custom behaviour in a subclass of QxtProxyStyle:
    \code
    class MyCustomStyle : public QxtProxyStyle
    {
       public:
          MyCustomStyle(const QString& baseStyle) : QxtProxyStyle(baseStyle)
          {
          }

          int pixelMetric(PixelMetric metric, const QStyleOption* option = 0, const QWidget* widget = 0) const
          {
             if (metric == QStyle::PM_ButtonMargin)
                return 6;
             return QxtProxyStyle::pixelMetric(metric, option, widget);
          }
    };
    \endcode

    Using the custom style for the whole application:
    \code
    QString defaultStyle = QApplication::style()->objectName();
    QApplication::setStyle(new MyCustomStyle(defaultStyle));
    \endcode

    Using the custom style for a single widget:
    \code
    QString defaultStyle = widget->style()->objectName();
    widget->setStyle(new MyCustomStyle(defaultStyle));
    \endcode
 */

/*!
    Constructs a new QxtProxyStyle for \a style.
    See QStyleFactory::keys() for supported styles.

    \sa QStyleFactory::keys()
 */
QxtProxyStyle::QxtProxyStyle(const QString& baseStyle)
        : QStyle(), style(QStyleFactory::create(baseStyle))
{
    setObjectName(QLatin1String("proxy"));
}

/*!
    Destructs the proxy style.
 */
QxtProxyStyle::~QxtProxyStyle()
{
    delete style;
}

void QxtProxyStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
    style->drawComplexControl(control, option, painter, widget);
}

void QxtProxyStyle::drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget)  const
{
    style->drawControl(element, option, painter, widget);
}

void QxtProxyStyle::drawItemPixmap(QPainter* painter, const QRect& rect, int alignment, const QPixmap& pixmap) const
{
    style->drawItemPixmap(painter, rect, alignment, pixmap);
}

void QxtProxyStyle::drawItemText(QPainter* painter, const QRect& rect, int alignment, const QPalette& pal, bool enabled, const QString& text, QPalette::ColorRole textRole) const
{
    style->drawItemText(painter, rect, alignment, pal, enabled, text, textRole);
}

void QxtProxyStyle::drawPrimitive(PrimitiveElement elem, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    style->drawPrimitive(elem, option, painter, widget);
}

QPixmap QxtProxyStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* option) const
{
    return style->generatedIconPixmap(iconMode, pixmap, option);
}

QStyle::SubControl QxtProxyStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex* option, const QPoint& pos, const QWidget* widget) const
{
    return style->hitTestComplexControl(control, option, pos, widget);
}

QRect QxtProxyStyle::itemPixmapRect(const QRect& rect, int alignment, const QPixmap& pixmap) const
{
    return style->itemPixmapRect(rect, alignment, pixmap);
}

QRect QxtProxyStyle::itemTextRect(const QFontMetrics& metrics, const QRect& rect, int alignment, bool enabled, const QString& text) const
{
    return style->itemTextRect(metrics, rect, alignment, enabled, text);
}

int QxtProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    return style->pixelMetric(metric, option, widget);
}

void QxtProxyStyle::polish(QWidget* widget)
{
    style->polish(widget);
}

void QxtProxyStyle::polish(QApplication* app)
{
    style->polish(app);
}

void QxtProxyStyle::polish(QPalette& pal)
{
    style->polish(pal);
}

QSize QxtProxyStyle::sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget) const
{
    return style->sizeFromContents(type, option, contentsSize, widget);
}

QIcon QxtProxyStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget) const
{
    return style->standardIcon(standardIcon, option, widget);
}

QPalette QxtProxyStyle::standardPalette() const
{
    return style->standardPalette();
}

QPixmap QxtProxyStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption* option, const QWidget* widget) const
{
    return style->standardPixmap(standardPixmap, option, widget);
}

int QxtProxyStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
{
    return style->styleHint(hint, option, widget, returnData);
}

QRect QxtProxyStyle::subControlRect(ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget) const
{
    return style->subControlRect(control, option, subControl, widget);
}

QRect QxtProxyStyle::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
{
    return style->subElementRect(element, option, widget);
}

void QxtProxyStyle::unpolish(QWidget* widget)
{
    style->unpolish(widget);
}

void QxtProxyStyle::unpolish(QApplication* app)
{
    style->unpolish(app);
}
