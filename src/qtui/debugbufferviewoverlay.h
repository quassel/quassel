/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#ifndef DEBUGBUFFERVIEWOVERLAY_H
#define DEBUGBUFFERVIEWOVERLAY_H

#include "ui_debugbufferviewoverlay.h"

class QLabel;
class QLineEdit;
class QTextEdit;

class DebugBufferViewOverlay : public QWidget
{
    Q_OBJECT

public:
    DebugBufferViewOverlay(QWidget *parent = 0);

private slots:
    void update();

private:
    Ui::DebugBufferViewOverlay ui;
    QLineEdit *_bufferViews;
    QLabel *_allNetworks;
    QLineEdit *_networks;
    QTextEdit *_bufferIds;
    QTextEdit *_removedBufferIds;
    QTextEdit *_tempRemovedBufferIds;
    QLabel *_addBuffersAutomatically;
    QLabel *_hideInactiveBuffers;
    QLabel *_allowedBufferTypes;
    QLabel *_minimumActivity;
    QLabel *_isInitialized;
};


#endif //DEBUGBUFFERVIEWOVERLAY_H
