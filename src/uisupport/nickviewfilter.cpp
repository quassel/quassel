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

#include "nickviewfilter.h"

#include "buffersettings.h"
#include "iconloader.h"
#include "networkmodel.h"

/******************************************************************************************
 * NickViewFilter
 ******************************************************************************************/
NickViewFilter::NickViewFilter(const BufferId &bufferId, NetworkModel *parent)
  : QSortFilterProxyModel(parent),
    _bufferId(bufferId),
    _userOnlineIcon(SmallIcon("im-user")),
    _userAwayIcon(SmallIcon("im-user-away")),
    _categoryOpIcon(SmallIcon("irc-operator")),
    _categoryVoiceIcon(SmallIcon("irc-voice")),
    _opIconLimit(UserCategoryItem::categoryFromModes("o")),
    _voiceIconLimit(UserCategoryItem::categoryFromModes("v"))
{
  setSourceModel(parent);
  setDynamicSortFilter(true);
  setSortCaseSensitivity(Qt::CaseInsensitive);
  setSortRole(TreeModel::SortRole);

  BufferSettings bufferSettings;
  _showUserStateIcons = bufferSettings.showUserStateIcons();
  bufferSettings.notify("ShowUserStateIcons", this, SLOT(showUserStateIconsChanged()));
}

bool NickViewFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  // root node, networkindexes, the bufferindex of the buffer this filter is active for and it's childs are accepted
  if(!source_parent.isValid())
    return true;

  QModelIndex source_child = source_parent.child(source_row, 0);
  return (sourceModel()->data(source_child, NetworkModel::BufferIdRole).value<BufferId>() == _bufferId);
}

QVariant NickViewFilter::data(const QModelIndex &index, int role) const {
  switch(role) {
  case Qt::DecorationRole:
    return icon(index);
  default:
    return QSortFilterProxyModel::data(index, role);
  }
}

QVariant NickViewFilter::icon(const QModelIndex &index) const {
  if(!_showUserStateIcons)
    return QVariant();

  if(index.column() != 0)
    return QVariant();

  QModelIndex source_index = mapToSource(index);
  NetworkModel::ItemType itemType = (NetworkModel::ItemType)sourceModel()->data(source_index, NetworkModel::ItemTypeRole).toInt();
  switch(itemType) {
  case NetworkModel::UserCategoryItemType:
    {
      int categoryId = sourceModel()->data(source_index, TreeModel::SortRole).toInt();
      if(categoryId <= _opIconLimit)
	return _categoryOpIcon;
      if(categoryId <= _voiceIconLimit)
	return _categoryVoiceIcon;
      return _userOnlineIcon;
    }
  case NetworkModel::IrcUserItemType:
    if(sourceModel()->data(source_index, NetworkModel::ItemActiveRole).toBool())
      return _userOnlineIcon;
    else
      return _userAwayIcon;
    break;
  default:
    return QVariant();
  };
}

void NickViewFilter::showUserStateIconsChanged() {
  BufferSettings bufferSettings;
  _showUserStateIcons = bufferSettings.showUserStateIcons();
}
