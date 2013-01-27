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

#ifndef ABSTRACTBUFFERCONTAINER_H_
#define ABSTRACTBUFFERCONTAINER_H_

#include "abstractitemview.h"
#include "buffermodel.h"

class AbstractChatView;
class AbstractUiMsg;
class Buffer;

class AbstractBufferContainer : public AbstractItemView
{
    Q_OBJECT

public:
    AbstractBufferContainer(QWidget *parent);
    virtual ~AbstractBufferContainer();

    inline BufferId currentBuffer() const { return _currentBuffer; }

signals:
    void currentChanged(BufferId);
    void currentChanged(const QModelIndex &);

protected:
    //! Create an AbstractChatView for the given BufferId and add it to the UI if necessary
    virtual AbstractChatView *createChatView(BufferId) = 0;

    //! Remove a chat view from the UI and delete it
    /** This method shall remove the view from the UI (for example, from a QStackedWidget) if appropriate.
     *  It also shall delete the object afterwards.
     * \param view The chat view to be removed and deleted
     */
    virtual void removeChatView(BufferId) = 0;

    //! If true, the marker line will be set automatically on buffer switch
    /** \return Whether the marker line should be set on buffer switch
     */
    virtual inline bool autoMarkerLine() const { return true; }

protected slots:
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

    //! Show the given chat view
    /** This method is called when the given chat view should be displayed. Use this e.g. for
     *  selecting the appropriate page in a QStackedWidget.
     * \param view The chat view to be displayed. May be 0 if no chat view is selected.
     */
    virtual void showChatView(BufferId) = 0;

private slots:
    void removeBuffer(BufferId bufferId);
    void setCurrentBuffer(BufferId bufferId);

private:
    BufferId _currentBuffer;
    QHash<BufferId, AbstractChatView *> _chatViews;
};


class AbstractChatView
{
public:
    virtual ~AbstractChatView() {};
    virtual MsgId lastMsgId() const = 0;
};


#endif
