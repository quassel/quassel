/***************************************************************************
 *   Copyright (C) 2005-07 by The Quassel Team                             *
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
#include "guiproxy.h"
#include "buffer.h"

typedef QHash<QString, QHash<QString, Buffer*> > BufferHash;

class NetworkViewWidget : public QWidget {
  Q_OBJECT

  public:
    NetworkViewWidget(QWidget *parent = 0);

    QTreeWidget *tree() { return ui.tree; }

    virtual QSize sizeHint () const;

  signals:
    void bufferSelected(QString net, QString buf);

  private slots:


  private:
    Ui::NetworkViewWidget ui;

};

class NetworkView : public QDockWidget {
  Q_OBJECT

  public:
    enum Mode {
      NoActive = 0x01, NoInactive = 0x02,
      SomeNets = 0x04, AllNets = 0x08,
      NoChannels = 0x10, NoQueries = 0x20, NoServers = 0x40
    };

    NetworkView(QString name, int mode, QStringList nets = QStringList(), QWidget *parent = 0);
    void setMode(int mode, QStringList nets = QStringList());
    void setName(QString name);


  public slots:
    void bufferUpdated(Buffer *);
    void bufferDestroyed(Buffer *);
    void setBuffers(QList<Buffer *>);
    void selectBuffer(Buffer *);

  signals:
    void bufferSelected(Buffer *);
    void fakeUserInput(BufferId, QString);
    
  private slots:
    void itemClicked(QTreeWidgetItem *item);
    void itemDoubleClicked(QTreeWidgetItem *item);
    
  private:
    int mode;
    QString name;
    QStringList networks;
    Buffer *currentBuffer;
    //QHash<QString, QHash<QString, Buffer*> > buffers;
    QHash<Buffer *, QTreeWidgetItem *> bufitems;
    QHash<QString, QTreeWidgetItem *> netitems;
    //QHash<QString, QHash<QString, QTreeWidgetItem *> > items;
    QTreeWidget *tree;

    bool shouldShow(Buffer *);
};


#endif
