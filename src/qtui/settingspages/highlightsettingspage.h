/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Highlight Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Highlight Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Highlight Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _HIGHLIGHTSETTINGSPAGE_H_
#define _HIGHLIGHTSETTINGSPAGE_H_

#include <QVariantList>
#include <QTableWidgetItem>

#include "settingspage.h"
#include "ui_highlightsettingspage.h"

class HighlightSettingsPage : public SettingsPage {
  Q_OBJECT

  public:
    HighlightSettingsPage(QWidget *parent = 0);

    bool hasDefaults() const;

  public slots:
    void save();
    void load();
    void defaults();

  private slots:
    void widgetHasChanged();
    void addNewRow(QString name = tr("highlight rule"), bool regex = false, bool cs = true, bool enable = true);
    void removeSelectedRows();
    void selectRow(QTableWidgetItem *item);
    void tableChanged(QTableWidgetItem *item);

  private:
    Ui::HighlightSettingsPage ui;
    QVariantList  highlightList;
    // QVariant -> QHash<QString, QVariant>:
    //    regex:  bool
    //    name:   QString
    //    enable: bool
    enum column {
      NameColumn = 0,
      RegExColumn = 1,
      CsColumn = 2,
      EnableColumn = 3,
      ColumnCount = 4
    };

    void emptyTable();

    bool testHasChanged();
};

#endif
