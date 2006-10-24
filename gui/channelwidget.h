/***************************************************************************
 *   Copyright (C) 2005 by The Quassel Team                                *
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

#ifndef _CHANNELWIDGET_H_
#define _CHANNELWIDGET_H_

#include "ui_channelwidget.h"
#include "ui_ircwidget.h"

#include "message.h"

class ChannelWidget : public QWidget {
  Q_OBJECT

  public:
    ChannelWidget(QString netname, QString bufname, QWidget *parent = 0);

    QString bufferName() { return _bufferName; }
    QString networkName() { return _networkName; }
  signals:
    void sendMessage(QString, QString, QString);

  public slots:
    void recvMessage(Message);
    void recvStatusMsg(QString msg);
    void setTopic(QString);
    void setNicks(QStringList);

  private slots:
    void enterPressed();

  private:
    Ui::ChannelWidget ui;

    QColor stdCol, errorCol, noticeCol, joinCol, quitCol, partCol, serverCol;
    QString _networkName;
    QString _bufferName;
};

/** Temporary widget for displaying a set of ChannelWidgets. */
class IrcWidget : public QWidget {
  Q_OBJECT

  public:
    IrcWidget(QWidget *parent = 0);

  public slots:
    void recvMessage(QString network, QString buffer, Message message);
    void recvStatusMsg(QString network, QString message);
    void setTopic(QString, QString, QString);
    void setNicks(QString, QString, QStringList);

  signals:
    void sendMessage(QString network, QString buffer, QString message);

  private slots:
    void userInput(QString, QString, QString);

  private:
    Ui::IrcWidget ui;
    QHash<QString, ChannelWidget *> buffers;

    ChannelWidget * getBuffer(QString net, QString buf);
};

#endif
