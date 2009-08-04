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

#ifndef FONTSELECTOR_H_
#define FONTSELECTOR_H_

#include <QLabel>
#include <QWidget>

class FontSelector : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QFont selectedFont READ selectedFont WRITE setSelectedFont)

public:
  FontSelector(QWidget *parent = 0);

  inline const QFont &selectedFont() const { return _font; }

public slots:
  void setSelectedFont(const QFont &font);

signals:
  void fontChanged(const QFont &);

protected:
  void changeEvent(QEvent *e);

protected slots:
  void chooseFont();

private:
  QFont _font;
  QLabel *_demo;
};

#endif // FONTSELECTOR_H_
