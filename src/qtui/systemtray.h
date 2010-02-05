/***************************************************************************
 *   Copyright (C) 2005-2010 by the Quassel Project                        *
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

#ifndef SYSTEMTRAY_H_
#define SYSTEMTRAY_H_

#include "icon.h"

class QMenu;

class SystemTray : public QObject {
  Q_OBJECT

public:
  enum State {
    Passive,
    Active,
    NeedsAttention
  };

  enum Mode {
    Invalid,
    Legacy,
    StatusNotifier
  };

  // same as in QSystemTrayIcon
  enum MessageIcon {
    NoIcon,
    Information,
    Warning,
    Critical
  };

  // same as in QSystemTrayIcon
  enum ActivationReason {
    Unknown,
    Context,
    DoubleClick,
    Trigger,
    MiddleClick
  };

  SystemTray(QObject *parent = 0);
  virtual ~SystemTray();
  virtual void init() {}

  inline State state() const;
  inline bool isAlerted() const;
  virtual inline bool isSystemTrayAvailable() const;

  void setAlert(bool alerted);
  inline void setInhibitActivation();
  inline bool isActivationInhibited() const;

public slots:
  virtual void setState(State);
  virtual void setVisible(bool visible = true);
  virtual void setToolTip(const QString &title, const QString &subtitle);
  virtual void showMessage(const QString &title, const QString &message, MessageIcon icon = Information, int millisecondsTimeoutHint = 10000);

signals:
  void activated(SystemTray::ActivationReason);
  void iconChanged(const Icon &);
  void toolTipChanged(const QString &title, const QString &subtitle);
  void messageClicked();

protected:
  virtual void setMode(Mode mode);
  inline Mode mode() const;

  virtual Icon stateIcon() const;
  Icon stateIcon(State state) const;
  inline QString toolTipTitle() const;
  inline QString toolTipSubTitle() const;
  inline QMenu *trayMenu() const;
  void setTrayMenu(QMenu *);

  virtual bool eventFilter(QObject *obj, QEvent *event);

private slots:

private:
  Mode _mode;
  State _state;

  bool _inhibitActivation;

  QString _toolTipTitle, _toolTipSubTitle;
  Icon _passiveIcon, _activeIcon, _needsAttentionIcon;

  QMenu *_trayMenu;
};

// inlines

bool SystemTray::isSystemTrayAvailable() const { return false; }
bool SystemTray::isAlerted() const { return state() == NeedsAttention; }
void SystemTray::setInhibitActivation() { _inhibitActivation = true; }
bool SystemTray::isActivationInhibited() const { return _inhibitActivation; }
SystemTray::Mode SystemTray::mode() const { return _mode; }
SystemTray::State SystemTray::state() const { return _state; }
QMenu *SystemTray::trayMenu() const { return _trayMenu; }
QString SystemTray::toolTipTitle() const { return _toolTipTitle; }
QString SystemTray::toolTipSubTitle() const { return _toolTipSubTitle; }


#endif
