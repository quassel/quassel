/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#pragma once

#include <QDialog>

#include "ui_coreinfodlg.h"
#include "coreinfo.h"
#include "coresessionwidget.h"

class CoreInfoDlg : public QDialog {
Q_OBJECT

public:
    explicit CoreInfoDlg(QWidget *parent = nullptr);

public slots:
    void coreInfoChanged(const QVariantMap &);

protected:
    void timerEvent(QTimerEvent *) override { updateUptime(); }

private slots:
    /**
     * Requests resynchronization of CoreInfo object for legacy (pre-0.13) cores
     *
     * This provides compatibility with updating core information for legacy cores, and can be
     * removed after protocol break.
     */
    void refreshLegacyCoreInfo();

    /**
     * Handler for recreation of CoreInfo object, including first-time setup
     *
     * Applies existing CoreInfo information to the dialog, too.
     */
    void coreInfoResynchronized();

    void on_closeButton_clicked() { reject(); }
    void updateUptime();
    void disconnectClicked(int peerId);

    /**
      * Event handler for core unspported Details button
      */
    void on_coreUnsupportedDetails_clicked();

private:
    Ui::CoreInfoDlg ui;
    QMap<int, CoreSessionWidget *> _widgets;
};
