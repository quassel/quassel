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

#include <QPainter>
#include <QTextDocument>
#include <QTextLayout>

#include "graphicalui.h"
#include "styledlabel.h"
#include "uistyle.h"

StyledLabel::StyledLabel(QWidget *parent)
: QFrame(parent),
  _wrapMode(QTextOption::NoWrap),
  _alignment(Qt::AlignVCenter|Qt::AlignLeft),
  _toolTipEnabled(true)
{
  QTextOption opt = _layout.textOption();
  opt.setWrapMode(_wrapMode);
  opt.setAlignment(_alignment);
  _layout.setTextOption(opt);
}

void StyledLabel::setWrapMode(QTextOption::WrapMode mode) {
  if(_wrapMode == mode)
    return;

  _wrapMode = mode;
  QTextOption opt = _layout.textOption();
  opt.setWrapMode(mode);
  _layout.setTextOption(opt);

  layout();
}

void StyledLabel::setAlignment(Qt::Alignment alignment) {
  if(_alignment == alignment)
    return;

  _alignment = alignment;
  QTextOption opt = _layout.textOption();
  opt.setAlignment(alignment);
  _layout.setTextOption(opt);

  layout();
}

void StyledLabel::resizeEvent(QResizeEvent *event) {
  QFrame::resizeEvent(event);

  layout();
}

QSize StyledLabel::sizeHint() const {
  return _sizeHint;
}

void StyledLabel::updateSizeHint() {
  QSize sh;
  int padding = frameWidth() * 2;
  sh = _layout.boundingRect().size().toSize() + QSize(padding, padding);

  if(_sizeHint != sh) {
    _sizeHint = sh;
    updateGeometry();
  }
}

void StyledLabel::setText(const QString &text) {
  UiStyle *style = GraphicalUi::uiStyle();

  UiStyle::StyledString sstr = style->styleString(style->mircToInternal(text), UiStyle::PlainMsg);
  QList<QTextLayout::FormatRange> layoutList = style->toTextLayoutList(sstr.formatList, sstr.plainText.length(), 0);

  _layout.setText(sstr.plainText);
  _layout.setAdditionalFormats(layoutList);

  layout();
}

void StyledLabel::updateToolTip() {
  if(frameRect().width() - 2*frameWidth() < _layout.minimumWidth())
    setToolTip(QString("<qt>%1</qt>").arg(Qt::escape(_layout.text()))); // only rich text gets wordwrapped!
  else
    setToolTip(QString());
}

void StyledLabel::layout() {
  qreal h = 0;
  qreal w = frameRect().width() - 2*frameWidth();

  _layout.beginLayout();
  forever {
    QTextLine line = _layout.createLine();
    if(!line.isValid())
      break;
    line.setLineWidth(w);
    line.setPosition(QPointF(0, h));
    h += line.height();
  }
  _layout.endLayout();

  updateSizeHint();
  updateToolTip();
  update();
}

void StyledLabel::paintEvent(QPaintEvent *) {
  QPainter painter(this);

  qreal y = (frameRect().height() - _layout.boundingRect().height()) / 2;
  _layout.draw(&painter, QPointF(0, y), QVector<QTextLayout::FormatRange>());
}
