/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#include "colorsettingspage.h"

#include "qtui.h"
#include "qtuisettings.h"
#include "colorbutton.h"

#include <QColorDialog>
#include <QPainter>

// #define PHONDEV

ColorSettingsPage::ColorSettingsPage(QWidget *parent)
  : SettingsPage(tr("Appearance"), tr("Color settings"), parent) {
  ui.setupUi(this);

  mapper = new QSignalMapper(this);
  //Bufferview tab:
  connect(ui.inactiveActivityFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.inactiveActivityBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.inactiveActivityUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.noActivityFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.noActivityBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.noActivityUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.highlightActivityFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.highlightActivityBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.highlightActivityUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.newMessageActivityFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.newMessageActivityBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.newMessageActivityUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.otherActivityFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.otherActivityBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.otherActivityUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));

  mapper->setMapping(ui.inactiveActivityFG, ui.inactiveActivityFG);
  mapper->setMapping(ui.inactiveActivityBG, ui.inactiveActivityBG);
  mapper->setMapping(ui.highlightActivityFG, ui.highlightActivityFG);
  mapper->setMapping(ui.highlightActivityBG, ui.highlightActivityBG);
  mapper->setMapping(ui.newMessageActivityFG, ui.newMessageActivityFG);
  mapper->setMapping(ui.newMessageActivityBG, ui.newMessageActivityBG);
  mapper->setMapping(ui.noActivityFG, ui.noActivityFG);
  mapper->setMapping(ui.noActivityBG, ui.noActivityBG);
  mapper->setMapping(ui.otherActivityFG, ui.otherActivityFG);
  mapper->setMapping(ui.otherActivityBG, ui.otherActivityBG);


  //Chatview tab:
  connect(ui.errorMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.errorMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.errorMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.noticeMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.noticeMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.noticeMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.plainMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.plainMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.plainMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.serverMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.serverMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.serverMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.actionMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.actionMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.actionMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.joinMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.joinMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.joinMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.kickMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.kickMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.kickMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.modeMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.modeMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.modeMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.partMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.partMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.partMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.quitMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.quitMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.quitMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.renameMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.renameMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.renameMessageUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.highlightColor, SIGNAL(clicked()), mapper, SLOT(map()));

  mapper->setMapping(ui.errorMessageFG, ui.errorMessageFG);
  mapper->setMapping(ui.errorMessageBG, ui.errorMessageBG);
  mapper->setMapping(ui.noticeMessageFG, ui.noticeMessageFG);
  mapper->setMapping(ui.noticeMessageBG, ui.noticeMessageBG);
  mapper->setMapping(ui.plainMessageFG, ui.plainMessageFG);
  mapper->setMapping(ui.plainMessageBG, ui.plainMessageBG);
  mapper->setMapping(ui.serverMessageFG, ui.serverMessageFG);
  mapper->setMapping(ui.serverMessageBG, ui.serverMessageBG);
  mapper->setMapping(ui.actionMessageFG, ui.actionMessageFG);
  mapper->setMapping(ui.actionMessageBG, ui.actionMessageBG);
  mapper->setMapping(ui.joinMessageFG, ui.joinMessageFG);
  mapper->setMapping(ui.joinMessageBG, ui.joinMessageBG);
  mapper->setMapping(ui.kickMessageFG, ui.kickMessageFG);
  mapper->setMapping(ui.kickMessageBG, ui.kickMessageBG);
  mapper->setMapping(ui.modeMessageFG, ui.modeMessageFG);
  mapper->setMapping(ui.modeMessageBG, ui.modeMessageBG);
  mapper->setMapping(ui.partMessageFG, ui.partMessageFG);
  mapper->setMapping(ui.partMessageBG, ui.partMessageBG);
  mapper->setMapping(ui.quitMessageFG, ui.quitMessageFG);
  mapper->setMapping(ui.quitMessageBG, ui.quitMessageBG);
  mapper->setMapping(ui.renameMessageFG, ui.renameMessageFG);
  mapper->setMapping(ui.renameMessageBG, ui.renameMessageBG);
  mapper->setMapping(ui.highlightColor, ui.highlightColor);

  //Message Layout tab:
  connect(ui.timestampFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.timestampBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.timestampUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.senderFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.senderBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.senderUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.nickFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.nickBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.nickUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.hostmaskFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.hostmaskBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.hostmaskUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.channelnameFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.channelnameBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.channelnameUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.modeFlagsFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.modeFlagsBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.modeFlagsUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.urlFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.urlBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.urlUseBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));

  mapper->setMapping(ui.timestampFG, ui.timestampFG);
  mapper->setMapping(ui.timestampBG, ui.timestampBG);
  mapper->setMapping(ui.senderFG, ui.senderFG);
  mapper->setMapping(ui.senderBG, ui.senderBG);
  mapper->setMapping(ui.nickFG, ui.nickFG);
  mapper->setMapping(ui.nickBG, ui.nickBG);
  mapper->setMapping(ui.hostmaskFG, ui.hostmaskFG);
  mapper->setMapping(ui.hostmaskBG, ui.hostmaskBG);
  mapper->setMapping(ui.channelnameFG, ui.channelnameFG);
  mapper->setMapping(ui.channelnameBG, ui.channelnameBG);
  mapper->setMapping(ui.modeFlagsFG, ui.modeFlagsFG);
  mapper->setMapping(ui.modeFlagsBG, ui.modeFlagsBG);
  mapper->setMapping(ui.urlFG, ui.urlFG);
  mapper->setMapping(ui.urlBG, ui.urlBG);

  //Mirc Color Codes tab:
  connect(ui.color0, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color1, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color2, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color3, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color4, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color5, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color6, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color7, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color8, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color9, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color10, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color11, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color12, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color13, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color14, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.color15, SIGNAL(clicked()), mapper, SLOT(map()));

  mapper->setMapping(ui.color0, ui.color0);
  mapper->setMapping(ui.color1, ui.color1);
  mapper->setMapping(ui.color2, ui.color2);
  mapper->setMapping(ui.color3, ui.color3);
  mapper->setMapping(ui.color4, ui.color4);
  mapper->setMapping(ui.color5, ui.color5);
  mapper->setMapping(ui.color6, ui.color6);
  mapper->setMapping(ui.color7, ui.color7);
  mapper->setMapping(ui.color8, ui.color8);
  mapper->setMapping(ui.color9, ui.color9);
  mapper->setMapping(ui.color10, ui.color10);
  mapper->setMapping(ui.color11, ui.color11);
  mapper->setMapping(ui.color12, ui.color12);
  mapper->setMapping(ui.color13, ui.color13);
  mapper->setMapping(ui.color14, ui.color14);
  mapper->setMapping(ui.color15, ui.color15);

  //NickView tab:
  connect(ui.onlineStatusFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.onlineStatusBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.onlineStatusBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  connect(ui.awayStatusFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.awayStatusBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.awayStatusBG, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));

  mapper->setMapping(ui.onlineStatusFG, ui.onlineStatusFG);
  mapper->setMapping(ui.onlineStatusBG, ui.onlineStatusBG);
  mapper->setMapping(ui.awayStatusFG, ui.awayStatusFG);
  mapper->setMapping(ui.awayStatusBG, ui.awayStatusBG);

  connect(mapper, SIGNAL(mapped(QWidget *)), this, SLOT(chooseColor(QWidget *)));

  //disable unused buttons:
#ifndef PHONDEV
  ui.inactiveActivityUseBG->setEnabled(false);
  ui.noActivityUseBG->setEnabled(false);
  ui.highlightActivityUseBG->setEnabled(false);
  ui.newMessageActivityUseBG->setEnabled(false);
  ui.otherActivityUseBG->setEnabled(false);

  ui.nickFG->setEnabled(false);
  ui.nickUseBG->setEnabled(false);
  ui.hostmaskFG->setEnabled(false);
  ui.hostmaskUseBG->setEnabled(false);
  ui.channelnameFG->setEnabled(false);
  ui.channelnameUseBG->setEnabled(false);
  ui.modeFlagsFG->setEnabled(false);
  ui.modeFlagsUseBG->setEnabled(false);
  ui.urlFG->setEnabled(false);
  ui.urlUseBG->setEnabled(false);

  ui.onlineStatusUseBG->setEnabled(false);
  ui.awayStatusUseBG->setEnabled(false);
#endif
}

bool ColorSettingsPage::hasDefaults() const {
  return true;
}

void ColorSettingsPage::defaults() {
  defaultBufferview();
  defaultServerActivity();
  defaultUserActivity();
  defaultMessage();
  defaultMircColorCodes();
  defaultNickview();
}

void ColorSettingsPage::defaultBufferview() {
  ui.inactiveActivityFG->setColor(QColor(Qt::gray));
  ui.inactiveActivityBG->setColor(QColor(Qt::white));
  ui.inactiveActivityBG->setEnabled(false);
  ui.inactiveActivityUseBG->setChecked(false);
  ui.noActivityFG->setColor(QColor(Qt::black));
  ui.noActivityBG->setColor(QColor(Qt::white));
  ui.noActivityBG->setEnabled(false);
  ui.noActivityUseBG->setChecked(false);
  ui.highlightActivityFG->setColor(QColor(Qt::magenta));
  ui.highlightActivityBG->setColor(QColor(Qt::white));
  ui.highlightActivityBG->setEnabled(false);
  ui.highlightActivityUseBG->setChecked(false);
  ui.newMessageActivityFG->setColor(QColor(Qt::green));
  ui.newMessageActivityBG->setColor(QColor(Qt::white));
  ui.newMessageActivityBG->setEnabled(false);
  ui.newMessageActivityUseBG->setChecked(false);
  ui.otherActivityFG->setColor(QColor(Qt::darkGreen));
  ui.otherActivityBG->setColor(QColor(Qt::white));
  ui.otherActivityBG->setEnabled(false);
  ui.otherActivityUseBG->setChecked(false);

  widgetHasChanged();
  bufferviewPreview();
}

void ColorSettingsPage::defaultServerActivity() {
  ui.errorMessageFG->setColor(QtUi::style()->format(UiStyle::ErrorMsg, Settings::Default).foreground().color());
  ui.errorMessageBG->setColor(QColor("white"));
  ui.errorMessageBG->setEnabled(false);
  ui.errorMessageUseBG->setChecked(false);
  ui.noticeMessageFG->setColor(QtUi::style()->format(UiStyle::NoticeMsg, Settings::Default).foreground().color());
  ui.noticeMessageBG->setColor(QColor("white"));
  ui.noticeMessageBG->setEnabled(false);
  ui.noticeMessageUseBG->setChecked(false);
  ui.plainMessageFG->setColor(QtUi::style()->format(UiStyle::PlainMsg, Settings::Default).foreground().color());
  ui.plainMessageBG->setColor(QColor("white"));
  ui.plainMessageBG->setEnabled(false);
  ui.plainMessageUseBG->setChecked(false);
  ui.serverMessageFG->setColor(QtUi::style()->format(UiStyle::ServerMsg, Settings::Default).foreground().color());
  ui.serverMessageBG->setColor(QColor("white"));
  ui.serverMessageBG->setEnabled(false);
  ui.serverMessageUseBG->setChecked(false);
  ui.highlightColor->setColor(QColor("lightcoral"));

  widgetHasChanged();
  chatviewPreview();
}

void ColorSettingsPage::defaultUserActivity() {
  ui.actionMessageFG->setColor(QtUi::style()->format(UiStyle::ActionMsg, Settings::Default).foreground().color());
  ui.actionMessageBG->setColor(QColor("white"));
  ui.actionMessageBG->setEnabled(false);
  ui.actionMessageUseBG->setChecked(false);
  ui.joinMessageFG->setColor(QtUi::style()->format(UiStyle::JoinMsg, Settings::Default).foreground().color());
  ui.joinMessageBG->setColor(QColor("white"));
  ui.joinMessageBG->setEnabled(false);
  ui.joinMessageUseBG->setChecked(false);
  ui.kickMessageFG->setColor(QtUi::style()->format(UiStyle::KickMsg, Settings::Default).foreground().color());
  ui.kickMessageBG->setColor(QColor("white"));
  ui.kickMessageBG->setEnabled(false);
  ui.kickMessageUseBG->setChecked(false);
  ui.modeMessageFG->setColor(QtUi::style()->format(UiStyle::ModeMsg, Settings::Default).foreground().color());
  ui.modeMessageBG->setColor(QColor("white"));
  ui.modeMessageBG->setEnabled(false);
  ui.modeMessageUseBG->setChecked(false);
  ui.partMessageFG->setColor(QtUi::style()->format(UiStyle::PartMsg, Settings::Default).foreground().color());
  ui.partMessageBG->setColor(QColor("white"));
  ui.partMessageBG->setEnabled(false);
  ui.partMessageUseBG->setChecked(false);
  ui.quitMessageFG->setColor(QtUi::style()->format(UiStyle::QuitMsg, Settings::Default).foreground().color());
  ui.quitMessageBG->setColor(QColor("white"));
  ui.quitMessageBG->setEnabled(false);
  ui.quitMessageUseBG->setChecked(false);
  ui.renameMessageFG->setColor(QtUi::style()->format(UiStyle::RenameMsg, Settings::Default).foreground().color());
  ui.renameMessageBG->setColor(QColor("white"));
  ui.renameMessageBG->setEnabled(false);
  ui.renameMessageUseBG->setChecked(false);

  widgetHasChanged();
  chatviewPreview();
}

void ColorSettingsPage::defaultMessage() {
  ui.timestampFG->setColor(QtUi::style()->format(UiStyle::Timestamp, Settings::Default).foreground().color());
  ui.timestampBG->setColor(QColor("white"));
  ui.timestampBG->setEnabled(false);
  ui.timestampUseBG->setChecked(false);
  ui.senderFG->setColor(QtUi::style()->format(UiStyle::Sender, Settings::Default).foreground().color());
  ui.senderBG->setColor(QColor("white"));
  ui.senderBG->setEnabled(false);
  ui.senderUseBG->setChecked(false);
  ui.nickFG->setColor(QColor("black"));
  ui.nickBG->setColor(QColor("white"));
  ui.nickBG->setEnabled(false);
  ui.nickUseBG->setChecked(false);
  ui.hostmaskFG->setColor(QColor("black"));
  ui.hostmaskBG->setColor(QColor("white"));
  ui.hostmaskBG->setEnabled(false);
  ui.hostmaskUseBG->setChecked(false);
  ui.channelnameFG->setColor(QColor("black"));
  ui.channelnameBG->setColor(QColor("white"));
  ui.channelnameBG->setEnabled(false);
  ui.channelnameUseBG->setChecked(false);
  ui.modeFlagsFG->setColor(QColor("black"));
  ui.modeFlagsBG->setColor(QColor("white"));
  ui.modeFlagsBG->setEnabled(false);
  ui.modeFlagsUseBG->setChecked(false);
  ui.urlFG->setColor(QColor("black"));
  ui.urlBG->setColor(QColor("white"));
  ui.urlBG->setEnabled(false);
  ui.urlUseBG->setChecked(false);

  widgetHasChanged();
  chatviewPreview();
}

void ColorSettingsPage::defaultMircColorCodes() {
  ui.color0->setColor(QtUi::style()->format(UiStyle::FgCol00, Settings::Default).foreground().color());
  ui.color1->setColor(QtUi::style()->format(UiStyle::FgCol01, Settings::Default).foreground().color());
  ui.color2->setColor(QtUi::style()->format(UiStyle::FgCol02, Settings::Default).foreground().color());
  ui.color3->setColor(QtUi::style()->format(UiStyle::FgCol03, Settings::Default).foreground().color());
  ui.color4->setColor(QtUi::style()->format(UiStyle::FgCol04, Settings::Default).foreground().color());
  ui.color5->setColor(QtUi::style()->format(UiStyle::FgCol05, Settings::Default).foreground().color());
  ui.color6->setColor(QtUi::style()->format(UiStyle::FgCol06, Settings::Default).foreground().color());
  ui.color7->setColor(QtUi::style()->format(UiStyle::FgCol07, Settings::Default).foreground().color());
  ui.color8->setColor(QtUi::style()->format(UiStyle::FgCol08, Settings::Default).foreground().color());
  ui.color9->setColor(QtUi::style()->format(UiStyle::FgCol09, Settings::Default).foreground().color());
  ui.color10->setColor(QtUi::style()->format(UiStyle::FgCol10, Settings::Default).foreground().color());
  ui.color11->setColor(QtUi::style()->format(UiStyle::FgCol11, Settings::Default).foreground().color());
  ui.color12->setColor(QtUi::style()->format(UiStyle::FgCol12, Settings::Default).foreground().color());
  ui.color13->setColor(QtUi::style()->format(UiStyle::FgCol13, Settings::Default).foreground().color());
  ui.color14->setColor(QtUi::style()->format(UiStyle::FgCol14, Settings::Default).foreground().color());
  ui.color15->setColor(QtUi::style()->format(UiStyle::FgCol15, Settings::Default).foreground().color());

  widgetHasChanged();
}

void ColorSettingsPage::defaultNickview() {
  ui.onlineStatusFG->setColor(QColor(Qt::black));
  ui.onlineStatusBG->setColor(QColor("white"));
  ui.onlineStatusBG->setEnabled(false);
  ui.onlineStatusUseBG->setChecked(false);
  ui.awayStatusFG->setColor(QColor(Qt::gray));
  ui.awayStatusBG->setColor(QColor("white"));
  ui.awayStatusBG->setEnabled(false);
  ui.awayStatusUseBG->setChecked(false);

  widgetHasChanged();
}

void ColorSettingsPage::load() {
  QtUiSettings s("QtUi/Colors");
  settings["InactiveActivityFG"] = s.value("inactiveActivityFG", QVariant(QColor(Qt::gray)));
  ui.inactiveActivityFG->setColor(settings["InactiveActivityFG"].value<QColor>());
  settings["InactiveActivityBG"] = s.value("inactiveActivityBG", QVariant(QColor(Qt::white)));
  ui.inactiveActivityBG->setColor(settings["InactiveActivityBG"].value<QColor>());
  settings["InactiveActivityUseBG"] = s.value("inactiveActivityUseBG");
  ui.inactiveActivityUseBG->setChecked(settings["InactiveActivityUseBG"].toBool());

  settings["NoActivityFG"] = s.value("noActivityFG", QVariant(QColor(Qt::black)));
  ui.noActivityFG->setColor(settings["NoActivityFG"].value<QColor>());
  settings["NoActivityBG"] = s.value("noActivityBG", QVariant(QColor(Qt::white)));
  ui.noActivityBG->setColor(settings["NoActivityBG"].value<QColor>());
  settings["NoActivityUseBG"] = s.value("noActivityUseBG");
  ui.noActivityUseBG->setChecked(settings["NoActivityUseBG"].toBool());

  settings["HighlightActivityFG"] = s.value("highlightActivityFG", QVariant(QColor(Qt::magenta)));
  ui.highlightActivityFG->setColor(settings["HighlightActivityFG"].value<QColor>());
  settings["HighlightActivityBG"] = s.value("highlightActivityBG", QVariant(QColor(Qt::white)));
  ui.highlightActivityBG->setColor(settings["HighlightActivityBG"].value<QColor>());
  settings["HighlightActivityUseBG"] = s.value("highlightActivityUseBG");
  ui.highlightActivityUseBG->setChecked(settings["HighlightActivityUseBG"].toBool());

  settings["NewMessageActivityFG"] = s.value("newMessageActivityFG", QVariant(QColor(Qt::green)));
  ui.newMessageActivityFG->setColor(settings["NewMessageActivityFG"].value<QColor>());
  settings["NewMessageActivityBG"] = s.value("newMessageActivityBG", QVariant(QColor(Qt::white)));
  ui.newMessageActivityBG->setColor(settings["NewMessageActivityBG"].value<QColor>());
  settings["NewMessageActivityUseBG"] = s.value("newMessageActivityUseBG");
  ui.newMessageActivityUseBG->setChecked(settings["NewMessageActivityUseBG"].toBool());

  settings["OtherActivityFG"] = s.value("otherActivityFG", QVariant(QColor(Qt::darkGreen)));
  ui.otherActivityFG->setColor(settings["OtherActivityFG"].value<QColor>());
  settings["OtherActivityBG"] = s.value("otherActivityBG", QVariant(QColor(Qt::white)));
  ui.otherActivityBG->setColor(settings["OtherActivityBG"].value<QColor>());
  settings["OtherActivityUseBG"] = s.value("otherActivityUseBG");
  ui.otherActivityUseBG->setChecked(settings["OtherActivityUseBG"].toBool());

  ui.actionMessageFG->setColor(QtUi::style()->format(UiStyle::ActionMsg).foreground().color());
  ui.errorMessageFG->setColor(QtUi::style()->format(UiStyle::ErrorMsg).foreground().color());
  ui.joinMessageFG->setColor(QtUi::style()->format(UiStyle::JoinMsg).foreground().color());
  ui.kickMessageFG->setColor(QtUi::style()->format(UiStyle::KickMsg).foreground().color());
  ui.modeMessageFG->setColor(QtUi::style()->format(UiStyle::ModeMsg).foreground().color());
  ui.noticeMessageFG->setColor(QtUi::style()->format(UiStyle::NoticeMsg).foreground().color());
  ui.partMessageFG->setColor(QtUi::style()->format(UiStyle::PartMsg).foreground().color());
  ui.plainMessageFG->setColor(QtUi::style()->format(UiStyle::PlainMsg).foreground().color());
  ui.quitMessageFG->setColor(QtUi::style()->format(UiStyle::QuitMsg).foreground().color());
  ui.renameMessageFG->setColor(QtUi::style()->format(UiStyle::RenameMsg).foreground().color());
  ui.serverMessageFG->setColor(QtUi::style()->format(UiStyle::ServerMsg).foreground().color());

  settings["ActionMessageBG"] = s.value("actionMessageBG", QVariant(QColor("white")));
  ui.actionMessageBG->setColor(settings["ActionMessageBG"].value<QColor>());
  settings["ActionMessageUseBG"] = s.value("actionMessageUseBG", QVariant(false));
  if(settings["ActionMessageUseBG"].toBool()) {
    ui.actionMessageUseBG->setChecked(true);
    ui.actionMessageBG->setEnabled(true);
  }
  settings["ErrorMessageBG"] = s.value("errorMessageBG", QVariant(QColor("white")));
  ui.errorMessageBG->setColor(settings["ErrorMessageBG"].value<QColor>());
  settings["ErrorMessageUseBG"] = s.value("errorMessageUseBG", QVariant(false));
  if(settings["ErrorMessageUseBG"].toBool()) {
    ui.errorMessageUseBG->setChecked(true);
    ui.errorMessageBG->setEnabled(true);
  }
  settings["JoinMessageBG"] = s.value("joinMessageBG", QVariant(QColor("white")));
  ui.joinMessageBG->setColor(settings["JoinMessageBG"].value<QColor>());
  settings["JoinMessageUseBG"] = s.value("joinMessageUseBG", QVariant(false));
  if(settings["JoinMessageUseBG"].toBool()) {
    ui.joinMessageUseBG->setChecked(true);
    ui.joinMessageBG->setEnabled(true);
  }
  settings["KickMessageBG"] = s.value("kickMessageBG", QVariant(QColor("white")));
  ui.kickMessageBG->setColor(settings["KickMessageBG"].value<QColor>());
  settings["KickMessageUseBG"] = s.value("kickMessageUseBG", QVariant(false));
  if(settings["KickMessageUseBG"].toBool()) {
    ui.kickMessageUseBG->setChecked(true);
    ui.kickMessageBG->setEnabled(true);
  }
  settings["ModeMessageBG"] = s.value("modeMessageBG", QVariant(QColor("white")));
  ui.modeMessageBG->setColor(settings["ModeMessageBG"].value<QColor>());
  settings["ModeMessageUseBG"] = s.value("modeMessageUseBG", QVariant(false));
  if(settings["ModeMessageUseBG"].toBool()) {
    ui.modeMessageUseBG->setChecked(true);
    ui.modeMessageBG->setEnabled(true);
  }
  settings["NoticeMessageBG"] = s.value("noticeMessageBG", QVariant(QColor("white")));
  ui.noticeMessageBG->setColor(settings["NoticeMessageBG"].value<QColor>());
  settings["NoticeMessageUseBG"] = s.value("noticeMessageUseBG", QVariant(false));
  if(settings["NoticeMessageUseBG"].toBool()) {
    ui.noticeMessageUseBG->setChecked(true);
    ui.noticeMessageBG->setEnabled(true);
  }
  settings["PartMessageBG"] = s.value("partMessageBG", QVariant(QColor("white")));
  ui.partMessageBG->setColor(settings["PartMessageBG"].value<QColor>());
  settings["PartMessageUseBG"] = s.value("partMessageUseBG", QVariant(false));
  if(settings["PartMessageUseBG"].toBool()) {
    ui.partMessageUseBG->setChecked(true);
    ui.partMessageBG->setEnabled(true);
  }
  settings["PlainMessageBG"] = s.value("plainMessageBG", QVariant(QColor("white")));
  ui.plainMessageBG->setColor(settings["PlainMessageBG"].value<QColor>());
  settings["PlainMessageUseBG"] = s.value("plainMessageUseBG", QVariant(false));
  if(settings["PlainMessageUseBG"].toBool()) {
    ui.plainMessageUseBG->setChecked(true);
    ui.plainMessageBG->setEnabled(true);
  }
  settings["QuitMessageBG"] = s.value("quitMessageBG", QVariant(QColor("white")));
  ui.quitMessageBG->setColor(settings["QuitMessageBG"].value<QColor>());
  settings["QuitMessageUseBG"] = s.value("quitMessageUseBG", QVariant(false));
  if(settings["QuitMessageUseBG"].toBool()) {
    ui.quitMessageUseBG->setChecked(true);
    ui.quitMessageBG->setEnabled(true);
  }
  settings["RenameMessageBG"] = s.value("renameMessageBG", QVariant(QColor("white")));
  ui.renameMessageBG->setColor(settings["RenameMessageBG"].value<QColor>());
  settings["RenameMessageUseBG"] = s.value("renameMessageUseBG", QVariant(false));
  if(settings["RenameMessageUseBG"].toBool()) {
    ui.renameMessageUseBG->setChecked(true);
    ui.renameMessageBG->setEnabled(true);
  }
  settings["ServerMessageBG"] = s.value("serverMessageBG", QVariant(QColor("white")));
  ui.serverMessageBG->setColor(settings["ServerMessageBG"].value<QColor>());
  settings["ServerMessageUseBG"] = s.value("serverMessageUseBG", QVariant(false));
  if(settings["ServerMessageUseBG"].toBool()) {
    ui.serverMessageUseBG->setChecked(true);
    ui.serverMessageBG->setEnabled(true);
  }

  ui.timestampFG->setColor(QtUi::style()->format(UiStyle::Timestamp).foreground().color());
  ui.senderFG->setColor(QtUi::style()->format(UiStyle::Sender).foreground().color());

  settings["TimestampBG"] = s.value("timestampBG", QVariant(QColor("white")));
  ui.timestampBG->setColor(settings["TimestampBG"].value<QColor>());
  settings["TimestampUseBG"] = s.value("timestampUseBG", QVariant(false));
  if(settings["TimestampUseBG"].toBool()) {
    ui.timestampUseBG->setChecked(true);
    ui.timestampBG->setEnabled(true);
  }
  settings["SenderBG"] = s.value("senderBG", QVariant(QColor("white")));
  ui.senderBG->setColor(settings["SenderBG"].value<QColor>());
  settings["SenderUseBG"] = s.value("senderUseBG", QVariant(false));
  if(settings["SenderUseBG"].toBool()) {
    ui.senderUseBG->setChecked(true);
    ui.senderBG ->setEnabled(true);
  }
  settings["NickFG"] = s.value("nickFG", QVariant(QColor("black")));
  ui.nickFG->setColor(settings["NickFG"].value<QColor>());
  settings["NickBG"] = s.value("nickBG", QVariant(QColor("white")));
  ui.nickBG->setColor(settings["NickBG"].value<QColor>());
  settings["HostmaskFG"] = s.value("hostmaskFG", QVariant(QColor("black")));
  ui.hostmaskFG->setColor(settings["HostmaskFG"].value<QColor>());
  settings["HostmaskBG"] = s.value("hostmaskBG", QVariant(QColor("white")));
  ui.hostmaskBG->setColor(settings["HostmaskBG"].value<QColor>());
  settings["ChannelnameFG"] = s.value("channelnameFG", QVariant(QColor("black")));
  ui.channelnameFG->setColor(settings["ChannelnameFG"].value<QColor>());
  settings["ChannelnameBG"] = s.value("channelnameBG", QVariant(QColor("white")));
  ui.channelnameBG->setColor(settings["ChannelnameBG"].value<QColor>());
  settings["ModeFlagsFG"] = s.value("modeFlagsFG", QVariant(QColor("black")));
  ui.modeFlagsFG->setColor(settings["ModeFlagsFG"].value<QColor>());
  settings["ModeFlagsBG"] = s.value("modeFlagsBG", QVariant(QColor("white")));
  ui.modeFlagsBG->setColor(settings["ModeFlagsBG"].value<QColor>());
  settings["UrlFG"] = s.value("urlFG", QVariant(QColor("black")));
  ui.urlFG->setColor(settings["UrlFG"].value<QColor>());
  settings["UrlBG"] = s.value("urlBG", QVariant(QColor("white")));
  ui.urlBG->setColor(settings["UrlBG"].value<QColor>());

  settings["HighlightColor"] = s.value("highlightColor", QVariant(QColor("lightcoral")));
  ui.highlightColor->setColor(settings["HighlightColor"].value<QColor>());

  ui.color0->setColor(QtUi::style()->format(UiStyle::FgCol00).foreground().color());
  ui.color1->setColor(QtUi::style()->format(UiStyle::FgCol01).foreground().color());
  ui.color2->setColor(QtUi::style()->format(UiStyle::FgCol02).foreground().color());
  ui.color3->setColor(QtUi::style()->format(UiStyle::FgCol03).foreground().color());
  ui.color4->setColor(QtUi::style()->format(UiStyle::FgCol04).foreground().color());
  ui.color5->setColor(QtUi::style()->format(UiStyle::FgCol05).foreground().color());
  ui.color6->setColor(QtUi::style()->format(UiStyle::FgCol06).foreground().color());
  ui.color7->setColor(QtUi::style()->format(UiStyle::FgCol07).foreground().color());
  ui.color8->setColor(QtUi::style()->format(UiStyle::FgCol08).foreground().color());
  ui.color9->setColor(QtUi::style()->format(UiStyle::FgCol09).foreground().color());
  ui.color10->setColor(QtUi::style()->format(UiStyle::FgCol10).foreground().color());
  ui.color11->setColor(QtUi::style()->format(UiStyle::FgCol11).foreground().color());
  ui.color12->setColor(QtUi::style()->format(UiStyle::FgCol12).foreground().color());
  ui.color13->setColor(QtUi::style()->format(UiStyle::FgCol13).foreground().color());
  ui.color14->setColor(QtUi::style()->format(UiStyle::FgCol14).foreground().color());
  ui.color15->setColor(QtUi::style()->format(UiStyle::FgCol15).foreground().color());

  settings["OnlineStatusFG"] = s.value("onlineStatusFG", QVariant(QColor(Qt::black)));
  ui.onlineStatusFG->setColor(settings["OnlineStatusFG"].value<QColor>());
  settings["OnlineStatusBG"] = s.value("onlineStatusBG", QVariant(QColor(Qt::white)));
  ui.onlineStatusBG->setColor(settings["OnlineStatusBG"].value<QColor>());
  settings["OnlineStatusUseBG"] = s.value("onlineStatusUseBG");
  ui.onlineStatusUseBG->setChecked(settings["OnlineStatusUseBG"].toBool());

  settings["AwayStatusFG"] = s.value("awayStatusFG", QVariant(QColor(Qt::gray)));
  ui.awayStatusFG->setColor(settings["AwayStatusFG"].value<QColor>());
  settings["AwayStatusBG"] = s.value("awayStatusBG", QVariant(QColor(Qt::white)));
  ui.awayStatusBG->setColor(settings["AwayStatusBG"].value<QColor>());
  settings["AwayStatusUseBG"] = s.value("awayStatusUseBG");
  ui.awayStatusUseBG->setChecked(settings["AwayStatusUseBG"].toBool());
  setChangedState(false);

  bufferviewPreview();
  chatviewPreview();
}

void ColorSettingsPage::save() {
  QtUiSettings s("QtUi/Colors");
  s.setValue("noActivityFG", ui.noActivityFG->color());
  s.setValue("noActivityBG", ui.noActivityBG->color());
  s.setValue("noActivityUseBG", ui.noActivityUseBG->isChecked());
  s.setValue("inactiveActivityFG", ui.inactiveActivityFG->color());
  s.setValue("inactiveActivityBG", ui.inactiveActivityBG->color());
  s.setValue("inactiveActivityUseBG", ui.inactiveActivityUseBG->isChecked());
  s.setValue("highlightActivityFG", ui.highlightActivityFG->color());
  s.setValue("highlightActivityBG", ui.highlightActivityBG->color());
  s.setValue("highlightActivityUseBG", ui.highlightActivityUseBG->isChecked());
  s.setValue("newMessageActivityFG", ui.newMessageActivityFG->color());
  s.setValue("newMessageActivityBG", ui.newMessageActivityBG->color());
  s.setValue("newMessageActivityUseBG", ui.newMessageActivityUseBG->isChecked());
  s.setValue("otherActivityFG", ui.otherActivityFG->color());
  s.setValue("otherActivityBG", ui.otherActivityBG->color());
  s.setValue("otherActivityUseBG", ui.otherActivityUseBG->isChecked());

  saveColor(true, UiStyle::ErrorMsg, ui.errorMessageFG->color());
  saveColor(false, UiStyle::ErrorMsg, ui.errorMessageBG->color(), ui.errorMessageUseBG->isChecked());
  s.setValue("errorMessageBG", ui.errorMessageBG->color());
  s.setValue("errorMessageUseBG", ui.errorMessageUseBG->isChecked());
  saveColor(true, UiStyle::NoticeMsg, ui.noticeMessageFG->color());
  saveColor(false, UiStyle::NoticeMsg, ui.noticeMessageBG->color(), ui.noticeMessageUseBG->isChecked());
  s.setValue("noticeMessageBG", ui.noticeMessageBG->color());
  s.setValue("noticeMessageUseBG", ui.noticeMessageUseBG->isChecked());
  saveColor(true, UiStyle::PlainMsg, ui.plainMessageFG->color());
  saveColor(false, UiStyle::PlainMsg, ui.plainMessageBG->color(), ui.plainMessageUseBG->isChecked());
  s.setValue("plainMessageBG", ui.plainMessageBG->color());
  s.setValue("plainMessageUseBG", ui.plainMessageUseBG->isChecked());
  saveColor(true, UiStyle::ServerMsg, ui.serverMessageFG->color());
  saveColor(false, UiStyle::ServerMsg, ui.serverMessageBG->color(), ui.serverMessageUseBG->isChecked());
  s.setValue("serverMessageBG", ui.serverMessageBG->color());
  s.setValue("serverMessageUseBG", ui.serverMessageUseBG->isChecked());
  saveColor(true, UiStyle::ActionMsg, ui.actionMessageFG->color());
  saveColor(false, UiStyle::ActionMsg, ui.actionMessageBG->color(), ui.actionMessageUseBG->isChecked());
  s.setValue("actionMessageBG", ui.actionMessageBG->color());
  s.setValue("actionMessageUseBG", ui.actionMessageUseBG->isChecked());
  saveColor(true, UiStyle::JoinMsg, ui.joinMessageFG->color());
  saveColor(false, UiStyle::JoinMsg, ui.joinMessageBG->color(), ui.joinMessageUseBG->isChecked());
  s.setValue("joinMessageBG", ui.joinMessageBG->color());
  s.setValue("joinMessageUseBG", ui.joinMessageUseBG->isChecked());
  saveColor(true, UiStyle::KickMsg, ui.kickMessageFG->color());
  saveColor(false, UiStyle::KickMsg, ui.kickMessageBG->color(), ui.kickMessageUseBG->isChecked());
  s.setValue("kickMessageBG", ui.kickMessageBG->color());
  s.setValue("kickMessageUseBG", ui.kickMessageUseBG->isChecked());
  saveColor(true, UiStyle::ModeMsg, ui.modeMessageFG->color());
  saveColor(false, UiStyle::ModeMsg, ui.modeMessageBG->color(), ui.modeMessageUseBG->isChecked());
  s.setValue("modeMessageBG", ui.modeMessageBG->color());
  s.setValue("modeMessageUseBG", ui.modeMessageUseBG->isChecked());
  saveColor(true, UiStyle::PartMsg, ui.partMessageFG->color());
  saveColor(false, UiStyle::PartMsg, ui.partMessageBG->color(), ui.partMessageUseBG->isChecked());
  s.setValue("partMessageBG", ui.partMessageBG->color());
  s.setValue("partMessageUseBG", ui.partMessageUseBG->isChecked());
  saveColor(true, UiStyle::QuitMsg, ui.quitMessageFG->color());
  saveColor(false, UiStyle::QuitMsg, ui.quitMessageBG->color(), ui.quitMessageUseBG->isChecked());
  s.setValue("quitMessageBG", ui.quitMessageBG->color());
  s.setValue("quitMessageUseBG", ui.quitMessageUseBG->isChecked());
  saveColor(true, UiStyle::RenameMsg, ui.renameMessageFG->color());
  saveColor(false, UiStyle::RenameMsg, ui.renameMessageBG->color(), ui.renameMessageUseBG->isChecked());
  s.setValue("renameMessageBG", ui.renameMessageBG->color());
  s.setValue("renameMessageUseBG", ui.renameMessageUseBG->isChecked());

  s.setValue("highlightColor", ui.highlightColor->color());

  saveColor(true, UiStyle::Timestamp, ui.timestampFG->color());
  saveColor(false, UiStyle::Timestamp, ui.timestampBG->color(), ui.timestampUseBG->isChecked());
  s.setValue("timestampBG", ui.timestampBG->color());
  s.setValue("timestampUseBG", ui.timestampUseBG->isChecked());
  saveColor(true, UiStyle::Sender, ui.senderFG->color());
  saveColor(false, UiStyle::Sender, ui.senderBG->color(), ui.senderUseBG->isChecked());
  s.setValue("senderBG", ui.senderBG->color());
  s.setValue("senderUseBG", ui.senderUseBG->isChecked());
  s.setValue("nickFG", ui.nickFG->color());
  s.setValue("nickBG", ui.nickBG->color());
  s.setValue("nickUseBG", ui.nickUseBG->isChecked());
  s.setValue("hostmaskFG", ui.hostmaskFG->color());
  s.setValue("hostmaskBG", ui.hostmaskBG->color());
  s.setValue("hostmaskUseBG", ui.hostmaskUseBG->isChecked());
  s.setValue("channelnameFG", ui.channelnameFG->color());
  s.setValue("channelnameBG", ui.channelnameBG->color());
  s.setValue("channelnameUseBG", ui.channelnameUseBG->isChecked());
  s.setValue("modeFlagsFG", ui.modeFlagsFG->color());
  s.setValue("modeFlagsBG", ui.modeFlagsBG->color());
  s.setValue("modeFlagsUseBG", ui.modeFlagsUseBG->isChecked());
  s.setValue("urlFG", ui.urlFG->color());
  s.setValue("urlBG", ui.urlBG->color());
  s.setValue("urlUseBG", ui.urlUseBG->isChecked());

  saveMircColor(true, UiStyle::FgCol00, ui.color0->color());
  saveMircColor(true, UiStyle::FgCol01, ui.color1->color());
  saveMircColor(true, UiStyle::FgCol02, ui.color2->color());
  saveMircColor(true, UiStyle::FgCol03, ui.color3->color());
  saveMircColor(true, UiStyle::FgCol04, ui.color4->color());
  saveMircColor(true, UiStyle::FgCol05, ui.color5->color());
  saveMircColor(true, UiStyle::FgCol06, ui.color6->color());
  saveMircColor(true, UiStyle::FgCol07, ui.color7->color());
  saveMircColor(true, UiStyle::FgCol08, ui.color8->color());
  saveMircColor(true, UiStyle::FgCol09, ui.color9->color());
  saveMircColor(true, UiStyle::FgCol10, ui.color10->color());
  saveMircColor(true, UiStyle::FgCol11, ui.color11->color());
  saveMircColor(true, UiStyle::FgCol12, ui.color12->color());
  saveMircColor(true, UiStyle::FgCol13, ui.color13->color());
  saveMircColor(true, UiStyle::FgCol14, ui.color14->color());
  saveMircColor(true, UiStyle::FgCol15, ui.color15->color());

  saveMircColor(false, UiStyle::BgCol00, ui.color0->color());
  saveMircColor(false, UiStyle::BgCol01, ui.color1->color());
  saveMircColor(false, UiStyle::BgCol02, ui.color2->color());
  saveMircColor(false, UiStyle::BgCol03, ui.color3->color());
  saveMircColor(false, UiStyle::BgCol04, ui.color4->color());
  saveMircColor(false, UiStyle::BgCol05, ui.color5->color());
  saveMircColor(false, UiStyle::BgCol06, ui.color6->color());
  saveMircColor(false, UiStyle::BgCol07, ui.color7->color());
  saveMircColor(false, UiStyle::BgCol08, ui.color8->color());
  saveMircColor(false, UiStyle::BgCol09, ui.color9->color());
  saveMircColor(false, UiStyle::BgCol10, ui.color10->color());
  saveMircColor(false, UiStyle::BgCol11, ui.color11->color());
  saveMircColor(false, UiStyle::BgCol12, ui.color12->color());
  saveMircColor(false, UiStyle::BgCol13, ui.color13->color());
  saveMircColor(false, UiStyle::BgCol14, ui.color14->color());
  saveMircColor(false, UiStyle::BgCol15, ui.color15->color());

  s.setValue("onlineStatusFG", ui.onlineStatusFG->color());
  s.setValue("onlineStatusBG", ui.onlineStatusBG->color());
  s.setValue("onlineStatusUseBG", ui.onlineStatusUseBG->isChecked());
  s.setValue("awayStatusFG", ui.awayStatusFG->color());
  s.setValue("awayStatusBG", ui.awayStatusBG->color());
  s.setValue("awayStatusUseBG", ui.awayStatusUseBG->isChecked());

  load(); //TODO: remove when settings hash map is unnescessary
  setChangedState(false);
}

void ColorSettingsPage::saveColor(bool foreground, UiStyle::FormatType formatType, const QColor &color, bool enableColor) {
  QTextCharFormat format = QtUi::style()->format(formatType);
  if(foreground) {
    if(enableColor) {
      format.setForeground(QBrush(color));
    } else {
      format.clearForeground();
    }
  } else {
    if(enableColor) {
      format.setBackground(QBrush(color));
    } else {
      format.clearBackground();
    }
  }
  QtUi::style()->setFormat(formatType, format, Settings::Custom);
}

//TODO: to be removed and exchanged by saveColor
void ColorSettingsPage::saveMircColor(bool foreground, UiStyle::FormatType formatType, const QColor &color) {
  QTextCharFormat format = QtUi::style()->format(formatType);
  if(foreground) {
    format.setForeground(QBrush(color));
    format.clearBackground();
  } else {
    format.clearForeground();
    format.setBackground(QBrush(color));
  }
  QtUi::style()->setFormat(formatType, format, Settings::Custom);
}


void ColorSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) {
    setChangedState(changed);
  }
  bufferviewPreview();
  chatviewPreview();
}

bool ColorSettingsPage::testHasChanged() {
  if(settings["InactiveActivityFG"].value<QColor>() != ui.inactiveActivityFG->color()) return true;
  if(settings["InactiveActivityBG"].value<QColor>() != ui.inactiveActivityBG->color()) return true;
  if(settings["InactiveActivityUseBG"].toBool() != ui.inactiveActivityUseBG->isChecked()) return true;
  if(settings["NoActivityFG"].value<QColor>() != ui.noActivityFG->color()) return true;
  if(settings["NoActivityBG"].value<QColor>() != ui.noActivityBG->color()) return true;
  if(settings["NoActivityUseBG"].toBool() != ui.noActivityUseBG->isChecked()) return true;
  if(settings["HighlightActivityFG"].value<QColor>() != ui.highlightActivityFG->color()) return true;
  if(settings["HighlightActivityBG"].value<QColor>() != ui.highlightActivityBG->color()) return true;
  if(settings["HighlightActivityUseBG"].toBool() != ui.highlightActivityUseBG->isChecked()) return true;
  if(settings["NewMessageActivityFG"].value<QColor>() != ui.newMessageActivityFG->color()) return true;
  if(settings["NewMessageActivityBG"].value<QColor>() != ui.newMessageActivityBG->color()) return true;
  if(settings["NewMessageActivityUseBG"].toBool() != ui.newMessageActivityUseBG->isChecked()) return true;
  if(settings["OtherActivityFG"].value<QColor>() != ui.otherActivityFG->color()) return true;
  if(settings["OtherActivityBG"].value<QColor>() != ui.otherActivityBG->color()) return true;
  if(settings["OtherActivityUseBG"].toBool() != ui.otherActivityUseBG->isChecked()) return true;

  if(QtUi::style()->format(UiStyle::ErrorMsg).foreground().color() != ui.errorMessageFG->color()) return true;
  if(settings["ErrorMessageBG"].value<QColor>() != ui.errorMessageBG->color()) return true;
  if(settings["ErrorMessageUseBG"].toBool() != ui.errorMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::NoticeMsg).foreground().color() != ui.noticeMessageFG->color()) return true;
  if(settings["NoticeMessageBG"].value<QColor>() != ui.noticeMessageBG->color()) return true;
  if(settings["NoticeMessageUseBG"].toBool() != ui.noticeMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::PlainMsg).foreground().color() != ui.plainMessageFG->color()) return true;
  if(settings["PlainMessageBG"].value<QColor>() != ui.plainMessageBG->color()) return true;
  if(settings["PlainMessageUseBG"].toBool() != ui.plainMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::ServerMsg).foreground().color() != ui.serverMessageFG->color()) return true;
  if(settings["ServerMessageBG"].value<QColor>() != ui.serverMessageBG->color()) return true;
  if(settings["ServerMessageUseBG"].toBool() != ui.serverMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::ActionMsg).foreground().color() != ui.actionMessageFG->color()) return true;
  if(settings["ActionMessageBG"].value<QColor>() != ui.actionMessageBG->color()) return true;
  if(settings["ActionMessageUseBG"].toBool() != ui.actionMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::JoinMsg).foreground().color() != ui.joinMessageFG->color()) return true;
  if(settings["JoinMessageBG"].value<QColor>() != ui.joinMessageBG->color()) return true;
  if(settings["JoinMessageUseBG"].toBool() != ui.joinMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::KickMsg).foreground().color() != ui.kickMessageFG->color()) return true;
  if(settings["KickMessageBG"].value<QColor>() != ui.kickMessageBG->color()) return true;
  if(settings["KickMessageUseBG"].toBool() != ui.kickMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::ModeMsg).foreground().color() != ui.modeMessageFG->color()) return true;
  if(settings["ModeMessageBG"].value<QColor>() != ui.modeMessageBG->color()) return true;
  if(settings["ModeMessageUseBG"].toBool() != ui.modeMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::PartMsg).foreground().color() != ui.partMessageFG->color()) return true;
  if(settings["PartMessageBG"].value<QColor>() != ui.partMessageBG->color()) return true;
  if(settings["PartMessageUseBG"].toBool() != ui.partMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::QuitMsg).foreground().color() != ui.quitMessageFG->color()) return true;
  if(settings["QuitMessageBG"].value<QColor>() != ui.quitMessageBG->color()) return true;
  if(settings["QuitMessageUseBG"].toBool() != ui.quitMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::RenameMsg).foreground().color() != ui.renameMessageFG->color()) return true;
  if(settings["RenameMessageBG"].value<QColor>() != ui.renameMessageBG->color()) return true;
  if(settings["RenameMessageUseBG"].toBool() != ui.renameMessageUseBG->isChecked()) return true;

  if(settings["HighlightColor"].value<QColor>() != ui.highlightColor->color()) return true;

  if(QtUi::style()->format(UiStyle::Timestamp).foreground().color() != ui.timestampFG->color()) return true;
  if(settings["TimestampBG"].value<QColor>() != ui.timestampBG->color()) return true;
  if(settings["TimestampUseBG"].toBool() != ui.timestampUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::Sender).foreground().color() != ui.senderFG->color()) return true;
  if(settings["SenderBG"].value<QColor>() != ui.senderBG->color()) return true;
  if(settings["SenderUseBG"].toBool() != ui.senderUseBG->isChecked()) return true;

  if(settings["NickFG"].value<QColor>() != ui.nickFG->color()) return true;
  if(settings["NickBG"].value<QColor>() != ui.nickBG->color()) return true;
  if(settings["HostmaskFG"].value<QColor>() != ui.hostmaskFG->color()) return true;
  if(settings["HostmaskBG"].value<QColor>() != ui.hostmaskBG->color()) return true;
  if(settings["ChannelnameFG"].value<QColor>() != ui.channelnameFG->color()) return true;
  if(settings["ChannelnameBG"].value<QColor>() != ui.channelnameBG->color()) return true;
  if(settings["ModeFlagsFG"].value<QColor>() != ui.modeFlagsFG->color()) return true;
  if(settings["ModeFlagsBG"].value<QColor>() != ui.modeFlagsBG->color()) return true;
  if(settings["UrlFG"].value<QColor>() != ui.urlFG->color()) return true;
  if(settings["UrlBG"].value<QColor>() != ui.urlBG->color()) return true;

  if(QtUi::style()->format(UiStyle::FgCol00).foreground().color() != ui.color0->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol01).foreground().color() != ui.color1->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol02).foreground().color() != ui.color2->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol03).foreground().color() != ui.color3->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol04).foreground().color() != ui.color4->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol05).foreground().color() != ui.color5->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol06).foreground().color() != ui.color6->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol07).foreground().color() != ui.color7->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol08).foreground().color() != ui.color8->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol09).foreground().color() != ui.color9->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol10).foreground().color() != ui.color10->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol11).foreground().color() != ui.color11->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol12).foreground().color() != ui.color12->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol13).foreground().color() != ui.color13->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol14).foreground().color() != ui.color14->color()) return true;
  if(QtUi::style()->format(UiStyle::FgCol15).foreground().color() != ui.color15->color()) return true;

  if(settings["OnlineStatusFG"].value<QColor>() != ui.onlineStatusFG->color()) return true;
  if(settings["OnlineStatusBG"].value<QColor>() != ui.onlineStatusBG->color()) return true;
  if(settings["OnlineStatusUseBG"].toBool() != ui.onlineStatusUseBG->isChecked()) return true;
  if(settings["AwayStatusFG"].value<QColor>() != ui.awayStatusFG->color()) return true;
  if(settings["AwayStatusBG"].value<QColor>() != ui.awayStatusBG->color()) return true;
  if(settings["AwayStatusUseBG"].toBool() != ui.awayStatusUseBG->isChecked()) return true;

  return false;
}

void ColorSettingsPage::chooseColor(QWidget *widget) {
  ColorButton *button = qobject_cast<ColorButton *>(widget);
  Q_ASSERT(button);
  QColor color = QColorDialog::getColor(button->color(), this);
  if(color.isValid()) {
    button->setColor(color);
  }
  widgetHasChanged();
}

void ColorSettingsPage::chatviewPreview() {
  //TODO: update chatviewPreview
}

void ColorSettingsPage::bufferviewPreview() {
  ui.bufferviewPreview->clear();
  ui.bufferviewPreview->setColumnCount(1);
  ui.bufferviewPreview->setHeaderLabels(QStringList("Buffers"));

  QTreeWidgetItem *topLevelItem = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("network")));
  ui.bufferviewPreview->insertTopLevelItem(0, topLevelItem);
  topLevelItem->setForeground(0, QBrush(ui.noActivityFG->color()));
  if(ui.noActivityUseBG->isChecked())
    topLevelItem->setBackground(0, QBrush(ui.noActivityBG->color()));


  QList<QTreeWidgetItem *> items;
  QTreeWidgetItem *statusBuffer = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("Status Buffer")));
  items.append(statusBuffer);
  statusBuffer->setForeground(0, QBrush(ui.noActivityFG->color()));
  if(ui.noActivityUseBG->isChecked())
    statusBuffer->setBackground(0, QBrush(ui.noActivityBG->color()));

  QTreeWidgetItem *inactiveActivity = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#inactive channel")));
  items.append(inactiveActivity);
  inactiveActivity->setForeground(0, QBrush(ui.inactiveActivityFG->color()));
  if(ui.inactiveActivityUseBG->isChecked())
    inactiveActivity->setBackground(0, QBrush(ui.inactiveActivityBG->color()));

  QTreeWidgetItem *noActivity = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#channel with no activity")));
  items.append(noActivity);
  noActivity->setForeground(0, QBrush(ui.noActivityFG->color()));
  if(ui.noActivityUseBG->isChecked())
    noActivity->setBackground(0, QBrush(ui.noActivityBG->color()));

  QTreeWidgetItem *highlightActivity = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#channel with highlight")));
  items.append(highlightActivity);
  highlightActivity->setForeground(0, QBrush(ui.highlightActivityFG->color()));
  if(ui.highlightActivityUseBG->isChecked())
    highlightActivity->setBackground(0, QBrush(ui.highlightActivityBG->color()));

  QTreeWidgetItem *newMessageActivity = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#channel with new message")));
  items.append(newMessageActivity);
  newMessageActivity->setForeground(0, QBrush(ui.newMessageActivityFG->color()));
  if(ui.newMessageActivityUseBG->isChecked())
    newMessageActivity->setBackground(0, QBrush(ui.newMessageActivityBG->color()));

  QTreeWidgetItem *otherActivity = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#channel with other activity")));
  items.append(otherActivity);
  otherActivity->setForeground(0, QBrush(ui.otherActivityFG->color()));
  if(ui.otherActivityUseBG->isChecked())
    otherActivity->setBackground(0, QBrush(ui.otherActivityBG->color()));

  topLevelItem->insertChildren(0, items);
  ui.bufferviewPreview->expandItem(topLevelItem);
}

