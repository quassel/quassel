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
#include "synchronizer.h"

#include <QString>
#include <QRegExp>
#include <QMetaProperty>
#include <QMetaMethod>

#include "util.h"
#include "signalproxy.h"

#include <QByteArray>
#include <QDebug>

// ====================
//  Public:
// ====================
Synchronizer::Synchronizer(QObject *parent, SignalProxy *signalproxy)
  : QObject(parent),
    _initialized(false),
    _signalproxy(signalproxy)
{
  attach();
  if(!getMethodByName("objectNameSet()").isEmpty())
    connect(parent,SIGNAL(objectNameSet()), this, SLOT(parentChangedName()));
}

bool Synchronizer::initialized() const {
  return _initialized;
}

SignalProxy *Synchronizer::proxy() const {
  return _signalproxy;
}

QVariantMap Synchronizer::initData() const {
  QVariantMap properties;
  
  // we collect data from properties
  foreach(QMetaProperty property, parentProperties()) {
    QString name = QString(property.name());
    QVariant value = property.read(parent());
    properties[name] = value;
    // qDebug() << ">>> SYNC:" << name << value;
  }

  // ...as well as methods, which have names starting with "init"
  foreach(QMetaMethod method, parentSlots()) {
    QString methodname = QString(method.signature()).section("(", 0, 0);
    if(methodname.startsWith("initSet") ||
       !methodname.startsWith("init"))
      continue;

    QVariant value = QVariant(QVariant::nameToType(method.typeName()));
    QGenericReturnArgument genericvalue = QGenericReturnArgument(method.typeName(), &value);
    QMetaObject::invokeMethod(parent(), methodname.toAscii(), genericvalue);

    properties[methodBaseName(method)] = value;
    // qDebug() << ">>> SYNC:" << methodBaseName(method) << value;
  }

  // properties["Payload"] = QByteArray(10000000, 'a');  // for testing purposes
  return properties;
}

void Synchronizer::setInitData(const QVariantMap &properties) {
  if(initialized())
    return;

  const QMetaObject *metaobject = parent()->metaObject();  
  QMapIterator<QString, QVariant> iterator(properties);
  while(iterator.hasNext()) {
    iterator.next();
    QString name = iterator.key();
    int propertyIndex = metaobject->indexOfProperty(name.toAscii());
    if(propertyIndex == -1) {
      setInitValue(name, iterator.value());
    } else {
      if((metaobject->property(propertyIndex)).isWritable())
	parent()->setProperty(name.toAscii(), iterator.value());
      else
	setInitValue(name, iterator.value());	
    }
    // qDebug() << "<<< SYNC:" << name << iterator.value();
  }

  _initialized = true;
  emit initDone();
}

// ====================
//  Public Slots
// ====================
void Synchronizer::synchronizeClients() const {
  emit sendInitData(initData());
}

void Synchronizer::recvInitData(QVariantMap properties) {
  proxy()->detachObject(this);
  setInitData(properties);
}

void Synchronizer::parentChangedName() {
  proxy()->detachObject(parent());
  proxy()->detachObject(this);
  attach();
}

// ====================
//  Private
// ====================
QString Synchronizer::signalPrefix() const {
  return QString(parent()->metaObject()->className()) + "_" + QString(parent()->objectName()) + "_";
}

QString Synchronizer::initSignal() const {
  return QString(metaObject()->className())
    + "_" + QString(parent()->metaObject()->className())
    + "_" + QString(parent()->objectName())
    + "_" + QString(SIGNAL(sendInitData(QVariantMap)));
}

QString Synchronizer::requestSyncSignal() const {
  return QString(metaObject()->className())
    + "_" + QString(parent()->metaObject()->className())
    + "_" + QString(parent()->objectName())
    + "_" + QString(SIGNAL(requestSync()));
}

QString Synchronizer::methodBaseName(const QMetaMethod &method) const {
  QString methodname = QString(method.signature()).section("(", 0, 0);

  // determine where we have to chop:
  if(method.methodType() == QMetaMethod::Slot) {
    // we take evertyhing from the first uppercase char if it's slot
    methodname = methodname.mid(methodname.indexOf(QRegExp("[A-Z]")));
  } else {
    // and if it's a signal we discard everything from the last uppercase char
    methodname = methodname.left(methodname.lastIndexOf(QRegExp("[A-Z]")));
  }

  methodname[0] = methodname[0].toUpper();

  return methodname;
}

bool Synchronizer::methodsMatch(const QMetaMethod &signal, const QMetaMethod &slot) const {
  // if we don't even have the same basename it's a sure NO
  if(methodBaseName(signal) != methodBaseName(slot))
    return false;

  const QMetaObject *metaobject = parent()->metaObject();

  // are the signatures compatible?
  if(! metaobject->checkConnectArgs(signal.signature(), slot.signature()))
    return false;

  // we take an educated guess if the signals and slots match
  QString signalsuffix = QString(signal.signature()).section("(", 0, 0);
  signalsuffix = signalsuffix.mid(signalsuffix.lastIndexOf(QRegExp("[A-Z]"))).toLower();
    
  QString slotprefix = QString(slot.signature()).section("(", 0, 0);
  slotprefix = slotprefix.left(slotprefix.indexOf(QRegExp("[A-Z]"))).toLower();

  uint sizediff;
  if(signalsuffix.size() < slotprefix.size())
    sizediff = slotprefix.size() - signalsuffix.size();
  else
    sizediff = signalsuffix.size() - slotprefix.size();

  int ratio = editingDistance(slotprefix, signalsuffix) - sizediff;

  return (ratio < 2);
}

QList<QMetaProperty> Synchronizer::parentProperties() const {
  QList<QMetaProperty> _properties;

  const QMetaObject *metaobject = parent()->metaObject();
  for(int i = metaobject->propertyOffset(); i < metaobject->propertyCount(); i++) {
    _properties << metaobject->property(i);
  }
  
  return _properties;
}

QList<QMetaMethod> Synchronizer::parentSlots() const {
  QList<QMetaMethod> _slots;

  const QMetaObject *metaobject = parent()->metaObject();
  for(int i = metaobject->methodOffset(); i < metaobject->methodCount(); i++) {
    QMetaMethod method = metaobject->method(i);
    if(method.methodType() == QMetaMethod::Slot)
      _slots << method;
  }

  return _slots;
}


QList<QMetaMethod> Synchronizer::parentSignals() const {
  QList<QMetaMethod> _signals;

  const QMetaObject *metaobject = parent()->metaObject();
  for(int i = metaobject->methodOffset(); i < metaobject->methodCount(); i++) {
    QMetaMethod method = metaobject->method(i);
    if(method.methodType() == QMetaMethod::Signal)
      _signals << method;
  }
  
  return _signals;
}

QList<QMetaMethod> Synchronizer::getMethodByName(const QString &methodname) {
  QList<QMetaMethod> _methods;
  
  const QMetaObject* metaobject = parent()->metaObject();
  for(int i = metaobject->methodOffset(); i < metaobject->methodCount(); i++) {
    if(QString(metaobject->method(i).signature()).startsWith(methodname))
      _methods << metaobject->method(i);
  }

  return _methods;
}

void Synchronizer::attach() {
  if(proxy()->proxyMode() == SignalProxy::Server)
    attachAsMaster();
  else
    attachAsSlave();
}
 
void Synchronizer::attachAsSlave() {
  QList<QMetaMethod> signals_ = parentSignals();
  
  foreach(QMetaMethod slot, parentSlots()) {
    if(signals_.empty())
      break;
    
    for(int i = 0; i < signals_.count(); i++) {
      QMetaMethod signal = signals_[i];
      if(!methodsMatch(signal, slot))
	continue;

      // we could simply put a "1" in front of the normalized signature
      // but to guarantee future compatibility we construct a dummy signal
      // and replace the known the fake signature by ours...
      QString dummySlot = QString(SIGNAL(dummy()));
      QString proxySignal = signalPrefix() + QString(signal.signature());
      QString slotsignature = dummySlot.replace("dummy()", QString(slot.signature()));

      // qDebug() << "attachSlot:" << proxySignal << slotsignature;
      proxy()->attachSlot(proxySignal.toAscii(), parent(), slotsignature.toAscii());
      signals_.removeAt(i);
      break;
    }
  }

  if(!getMethodByName("setInitialized()").isEmpty())
    connect(this, SIGNAL(initDone()), parent(), SLOT(setInitialized()));

  if(!initialized()) {
    // and then we connect ourself, so we can receive init data
    // qDebug() << "attachSlot:" << initSignal() << "recvInitData(QVariantMap)";
    // qDebug() << "attachSignal:" << "requestSync()" << requestSyncSignal();
    proxy()->attachSlot(initSignal().toAscii(), this, SLOT(recvInitData(QVariantMap)));
    proxy()->attachSignal(this, SIGNAL(requestSync()), requestSyncSignal().toAscii());

    emit requestSync();
  }
}

void Synchronizer::attachAsMaster() {
  QList<QMetaMethod> slots_ = parentSlots();
  
  foreach(QMetaMethod signal, parentSignals()) {
    if(slots_.isEmpty())
      break;

    // we don't attach all signals, just the ones that have a maching counterpart
    for(int i = 0; i < slots_.count(); i++) {
      QMetaMethod slot = slots_[i];
      if(!methodsMatch(signal, slot))
	continue;
      
      // we could simply put a "2" in front of the normalized signature
      // but to guarantee future compatibility we construct a dummy signal
      // and replace the known the fake signature by ours...
      QString dummySignal = QString(SIGNAL(dummy()));
      QString proxySignal = signalPrefix() + QString(signal.signature());
      QString signalsignature = dummySignal.replace("dummy()", QString(signal.signature()));
    
      // qDebug() << "attachSignal:" << signalsignature << proxySignal;
      proxy()->attachSignal(parent(), signalsignature.toAscii(), proxySignal.toAscii());
      slots_.removeAt(i);
      break;
    }
  }
  
  // and then we connect ourself, so we can initialize slaves
  // qDebug() << "attachSignal:" << "sendInitData(QVariantMap)" << initSignal();
  // qDebug() << "attachSlot:" << "synchronizeClients()" << requestSyncSignal();
  proxy()->attachSignal(this, SIGNAL(sendInitData(QVariantMap)), initSignal().toAscii());
  proxy()->attachSlot(requestSyncSignal().toAscii(), this, SLOT(synchronizeClients()));
}

bool Synchronizer::setInitValue(const QString &property, const QVariant &value) {
  QString handlername = QString("initSet") + property;
  handlername[7] = handlername[7].toUpper();

  //determine param type
  QByteArray paramtype;
  foreach(QMetaMethod method, getMethodByName(handlername)) {
    if(method.parameterTypes().size() == 1) {
      paramtype = method.parameterTypes()[0];
      break;
    }
  }

  if(paramtype.isNull())
    return false;

  QGenericArgument param = QGenericArgument(paramtype, &value);
  return QMetaObject::invokeMethod(parent(), handlername.toAscii(), param);
}


