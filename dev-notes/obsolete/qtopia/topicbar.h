/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef TOPICBAR_H_
#define TOPICBAR_H_

#include <QPushButton>
#include <QTimeLine>

#include "buffermodel.h"

class QPixmap;
class QTimer;

class TopicBar : public QPushButton {
  Q_OBJECT

  public:
    TopicBar(QWidget *parent = 0);
    ~TopicBar();

  public slots:
    void setContents(QString text, bool oneshot = true);
    void startScrolling();
    void stopScrolling();

  protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent (QResizeEvent *event);
    

  private slots:
    void updateOffset();
    void dataChanged(const QModelIndex &, const QModelIndex &);
    void currentChanged(const QModelIndex &, const QModelIndex &);

  private:
    void calcTextMetrics();

    BufferModel *_model;
    QItemSelectionModel *_selectionModel;
    
    QTimer *timer;
    int offset;
    int fillTextStart, secondTextStart;
    QString text, displayText;
    QString fillText;
    QFont topicFont;
    int frameWidth;
    int textWidth;
    bool oneshot;
};


#endif
