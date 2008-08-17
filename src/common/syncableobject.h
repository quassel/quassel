/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#ifndef _SYNCABLEOBJECT_H_
#define _SYNCABLEOBJECT_H_

#include <QDataStream>
#include <QMetaType>
#include <QObject>
#include <QVariantMap>

class SyncableObject : public QObject {
  Q_OBJECT

public:
  SyncableObject(QObject *parent = 0);
  SyncableObject(const SyncableObject &other, QObject *parent = 0);

  //! Stores the object's state into a QVariantMap.
  /** The default implementation takes dynamic properties as well as getters that have
   *  names starting with "init" and stores them in a QVariantMap. Override this method in
   *  derived classes in order to store the object state in a custom form.
   *  \note  This is used by SignalProxy to transmit the state of the object to clients
   *         that request the initial object state. Later updates use a different mechanism
   *         and assume that the state is completely covered by properties and init* getters.
   *         DO NOT OVERRIDE THIS unless you know exactly what you do!
   *
   *  \return The object's state in a QVariantMap
   */
  virtual QVariantMap toVariantMap();

  //! Initialize the object's state from a given QVariantMap.
  /** \see toVariantMap() for important information concerning this method.
   */
  virtual void fromVariantMap(const QVariantMap &properties);

  virtual bool isInitialized() const;

  virtual const QMetaObject *syncMetaObject() const { return metaObject(); };

  inline void setAllowClientUpdates(bool allow) { _allowClientUpdates = allow; }
  inline bool allowClientUpdates() const { return _allowClientUpdates; }

public slots:
  virtual void setInitialized();
  void requestUpdate(const QVariantMap &properties);
  void update(const QVariantMap &properties);

protected:
  void renameObject(const QString &newName);
  SyncableObject &operator=(const SyncableObject &other);

signals:
  void initDone();
  void updatedRemotely();
  void updated(const QVariantMap &properties);
  void updateRequested(const QVariantMap &properties);
  void objectRenamed(QString newName, QString oldName);

private:
  bool setInitValue(const QString &property, const QVariant &value);

  bool _initialized;
  bool _allowClientUpdates;

};

#endif
