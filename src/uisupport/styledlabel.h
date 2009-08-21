/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
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

#ifndef STYLEDLABEL_H
#define STYLEDLABEL_H

#include <QFrame>

#include "uistyle.h"

class StyledLabel : public QFrame {
  Q_OBJECT

public:
  StyledLabel(QWidget *parent = 0);

  void setText(const QString &text);

  virtual QSize sizeHint() const;
  //virtual QSize minimumSizeHint() const;

  inline QTextOption::WrapMode wrapMode() const { return _wrapMode; }
  void setWrapMode(QTextOption::WrapMode mode);

  inline Qt::Alignment alignment() const { return _alignment; }
  void setAlignment(Qt::Alignment alignment);

  inline bool toolTipEnabled() const { return _toolTipEnabled; }
  void setToolTipEnabled(bool);

protected:
  virtual void paintEvent(QPaintEvent *event);
  virtual void resizeEvent(QResizeEvent *event);

  //void mouseMoveEvent(QMouseEvent *event);
  //void mousePressEvent(QMouseEvent *event);
  //void mouseReleaseEvent(QMouseEvent *event);
  //void mouseDoubleClickEvent(QMouseEvent *event);

private:
  QSize _sizeHint;
  QTextOption::WrapMode _wrapMode;
  Qt::Alignment _alignment;
  QTextLayout _layout;
  bool _toolTipEnabled;

  QList<QTextLayout::FormatRange> _layoutList;

  void layout();
  void updateSizeHint();
  void updateToolTip();
};

#endif
