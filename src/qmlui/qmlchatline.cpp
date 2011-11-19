/***************************************************************************
 *   Copyright (C) 2005-2011 by the Quassel Project                        *
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
#include <QApplication>
#include <QPainter>

#include "graphicalui.h"
#include "qmlchatline.h"

void QmlChatLine::registerTypes() {
  qRegisterMetaType<RenderData>("QmlChatLine::RenderData");
  qRegisterMetaTypeStreamOperators<RenderData>("QmlChatLine::RenderData");
  qmlRegisterType<QmlChatLine>("eu.quassel.qml", 1, 0, "ChatLine");
}

QDataStream &operator<<(QDataStream &out, const QmlChatLine::RenderData &data) {
  out << data.isValid << data.messageLabel;
  for(int i = 0; i < (int)QmlChatLine::NumColumns; ++i) {
    const QmlChatLine::RenderData::Column &col = data[static_cast<QmlChatLine::ColumnType>(i)];
    out << col.text << col.formats << col.background << col.selectedBackground;
  }
  return out;
}

QDataStream &operator>>(QDataStream &in, QmlChatLine::RenderData &data) {
  in >> data.isValid >> data.messageLabel;
  for(int i = 0; i < (int)QmlChatLine::NumColumns; ++i) {
    QmlChatLine::RenderData::Column &col = data[static_cast<QmlChatLine::ColumnType>(i)];
    in >> col.text >> col.formats >> col.background >> col.selectedBackground;
  }
  return in;
}

QmlChatLine::QmlChatLine(QDeclarativeItem *parent)
  : QDeclarativeItem(parent),
    _timestampWidth(0),
    _senderWidth(0),
    _contentsWidth(0),
    _layout(0)
{
  setFlag(ItemHasNoContents, false);
  setImplicitHeight(QApplication::fontMetrics().height());
  setImplicitWidth(1000);
  connect(this, SIGNAL(columnWidthChanged(ColumnType)), SLOT(onColumnWidthChanged(ColumnType)));
}

QmlChatLine::~QmlChatLine() {

}

void QmlChatLine::setTimestampWidth(qreal w) {
  if(w != _timestampWidth) {
    _timestampWidth = w;
    emit timestampWidthChanged(w);
    emit columnWidthChanged(TimestampColumn);
  }
}

void QmlChatLine::setSenderWidth(qreal w) {
  if(w != _senderWidth) {
    _senderWidth = w;
    emit senderWidthChanged(w);
    emit columnWidthChanged(SenderColumn);
  }
}

void QmlChatLine::setContentsWidth(qreal w) {
  if(w != _contentsWidth) {
    _contentsWidth = w;

    if(renderData().isValid)
      layout()->compute();

    emit contentsWidthChanged(w);
    emit columnWidthChanged(ContentsColumn);
  }
}

void QmlChatLine::setColumnSpacing(qreal s) {
  if(s != _columnSpacing) {
    _columnSpacing = s;
    emit columnSpacingChanged(s);
  }
}

QPointF QmlChatLine::columnPos(ColumnType colType) const {
  switch(colType) {
  case TimestampColumn:
    return QPointF(0, 0);
  case SenderColumn:
    return QPointF(timestampWidth(), 0);
  case ContentsColumn:
    return QPointF(timestampWidth() + senderWidth(), 0);
  default:
    return QPointF();
  }
}

qreal QmlChatLine::columnWidth(ColumnType colType) const {
  switch(colType) {
  case TimestampColumn:
    return timestampWidth();
  case SenderColumn:
    return senderWidth();
  case ContentsColumn:
    return contentsWidth();
  default:
    return 0;
  }
}

QRectF QmlChatLine::columnBoundingRect(ColumnType colType) const {
  QRectF rect;
  switch(colType) {
  case TimestampColumn:
    return QRectF(columnPos(TimestampColumn), QSizeF(timestampWidth() - columnSpacing(), implicitHeight()));
  case SenderColumn:
    return QRectF(columnPos(SenderColumn), QSizeF(senderWidth() - columnSpacing(), implicitHeight()));
  case ContentsColumn:
    return QRectF(columnPos(ContentsColumn), QSizeF(contentsWidth(), implicitHeight()));
  default:
    return QRectF();
  }
}

void QmlChatLine::setRenderData(const RenderData &data) {
  _data = data;

  if(_layout) {
    delete _layout;
    _layout = 0;
  }

  //update();
}

QmlChatLine::Layout *QmlChatLine::layout() {
  if(!_layout) {
    _layout = new Layout(this);
  }
  return _layout;
}

void QmlChatLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option)
  Q_UNUSED(widget)
  if(renderData().isValid)
    layout()->draw(painter);
}

void QmlChatLine::onColumnWidthChanged(ColumnType colType) {
  Q_UNUSED(colType)
  setImplicitWidth(_timestampWidth + _senderWidth + _contentsWidth);
  update();
}

/**************************************************************************************/

QmlChatLine::Layout::Layout(QmlChatLine *parent)
  : _parent(parent)
{
  _timestampLayout = new TimestampLayout(parent);
  _senderLayout = new SenderLayout(parent);
  _contentsLayout = new ContentsLayout(parent);
}

QmlChatLine::Layout::~Layout() {
  delete _timestampLayout;
  delete _senderLayout;
  delete _contentsLayout;
}

qreal QmlChatLine::Layout::height() const {
  return _contentsLayout->height();
}

void QmlChatLine::Layout::compute() {
  _timestampLayout->compute();
  _senderLayout->compute();
  _contentsLayout->compute();
}

void QmlChatLine::Layout::draw(QPainter *p) {
  _timestampLayout->draw(p);
  _senderLayout->draw(p);
  _contentsLayout->draw(p);
}

/*************/

QmlChatLine::ColumnLayout::ColumnLayout(QmlChatLine::ColumnType type, QmlChatLine *parent)
  : _parent(parent),
    _type(type),
    _layout(0)
{

}

QmlChatLine::ColumnLayout::~ColumnLayout() {
  delete _layout;
}

void QmlChatLine::ColumnLayout::initLayout(QTextOption::WrapMode wrapMode, Qt::Alignment alignment) {
  if(_layout)
    delete _layout;

  const RenderData::Column &data = chatLine()->renderData()[columnType()];
  _layout = new QTextLayout(data.text);

  QTextOption option;
  option.setWrapMode(wrapMode);
  option.setAlignment(alignment);
  layout()->setTextOption(option);

  QList<QTextLayout::FormatRange> formats = GraphicalUi::uiStyle()->toTextLayoutList(data.formats, layout()->text().length(), chatLine()->renderData().messageLabel);
  layout()->setAdditionalFormats(formats);

  compute();
}

qreal QmlChatLine::ColumnLayout::height() const {
  return layout()->boundingRect().height();
}

void QmlChatLine::ColumnLayout::compute() {
  qreal width = chatLine()->columnBoundingRect(columnType()).width();
  qreal h = 0;
  layout()->beginLayout();
  forever {
    QTextLine line = layout()->createLine();
    if(!line.isValid())
      break;

    line.setLineWidth(width);
    line.setPosition(QPointF(0, h));
    h += line.height();
  }
  layout()->endLayout();
}

void QmlChatLine::ColumnLayout::draw(QPainter *p) {
  p->save();

  QRectF rect = chatLine()->columnBoundingRect(columnType());

  qreal layoutWidth = layout()->minimumWidth();
  qreal offset = 0;

  if(layout()->textOption().alignment() == Qt::AlignRight) {
    /*
      if(chatScene()->senderCutoffMode() == ChatScene::CutoffLeft)
        offset = qMin(width() - layoutWidth, (qreal)0);
      else
        offset = qMax(layoutWidth - width(), (qreal)0);
    */
      offset = qMax(layoutWidth - rect.width(), (qreal)0);
  }

  if(layoutWidth > rect.width()) {
    // Draw a nice gradient for longer items

    QLinearGradient gradient;
    if(offset < 0) {
      gradient.setStart(0, 0);
      gradient.setFinalStop(12, 0);
      gradient.setColorAt(0, Qt::transparent);
      gradient.setColorAt(1, Qt::white);
    } else {
      gradient.setStart(rect.width()-12, 0);
      gradient.setFinalStop(rect.width(), 0);
      gradient.setColorAt(0, Qt::white);
      gradient.setColorAt(1, Qt::transparent);
    }

    QImage img(layout()->boundingRect().toRect().size(), QImage::Format_ARGB32_Premultiplied);
    //img.fill(Qt::transparent);
    QPainter imgPainter(&img);
    imgPainter.fillRect(img.rect(), gradient);
    imgPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    layout()->draw(&imgPainter, QPointF(qMax(offset, (qreal)0), 0), selectionFormats());
    imgPainter.end();
    p->drawImage(rect.topLeft(), img);
  } else {
    layout()->draw(p, rect.topLeft(), selectionFormats(), rect);
  }

  p->restore();
}

QVector<QTextLayout::FormatRange> QmlChatLine::ColumnLayout::selectionFormats() const {
  return QVector<QTextLayout::FormatRange>();
}

/**************/

QmlChatLine::TimestampLayout::TimestampLayout(QmlChatLine *chatLine)
  : ColumnLayout(TimestampColumn, chatLine)
{
  initLayout(QTextOption::NoWrap, Qt::AlignLeft);
}


/**************/

QmlChatLine::SenderLayout::SenderLayout(QmlChatLine *chatLine)
  : ColumnLayout(SenderColumn, chatLine)
{
  initLayout(QTextOption::NoWrap, Qt::AlignRight);
}


/**************/

QmlChatLine::ContentsLayout::ContentsLayout(QmlChatLine *chatLine)
  : ColumnLayout(ContentsColumn, chatLine)
{
  initLayout(QTextOption::WrapAtWordBoundaryOrAnywhere, Qt::AlignLeft);
}

void QmlChatLine::ContentsLayout::compute() {
  ColumnLayout::compute();
  chatLine()->setImplicitHeight(layout()->boundingRect().height());
}
