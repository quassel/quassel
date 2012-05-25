/***************************************************************************
 *   The original file is part of the KDE libraries                        *
 *   Copyright (C) 2009 by Marco Martin <notmart@gmail.com>                *
 *   Quasselfied 2010 by Manuel Nickschas <sputnick@quassel-irc.org>       *
 *                                                                         *
 *   This file is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of the *
 *   License, or (at your option) any later version.                       *
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

#include "mainwin.h"
#include "qtui.h"
#include "statusnotifieritemdbus.h"
#include "statusnotifieritem.h"

#include <QDBusConnection>
#include <QPixmap>
#include <QImage>
#include <QApplication>
#include <QMenu>
#include <QMovie>

#ifdef HAVE_KDE
#  include <KWindowInfo>
#  include <KWindowSystem>
#endif

#include "statusnotifierwatcher.h"
#include "statusnotifieritemadaptor.h"

#ifdef Q_OS_WIN64
__inline int toInt(WId wid)
{
    return (int)((__int64)wid);
}


#else
__inline int toInt(WId wid)
{
    return (int)wid;
}


#endif

// Marshall the ImageStruct data into a D-BUS argument
const QDBusArgument &operator<<(QDBusArgument &argument, const DBusImageStruct &icon)
{
    argument.beginStructure();
    argument << icon.width;
    argument << icon.height;
    argument << icon.data;
    argument.endStructure();
    return argument;
}


// Retrieve the ImageStruct data from the D-BUS argument
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusImageStruct &icon)
{
    qint32 width;
    qint32 height;
    QByteArray data;

    argument.beginStructure();
    argument >> width;
    argument >> height;
    argument >> data;
    argument.endStructure();

    icon.width = width;
    icon.height = height;
    icon.data = data;

    return argument;
}


// Marshall the ImageVector data into a D-BUS argument
const QDBusArgument &operator<<(QDBusArgument &argument, const DBusImageVector &iconVector)
{
    argument.beginArray(qMetaTypeId<DBusImageStruct>());
    for (int i = 0; i < iconVector.size(); ++i) {
        argument << iconVector[i];
    }
    argument.endArray();
    return argument;
}


// Retrieve the ImageVector data from the D-BUS argument
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusImageVector &iconVector)
{
    argument.beginArray();
    iconVector.clear();

    while (!argument.atEnd()) {
        DBusImageStruct element;
        argument >> element;
        iconVector.append(element);
    }

    argument.endArray();

    return argument;
}


// Marshall the ToolTipStruct data into a D-BUS argument
const QDBusArgument &operator<<(QDBusArgument &argument, const DBusToolTipStruct &toolTip)
{
    argument.beginStructure();
    argument << toolTip.icon;
    argument << toolTip.image;
    argument << toolTip.title;
    argument << toolTip.subTitle;
    argument.endStructure();
    return argument;
}


// Retrieve the ToolTipStruct data from the D-BUS argument
const QDBusArgument &operator>>(const QDBusArgument &argument, DBusToolTipStruct &toolTip)
{
    QString icon;
    DBusImageVector image;
    QString title;
    QString subTitle;

    argument.beginStructure();
    argument >> icon;
    argument >> image;
    argument >> title;
    argument >> subTitle;
    argument.endStructure();

    toolTip.icon = icon;
    toolTip.image = image;
    toolTip.title = title;
    toolTip.subTitle = subTitle;

    return argument;
}


int StatusNotifierItemDBus::s_serviceCount = 0;

StatusNotifierItemDBus::StatusNotifierItemDBus(StatusNotifierItem *parent)
    : QObject(parent),
    m_statusNotifierItem(parent),
    m_service(QString("org.kde.StatusNotifierItem-%1-%2")
        .arg(QCoreApplication::applicationPid())
        .arg(++s_serviceCount)),
    m_dbus(QDBusConnection::connectToBus(QDBusConnection::SessionBus, m_service))
{
    new StatusNotifierItemAdaptor(this);
    //qDebug() << "service is" << m_service;
    registerService();
}


StatusNotifierItemDBus::~StatusNotifierItemDBus()
{
    unregisterService();
}


QDBusConnection StatusNotifierItemDBus::dbusConnection() const
{
    return m_dbus;
}


// FIXME: prevent double registrations, also test this on platforms != KDE
//
void StatusNotifierItemDBus::registerService()
{
    //qDebug() << "registering to" << m_service;
    m_dbus.registerService(m_service);
    m_dbus.registerObject("/StatusNotifierItem", this);
}


// FIXME: see above
void StatusNotifierItemDBus::unregisterService()
{
    //qDebug() << "unregistering from" << m_service;
    if (m_dbus.isConnected()) {
        m_dbus.unregisterObject("/StatusNotifierItem");
        m_dbus.unregisterService(m_service);
    }
}


QString StatusNotifierItemDBus::service() const
{
    return m_service;
}


//DBUS slots
//Values and calls have been adapted to Quassel

QString StatusNotifierItemDBus::Category() const
{
    return QString("Communications"); // no need to make this configurable for Quassel
}


QString StatusNotifierItemDBus::Title() const
{
    return m_statusNotifierItem->title();
}


QString StatusNotifierItemDBus::Id() const
{
    return QString("QuasselIRC");
}


QString StatusNotifierItemDBus::Status() const
{
    return m_statusNotifierItem->metaObject()->enumerator(m_statusNotifierItem->metaObject()->indexOfEnumerator("State")).valueToKey(m_statusNotifierItem->state());
}


int StatusNotifierItemDBus::WindowId() const
{
    return toInt(QtUi::mainWindow()->winId());
}


//Icon
//We don't need to support serialized icon data in Quassel

QString StatusNotifierItemDBus::IconName() const
{
    return m_statusNotifierItem->iconName();
}


DBusImageVector StatusNotifierItemDBus::IconPixmap() const
{
    return DBusImageVector();
}


QString StatusNotifierItemDBus::OverlayIconName() const
{
    return QString();
}


DBusImageVector StatusNotifierItemDBus::OverlayIconPixmap() const
{
    return DBusImageVector();
}


//Requesting attention icon and movie

QString StatusNotifierItemDBus::AttentionIconName() const
{
    return m_statusNotifierItem->attentionIconName();
}


DBusImageVector StatusNotifierItemDBus::AttentionIconPixmap() const
{
    return DBusImageVector();
}


QString StatusNotifierItemDBus::AttentionMovieName() const
{
    return QString();
}


//ToolTip

DBusToolTipStruct StatusNotifierItemDBus::ToolTip() const
{
    DBusToolTipStruct toolTip;
    toolTip.icon = m_statusNotifierItem->toolTipIconName();
    toolTip.image = DBusImageVector();
    toolTip.title = m_statusNotifierItem->toolTipTitle();
    toolTip.subTitle = m_statusNotifierItem->toolTipSubTitle();

    return toolTip;
}


QString StatusNotifierItemDBus::IconThemePath() const
{
    return m_statusNotifierItem->iconThemePath();
}


//Menu

QDBusObjectPath StatusNotifierItemDBus::Menu() const
{
    return QDBusObjectPath(m_statusNotifierItem->menuObjectPath());
}


//Interaction

void StatusNotifierItemDBus::ContextMenu(int x, int y)
{
    if (!m_statusNotifierItem->trayMenu()) {
        return;
    }

    //TODO: nicer placement, possible?
    if (!m_statusNotifierItem->trayMenu()->isVisible()) {
#ifdef HAVE_KDE
        m_statusNotifierItem->trayMenu()->setWindowFlags(Qt::Window|Qt::FramelessWindowHint);
#endif
        m_statusNotifierItem->trayMenu()->popup(QPoint(x, y));
#ifdef HAVE_KDE
        KWindowSystem::setState(m_statusNotifierItem->trayMenu()->winId(), NET::SkipTaskbar|NET::SkipPager|NET::KeepAbove);
        KWindowSystem::setType(m_statusNotifierItem->trayMenu()->winId(), NET::PopupMenu);
        KWindowSystem::forceActiveWindow(m_statusNotifierItem->trayMenu()->winId());
#endif
    }
    else {
        m_statusNotifierItem->trayMenu()->hide();
    }
}


void StatusNotifierItemDBus::Activate(int x, int y)
{
    m_statusNotifierItem->activated(QPoint(x, y));
}


void StatusNotifierItemDBus::SecondaryActivate(int x, int y)
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    // emit m_statusNotifierItem->secondaryActivateRequested(QPoint(x,y));
}


void StatusNotifierItemDBus::Scroll(int delta, const QString &orientation)
{
    Q_UNUSED(delta)
    Q_UNUSED(orientation)
    // Qt::Orientation dir = (orientation.toLower() == "horizontal" ? Qt::Horizontal : Qt::Vertical);
    // emit m_statusNotifierItem->scrollRequested(delta, dir);
}
