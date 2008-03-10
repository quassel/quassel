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

#ifndef TOPICBUTTON_H
#define TOPICBUTTON_H

#include <QSize>
#include <QAbstractButton>

#include "uistyle.h"

class TopicButton : public QAbstractButton {
  Q_OBJECT

public:
  TopicButton(QWidget *parent = 0);

  void setAndStyleText(const QString &text);

protected:
  virtual void paintEvent(QPaintEvent *event);

private:
  UiStyle::StyledText styledText;
  QSize _sizeHint;
};

#endif
