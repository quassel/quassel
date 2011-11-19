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

#ifndef QMLCHATLINE_H_
#define QMLCHATLINE_H_

#include <QDeclarativeItem>

#include "uistyle.h"

class QAbstractItemModel;

#include <QAbstractItemModel>

class QmlChatLine : public QDeclarativeItem {
  Q_OBJECT

  Q_PROPERTY(QObject *model READ modelPointer WRITE setModelPointer)
  Q_PROPERTY(QmlChatLine::RenderData renderData READ renderData WRITE setRenderData)
  Q_PROPERTY(qreal timestampWidth READ timestampWidth WRITE setTimestampWidth NOTIFY timestampWidthChanged)
  Q_PROPERTY(qreal senderWidth READ senderWidth WRITE setSenderWidth NOTIFY senderWidthChanged)
  Q_PROPERTY(qreal contentsWidth READ contentsWidth WRITE setContentsWidth NOTIFY contentsWidthChanged)
  Q_PROPERTY(qreal columnSpacing READ columnSpacing WRITE setColumnSpacing NOTIFY columnSpacingChanged)
  Q_PROPERTY(QVariant test READ test WRITE setTest)

public:
  enum ColumnType {
    TimestampColumn,
    SenderColumn,
    ContentsColumn,
    NumColumns
  };

  //! Contains all model data needed to render a QmlChatLine
  struct RenderData {
    struct Column {
      QString text;
      UiStyle::FormatList formats;
      QBrush background;
      QBrush selectedBackground;
    };

    qint32 messageLabel;
    bool isValid;

    Column &operator[](ColumnType col) {
      return _data[col];
    }

    Column const &operator[](ColumnType col) const {
      return _data[col];
    }

    RenderData() { messageLabel = 0; isValid = false; }

  private:
    Column _data[NumColumns];
  };

  class ColumnLayout;
  class TimestampLayout;
  class SenderLayout;
  class ContentsLayout;
  class Layout;

  QmlChatLine(QDeclarativeItem *parent = 0);
  virtual ~QmlChatLine();

  inline QAbstractItemModel *model() const { return _model; }
  inline QObject *modelPointer() const { return _model; }
  void setModelPointer(QObject *model) { _model = qobject_cast<QAbstractItemModel *>(model); }

  inline RenderData renderData() const { return _data; }
  void setRenderData(const RenderData &data);

  Layout *layout();

  inline qreal timestampWidth() const { return _timestampWidth; }
  void setTimestampWidth(qreal w);
  inline qreal senderWidth() const { return _senderWidth; }
  void setSenderWidth(qreal w);
  inline qreal contentsWidth() const { return _contentsWidth; }
  void setContentsWidth(qreal w);
  inline qreal columnSpacing() const { return _columnSpacing; }
  void setColumnSpacing(qreal s);

  inline QString text() const { return renderData()[ContentsColumn].text; }

  void setTest(const QVariant &test) { _test = test; qDebug() << "set test" << test; }
  QVariant test() const { return _test; }

  QPointF columnPos(ColumnType colType) const;
  qreal columnWidth(ColumnType colType) const;
  QRectF columnBoundingRect(ColumnType colType) const;

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

  static void registerTypes();

signals:
  void timestampWidthChanged(qreal);
  void senderWidthChanged(qreal);
  void contentsWidthChanged(qreal);
  void columnWidthChanged(ColumnType column);
  void columnSpacingChanged(qreal);

public slots:
  void onClicked(qreal mouseX, qreal mouseY) { qDebug() << "clicked" << mouseX << mouseY; }
  void onPressed(qreal mouseX, qreal mouseY) { qDebug() << "pressed" << mouseX << mouseY; }
  void onMousePositionChanged(qreal mouseX, qreal mouseY) { qDebug() << "moved" << mouseX << mouseY; }

protected:

protected slots:
  void onColumnWidthChanged(ColumnType column);

private:
  QAbstractItemModel *_model;
  RenderData _data;

  qreal _timestampWidth, _senderWidth, _contentsWidth;
  qreal _columnSpacing;

  QVariant _test;

  mutable Layout *_layout;
};

QDataStream &operator<<(QDataStream &out, const QmlChatLine::RenderData &data);
QDataStream &operator>>(QDataStream &in, QmlChatLine::RenderData &data);

Q_DECLARE_METATYPE(QmlChatLine::RenderData)

/** Layout classes */

class QmlChatLine::Layout {
public:
  explicit Layout(QmlChatLine *parent);
  ~Layout();

  inline const QmlChatLine *chatLine() const { return _parent; }

  qreal height() const;
  void compute();
  void draw(QPainter *p);

private:
  QmlChatLine *_parent;
  QmlChatLine::ColumnLayout *_timestampLayout, *_senderLayout, *_contentsLayout;
};

class QmlChatLine::ColumnLayout {
public:
  explicit ColumnLayout(QmlChatLine::ColumnType col, QmlChatLine *chatLine);
  virtual ~ColumnLayout();

  inline QmlChatLine *chatLine() const { return _parent; }
  inline ColumnType columnType() const { return _type; }

  virtual qreal height() const;
  virtual void compute();
  virtual void draw(QPainter *p);

protected:
  inline QTextLayout *layout() const { return _layout; }
  void initLayout(QTextOption::WrapMode wrapMode, Qt::Alignment alignment);
  QVector<QTextLayout::FormatRange> selectionFormats() const;

private:
  QmlChatLine *_parent;
  ColumnType _type;
  QTextLayout *_layout;
};

class QmlChatLine::TimestampLayout : public QmlChatLine::ColumnLayout {
public:
  explicit TimestampLayout(QmlChatLine *chatLine);

};

class QmlChatLine::SenderLayout : public QmlChatLine::ColumnLayout {
public:
  explicit SenderLayout(QmlChatLine *chatLine);

};

class QmlChatLine::ContentsLayout : public QmlChatLine::ColumnLayout {
public:
  explicit ContentsLayout(QmlChatLine *chatLine);

  void compute();
};

#endif
