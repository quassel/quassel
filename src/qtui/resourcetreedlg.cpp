/***************************************************************************
 *   Copyright (C) 2005-2020 by the Quassel Project                        *
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

#include "resourcetreedlg.h"

#include <QDir>
#include <QList>

#include "treemodel.h"

namespace {

void addEntries(const QString& dir, AbstractTreeItem* parentItem)
{
    auto entries = QDir{dir}.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name | QDir::DirsFirst);
    QList<AbstractTreeItem*> itemList;
    for (auto&& entry : entries) {
        auto item = new SimpleTreeItem({entry.fileName(), entry.size()}, parentItem);
        itemList << item;
        if (entry.isDir()) {
            addEntries(entry.absoluteFilePath(), item);
        }
    }
    parentItem->newChilds(itemList);
}

}  // namespace

ResourceTreeDlg::ResourceTreeDlg(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

    // We can't use QFileSystemModel, because it doesn't support the virtual resource file system :(
    auto model = new TreeModel({tr("File"), tr("Size")}, this);
    addEntries(":/", model->root());
    ui.treeView->setModel(model);
    ui.treeView->resizeColumnToContents(0);
}
