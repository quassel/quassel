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

#include "colorsettingspage.h"

#include "qtui.h"
#include "qtuisettings.h"
#include "qtuistyle.h"
#include "colorbutton.h"

#include <QColorDialog>
#include <QPainter>

ColorSettingsPage::ColorSettingsPage(QWidget *parent)
  : SettingsPage(tr("Appearance"), tr("Color settings"), parent),
    mapper(new QSignalMapper(this))
{
  ui.setupUi(this);

  foreach(ColorButton *button, findChildren<ColorButton *>()) {
    connect(button, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(button, button);
  }
  foreach(QCheckBox *checkBox, findChildren<QCheckBox *>()) {
    connect(checkBox, SIGNAL(clicked()), this, SLOT(widgetHasChanged()));
  }

  connect(mapper, SIGNAL(mapped(QWidget *)), this, SLOT(chooseColor(QWidget *)));

  //disable unused buttons:
  foreach(QWidget *widget, findChildren<QWidget *>()) {
    if(widget->property("NotInUse").toBool()) {
      widget->setEnabled(false);
      widget->hide();
    }
  }
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

  widgetHasChanged();
  bufferviewPreview();
  chatviewPreview();
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
  ui.senderAutoColor->setChecked(true);  
  ui.newMsgMarkerFG->setColor(Qt::red);

  /*
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
  */
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
}

void ColorSettingsPage::load() {
  QtUiStyleSettings s("Colors");
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

  ui.actionMessageBG->setColor(QtUi::style()->format(UiStyle::ActionMsg).background().color());
  ui.errorMessageBG->setColor(QtUi::style()->format(UiStyle::ErrorMsg).background().color());
  ui.joinMessageBG->setColor(QtUi::style()->format(UiStyle::JoinMsg).background().color());
  ui.kickMessageBG->setColor(QtUi::style()->format(UiStyle::KickMsg).background().color());
  ui.modeMessageBG->setColor(QtUi::style()->format(UiStyle::ModeMsg).background().color());
  ui.noticeMessageBG->setColor(QtUi::style()->format(UiStyle::NoticeMsg).background().color());
  ui.partMessageBG->setColor(QtUi::style()->format(UiStyle::PartMsg).background().color());
  ui.plainMessageBG->setColor(QtUi::style()->format(UiStyle::PlainMsg).background().color());
  ui.quitMessageBG->setColor(QtUi::style()->format(UiStyle::QuitMsg).background().color());
  ui.renameMessageBG->setColor(QtUi::style()->format(UiStyle::RenameMsg).background().color());
  ui.serverMessageBG->setColor(QtUi::style()->format(UiStyle::ServerMsg).background().color());

  // FIXME set to false if appropriate
  settings["ActionMessageUseBG"] = s.value("actionMessageUseBG", QVariant(false));
  if(settings["ActionMessageUseBG"].toBool()) {
    ui.actionMessageUseBG->setChecked(true);
    ui.actionMessageBG->setEnabled(true);
  }
  settings["ErrorMessageUseBG"] = s.value("errorMessageUseBG", QVariant(false));
  if(settings["ErrorMessageUseBG"].toBool()) {
    ui.errorMessageUseBG->setChecked(true);
    ui.errorMessageBG->setEnabled(true);
  }
  settings["JoinMessageUseBG"] = s.value("joinMessageUseBG", QVariant(false));
  if(settings["JoinMessageUseBG"].toBool()) {
    ui.joinMessageUseBG->setChecked(true);
    ui.joinMessageBG->setEnabled(true);
  }
  settings["KickMessageUseBG"] = s.value("kickMessageUseBG", QVariant(false));
  if(settings["KickMessageUseBG"].toBool()) {
    ui.kickMessageUseBG->setChecked(true);
    ui.kickMessageBG->setEnabled(true);
  }
  settings["ModeMessageUseBG"] = s.value("modeMessageUseBG", QVariant(false));
  if(settings["ModeMessageUseBG"].toBool()) {
    ui.modeMessageUseBG->setChecked(true);
    ui.modeMessageBG->setEnabled(true);
  }
  settings["NoticeMessageUseBG"] = s.value("noticeMessageUseBG", QVariant(false));
  if(settings["NoticeMessageUseBG"].toBool()) {
    ui.noticeMessageUseBG->setChecked(true);
    ui.noticeMessageBG->setEnabled(true);
  }
  settings["PartMessageUseBG"] = s.value("partMessageUseBG", QVariant(false));
  if(settings["PartMessageUseBG"].toBool()) {
    ui.partMessageUseBG->setChecked(true);
    ui.partMessageBG->setEnabled(true);
  }
  settings["PlainMessageUseBG"] = s.value("plainMessageUseBG", QVariant(false));
  if(settings["PlainMessageUseBG"].toBool()) {
    ui.plainMessageUseBG->setChecked(true);
    ui.plainMessageBG->setEnabled(true);
  }
  settings["QuitMessageUseBG"] = s.value("quitMessageUseBG", QVariant(false));
  if(settings["QuitMessageUseBG"].toBool()) {
    ui.quitMessageUseBG->setChecked(true);
    ui.quitMessageBG->setEnabled(true);
  }
  settings["RenameMessageUseBG"] = s.value("renameMessageUseBG", QVariant(false));
  if(settings["RenameMessageUseBG"].toBool()) {
    ui.renameMessageUseBG->setChecked(true);
    ui.renameMessageBG->setEnabled(true);
  }
  settings["ServerMessageUseBG"] = s.value("serverMessageUseBG", QVariant(false));
  if(settings["ServerMessageUseBG"].toBool()) {
    ui.serverMessageUseBG->setChecked(true);
    ui.serverMessageBG->setEnabled(true);
  }

  ui.timestampFG->setColor(QtUi::style()->format(UiStyle::Timestamp).foreground().color());
  ui.timestampBG->setColor(QtUi::style()->format(UiStyle::Timestamp).background().color());
  ui.senderFG->setColor(QtUi::style()->format(UiStyle::Sender).foreground().color());
  ui.senderBG->setColor(QtUi::style()->format(UiStyle::Sender).background().color());
  settings["SenderAutoColor"] = s.value("senderAutoColor", QVariant(true));
  if (settings["SenderAutoColor"].toBool()) {
    ui.senderAutoColor->setChecked(true);
    ui.senderFrame->setEnabled(false);
  }
  settings["NewMsgMarkerFG"] = s.value("newMsgMarkerFG", QColor(Qt::red));
  ui.newMsgMarkerFG->setColor(settings["NewMsgMarkerFG"].value<QColor>());

  settings["TimestampUseBG"] = s.value("timestampUseBG", QVariant(false));
  if(settings["TimestampUseBG"].toBool()) {
    ui.timestampUseBG->setChecked(true);
    ui.timestampBG->setEnabled(true);
  }
  settings["SenderUseBG"] = s.value("senderUseBG", QVariant(false));
  if(settings["SenderUseBG"].toBool()) {
    ui.senderUseBG->setChecked(true);
    ui.senderBG ->setEnabled(true);
  }

  ui.nickFG->setColor(QtUi::style()->format(UiStyle::Nick).foreground().color());
  ui.nickBG->setColor(QtUi::style()->format(UiStyle::Nick).background().color());
  ui.hostmaskFG->setColor(QtUi::style()->format(UiStyle::Hostmask).foreground().color());
  ui.hostmaskBG->setColor(QtUi::style()->format(UiStyle::Hostmask).background().color());
  ui.channelnameFG->setColor(QtUi::style()->format(UiStyle::ChannelName).foreground().color());
  ui.channelnameBG->setColor(QtUi::style()->format(UiStyle::ChannelName).background().color());
  ui.modeFlagsFG->setColor(QtUi::style()->format(UiStyle::ModeFlags).foreground().color());
  ui.modeFlagsBG->setColor(QtUi::style()->format(UiStyle::ModeFlags).background().color());
  ui.urlFG->setColor(QtUi::style()->format(UiStyle::Url).foreground().color());
  ui.urlBG->setColor(QtUi::style()->format(UiStyle::Url).background().color());

  ui.highlightColor->setColor(QtUi::style()->highlightColor());

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
  QtUiStyleSettings s("Colors");
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

  saveColor(UiStyle::ErrorMsg, ui.errorMessageFG->color(), ui.errorMessageBG->color(), ui.errorMessageUseBG->isChecked());
  s.setValue("errorMessageUseBG", ui.errorMessageUseBG->isChecked());
  saveColor(UiStyle::NoticeMsg, ui.noticeMessageFG->color(), ui.noticeMessageBG->color(), ui.noticeMessageUseBG->isChecked());
  s.setValue("noticeMessageUseBG", ui.noticeMessageUseBG->isChecked());
  saveColor(UiStyle::PlainMsg, ui.plainMessageFG->color(), ui.plainMessageBG->color(), ui.plainMessageUseBG->isChecked());
  s.setValue("plainMessageUseBG", ui.plainMessageUseBG->isChecked());
  saveColor(UiStyle::ServerMsg, ui.serverMessageFG->color(), ui.serverMessageBG->color(), ui.serverMessageUseBG->isChecked());
  s.setValue("serverMessageUseBG", ui.serverMessageUseBG->isChecked());
  saveColor(UiStyle::ActionMsg, ui.actionMessageFG->color(), ui.actionMessageBG->color(), ui.actionMessageUseBG->isChecked());
  s.setValue("actionMessageUseBG", ui.actionMessageUseBG->isChecked());
  saveColor(UiStyle::JoinMsg, ui.joinMessageFG->color(), ui.joinMessageBG->color(), ui.joinMessageUseBG->isChecked());
  s.setValue("joinMessageUseBG", ui.joinMessageUseBG->isChecked());
  saveColor(UiStyle::KickMsg, ui.kickMessageFG->color(), ui.kickMessageBG->color(), ui.kickMessageUseBG->isChecked());
  s.setValue("kickMessageUseBG", ui.kickMessageUseBG->isChecked());
  saveColor(UiStyle::ModeMsg, ui.modeMessageFG->color(), ui.modeMessageBG->color(), ui.modeMessageUseBG->isChecked());
  s.setValue("modeMessageUseBG", ui.modeMessageUseBG->isChecked());
  saveColor(UiStyle::NoticeMsg, ui.noticeMessageFG->color(), ui.noticeMessageBG->color(), ui.noticeMessageUseBG->isChecked());
  s.setValue("noticeMessageUseBG", ui.noticeMessageUseBG->isChecked());
  saveColor(UiStyle::PartMsg, ui.partMessageFG->color(), ui.partMessageBG->color(), ui.partMessageUseBG->isChecked());
  s.setValue("partMessageUseBG", ui.partMessageUseBG->isChecked());
  saveColor(UiStyle::QuitMsg, ui.quitMessageFG->color(), ui.quitMessageBG->color(), ui.quitMessageUseBG->isChecked());
  s.setValue("quitMessageUseBG", ui.quitMessageUseBG->isChecked());
  saveColor(UiStyle::RenameMsg, ui.renameMessageFG->color(), ui.renameMessageBG->color(), ui.renameMessageUseBG->isChecked());
  s.setValue("renameMessageUseBG", ui.renameMessageUseBG->isChecked());

  QtUi::style()->setHighlightColor(ui.highlightColor->color());

  saveColor(UiStyle::Timestamp, ui.timestampFG->color(), ui.timestampBG->color(), ui.timestampUseBG->isChecked());
  s.setValue("timestampUseBG", ui.timestampUseBG->isChecked());
  saveColor(UiStyle::Sender, ui.senderFG->color(), ui.senderBG->color(), ui.senderUseBG->isChecked());
  s.setValue("senderUseBG", ui.senderUseBG->isChecked());
  s.setValue("senderAutoColor", ui.senderAutoColor->isChecked());
  QtUi::style()->setSenderAutoColor(ui.senderAutoColor->isChecked());
  s.setValue("newMsgMarkerFG", ui.newMsgMarkerFG->color());

  /*
  saveColor(UiStyle::Nick, ui.nickFG->color(), ui.nickBG->color(), ui.nickUseBG->isChecked());
  s.setValue("nickUseBG", ui.nickUseBG->isChecked());
  saveColor(UiStyle::Hostmask, ui.hostmaskFG->color(), ui.hostmaskBG->color(), ui.hostmaskUseBG->isChecked());
  s.setValue("hostmaskUseBG", ui.hostmaskUseBG->isChecked());
  saveColor(UiStyle::ChannelName, ui.channelnameFG->color(), ui.channelnameBG->color(), ui.channelnameUseBG->isChecked());
  s.setValue("channelnameUseBG", ui.channelnameUseBG->isChecked());
  saveColor(UiStyle::ModeFlags, ui.modeFlagsFG->color(), ui.modeFlagsBG->color(), ui.modeFlagsUseBG->isChecked());
  s.setValue("modeFlagsUseBG", ui.modeFlagsUseBG->isChecked());
  saveColor(UiStyle::Url, ui.urlFG->color(), ui.urlBG->color(), ui.urlUseBG->isChecked());
  s.setValue("urlUseBG", ui.urlUseBG->isChecked());
  */

  saveMircColor(0, ui.color0->color());
  saveMircColor(1, ui.color1->color());
  saveMircColor(2, ui.color2->color());
  saveMircColor(3, ui.color3->color());
  saveMircColor(4, ui.color4->color());
  saveMircColor(5, ui.color5->color());
  saveMircColor(6, ui.color6->color());
  saveMircColor(7, ui.color7->color());
  saveMircColor(8, ui.color8->color());
  saveMircColor(9, ui.color9->color());
  saveMircColor(10, ui.color10->color());
  saveMircColor(11, ui.color11->color());
  saveMircColor(12, ui.color12->color());
  saveMircColor(13, ui.color13->color());
  saveMircColor(14, ui.color14->color());
  saveMircColor(15, ui.color15->color());

  s.setValue("onlineStatusFG", ui.onlineStatusFG->color());
  s.setValue("onlineStatusBG", ui.onlineStatusBG->color());
  s.setValue("onlineStatusUseBG", ui.onlineStatusUseBG->isChecked());
  s.setValue("awayStatusFG", ui.awayStatusFG->color());
  s.setValue("awayStatusBG", ui.awayStatusBG->color());
  s.setValue("awayStatusUseBG", ui.awayStatusUseBG->isChecked());

  load(); //TODO: remove when settings hash map is unnescessary
  setChangedState(false);
}

void ColorSettingsPage::saveColor(UiStyle::FormatType formatType, const QColor &foreground, const QColor &background, bool enableBG) {
  QTextCharFormat format = QtUi::style()->format(formatType);
  format.setForeground(QBrush(foreground));
  if(enableBG)
    format.setBackground(QBrush(background));
  else
    format.clearBackground();
  QtUi::style()->setFormat(formatType, format, Settings::Custom);
}

void ColorSettingsPage::saveMircColor(int num, const QColor &col) {
  QTextCharFormat fgf, bgf;
  fgf.setForeground(QBrush(col)); QtUi::style()->setFormat((UiStyle::FormatType)(UiStyle::FgCol00 | num<<24), fgf, Settings::Custom);
  bgf.setBackground(QBrush(col)); QtUi::style()->setFormat((UiStyle::FormatType)(UiStyle::BgCol00 | num<<28), bgf, Settings::Custom);
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
  if(QtUi::style()->format(UiStyle::ErrorMsg).background().color() != ui.errorMessageBG->color()) return true;
  if(settings["ErrorMessageUseBG"].toBool() != ui.errorMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::ErrorMsg).foreground().color() != ui.errorMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::ErrorMsg).background().color() != ui.errorMessageBG->color()) return true;
  if(settings["NoticeMessageUseBG"].toBool() != ui.noticeMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::PlainMsg).foreground().color() != ui.plainMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::PlainMsg).background().color() != ui.plainMessageBG->color()) return true;
  if(settings["PlainMessageUseBG"].toBool() != ui.plainMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::ServerMsg).foreground().color() != ui.serverMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::ServerMsg).background().color() != ui.serverMessageBG->color()) return true;
  if(settings["ServerMessageUseBG"].toBool() != ui.serverMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::ActionMsg).foreground().color() != ui.actionMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::ActionMsg).background().color() != ui.actionMessageBG->color()) return true;
  if(settings["ActionMessageUseBG"].toBool() != ui.actionMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::JoinMsg).foreground().color() != ui.joinMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::JoinMsg).background().color() != ui.joinMessageBG->color()) return true;
  if(settings["JoinMessageUseBG"].toBool() != ui.joinMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::KickMsg).foreground().color() != ui.kickMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::KickMsg).background().color() != ui.joinMessageBG->color()) return true;
  if(settings["KickMessageUseBG"].toBool() != ui.kickMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::ModeMsg).foreground().color() != ui.modeMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::ModeMsg).background().color() != ui.modeMessageBG->color()) return true;
  if(settings["ModeMessageUseBG"].toBool() != ui.modeMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::NoticeMsg).foreground().color() != ui.noticeMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::NoticeMsg).background().color() != ui.noticeMessageBG->color()) return true;
  if(settings["NoticeMessageUseBG"].toBool() != ui.noticeMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::PartMsg).foreground().color() != ui.partMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::PartMsg).background().color() != ui.partMessageBG->color()) return true;
  if(settings["PartMessageUseBG"].toBool() != ui.partMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::QuitMsg).foreground().color() != ui.quitMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::QuitMsg).background().color() != ui.quitMessageBG->color()) return true;
  if(settings["QuitMessageUseBG"].toBool() != ui.quitMessageUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::RenameMsg).foreground().color() != ui.renameMessageFG->color()) return true;
  if(QtUi::style()->format(UiStyle::RenameMsg).background().color() != ui.renameMessageBG->color()) return true;
  if(settings["RenameMessageUseBG"].toBool() != ui.renameMessageUseBG->isChecked()) return true;

  if(QtUi::style()->highlightColor() != ui.highlightColor->color()) return true;

  if(QtUi::style()->format(UiStyle::Timestamp).foreground().color() != ui.timestampFG->color()) return true;
  if(QtUi::style()->format(UiStyle::Timestamp).background().color() != ui.timestampBG->color()) return true;
  if(settings["TimestampUseBG"].toBool() != ui.timestampUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::Sender).foreground().color() != ui.senderFG->color()) return true;
  if(QtUi::style()->format(UiStyle::Sender).background().color() != ui.senderBG->color()) return true;
  if(settings["SenderUseBG"].toBool() != ui.senderUseBG->isChecked()) return true;
  if(settings["SenderAutoColor"].toBool() != ui.senderAutoColor->isChecked()) return true;
  if(settings["NewMsgMarkerFG"].value<QColor>() != ui.newMsgMarkerFG->color()) return true;

  /*
  if(QtUi::style()->format(UiStyle::Nick).foreground().color() != ui.nickFG->color()) return true;
  if(QtUi::style()->format(UiStyle::Nick).background().color() != ui.nickBG->color()) return true;
  if(settings["nickUseBG"].toBool() != ui.nickUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::Hostmask).foreground().color() != ui.hostmaskFG->color()) return true;
  if(QtUi::style()->format(UiStyle::Hostmask).background().color() != ui.hostmaskBG->color()) return true;
  if(settings["hostmaskUseBG"].toBool() != ui.hostmaskUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::ChannelName).foreground().color() != ui.channelnameFG->color()) return true;
  if(QtUi::style()->format(UiStyle::ChannelName).background().color() != ui.channelnameBG->color()) return true;
  if(settings["channelnameUseBG"].toBool() != ui.channelnameUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::ModeFlags).foreground().color() != ui.modeFlagsFG->color()) return true;
  if(QtUi::style()->format(UiStyle::ModeFlags).background().color() != ui.modeFlagsBG->color()) return true;
  if(settings["modeFlagsUseBG"].toBool() != ui.modeFlagsUseBG->isChecked()) return true;
  if(QtUi::style()->format(UiStyle::Url).foreground().color() != ui.urlFG->color()) return true;
  if(QtUi::style()->format(UiStyle::Url).background().color() != ui.urlBG->color()) return true;
  if(settings["urlUseBG"].toBool() != ui.urlUseBG->isChecked()) return true;
  */

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

