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

#ifndef QTUISTYLE_H_
#define QTUISTYLE_H_

#include "uistyle.h"

class QtUiStyle : public UiStyle {

public:
  QtUiStyle();
  virtual ~QtUiStyle();

  virtual inline qreal firstColumnSeparator() const { return 6; }
  virtual inline qreal secondColumnSeparator() const { return 6; }
  virtual inline QColor highlightColor() const { return _highlightColor; }
  virtual void setHighlightColor(const QColor &);

protected:
  void inline addSenderAutoColor( FormatType type, const QString name );

private:
  QColor _highlightColor;
};

#endif
