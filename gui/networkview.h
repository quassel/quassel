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

#ifndef _NETWORKVIEW_H_
#define _NETWORKVIEW_H_

#include <QtGui>
#include "ui_networkview.h"
#include "buffer.h"

typedef QHash<QString, QHash<QString, Buffer*> > BufferHash;

class NetworkViewWidget : public QWidget {
  Q_OBJECT

  public:
    NetworkViewWidget(QWidget *parent = 0);
    QTreeWidget *tree() { return ui.tree; }

    virtual QSize sizeHint () const;

  public slots:


  signals:
    void bufferSelected(QString net, QString buf);

  private slots:


  private:
    Ui::NetworkViewWidget ui;

};

class NetworkView : public QDockWidget {
  Q_OBJECT

  public:
    NetworkView(QString network, QWidget *parent = 0);

  public slots:
    void buffersUpdated(BufferHash);
    void selectBuffer(QString net, QString buf);

  signals:
    void bufferSelected(QString net, QString buf);

  private slots:
    void itemClicked(QTreeWidgetItem *item);

  private:
    QString network;
    QString currentNetwork, currentBuffer;
    QHash<QString, QHash<QString, Buffer*> > buffers;
    QHash<QString, QHash<QString, QTreeWidgetItem *> > items;
    QTreeWidget *tree;

};


#endif
