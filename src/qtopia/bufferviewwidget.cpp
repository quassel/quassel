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

#include "bufferviewwidget.h"
#include "client.h"


BufferViewWidget::BufferViewWidget(QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);
  setModal(true);
  setStyleSheet("background-color: rgba(220, 220, 255, 40%); color: rgb(0, 0, 0); font-size: 5pt;");
  //ui.tabWidget->tabBar()->setStyleSheet("font-size: 5pt;");

  // get rid of the default tab page designer forces upon us :(
  QWidget *w = ui.tabWidget->widget(0);
  ui.tabWidget->removeTab(0);
  delete w;

  addPage(tr("All"), BufferViewFilter::AllNets, QList<uint>());
  addPage(tr("Chans"), BufferViewFilter::AllNets|BufferViewFilter::NoQueries|BufferViewFilter::NoServers, QList<uint>());
  addPage(tr("Queries"), BufferViewFilter::AllNets|BufferViewFilter::NoChannels|BufferViewFilter::NoServers, QList<uint>());
  addPage(tr("Nets"), BufferViewFilter::AllNets|BufferViewFilter::NoChannels|BufferViewFilter::NoQueries, QList<uint>());

  // this sometimes deadlocks, so we have to hide the dialog from the outside:
  //connect(Client::bufferModel(), SIGNAL(bufferSelected(Buffer *)), this, SLOT(accept()));

}

BufferViewWidget::~BufferViewWidget() {


}

void BufferViewWidget::addPage(const QString &title, const BufferViewFilter::Modes &mode, const QList<uint> &nets) {
  BufferView *view = new BufferView(ui.tabWidget);
  view->setFilteredModel(Client::bufferModel(), mode, nets);
  Client::bufferModel()->synchronizeView(view);
  ui.tabWidget->addTab(view, title);
}

void BufferViewWidget::accept() {
  QDialog::accept();
}
