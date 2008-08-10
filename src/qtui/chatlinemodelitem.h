/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
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

#ifndef CHATLINEMODELITEM_H_
#define CHATLINEMODELITEM_H_

#include <QVector>
#include <QPair>

#include "chatlinemodel.h"
#include "uistyle.h"

class ChatLineModelItem : public MessageModelItem {

  public:

    ChatLineModelItem(const Message &);
    //virtual ~ChatLineModelItem() {};

    virtual QVariant data(int column, int role) const;
    virtual bool setData(int column, const QVariant &value, int role);

  private:
    void computeWrapList();

    struct ChatLinePart {
      QString plainText;
      UiStyle::FormatList formatList;
    };
    ChatLinePart _timestamp, _sender, _contents;

    ChatLineModel::WrapList _wrapList;
};

#endif
