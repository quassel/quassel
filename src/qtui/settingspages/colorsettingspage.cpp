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

ColorSettingsPage::ColorSettingsPage(QWidget *parent)
  : SettingsPage(tr("Appearance"), tr("Color settings"), parent) {
  ui.setupUi(this);

  mapper = new QSignalMapper(this);
  //Bufferview tab:
  connect(ui.inactiveActivity, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.noActivity, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.highlightActivity, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.newMessageActivity, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.otherActivity, SIGNAL(clicked()), mapper, SLOT(map()));

  mapper->setMapping(ui.inactiveActivity, ui.inactiveActivity);
  mapper->setMapping(ui.noActivity, ui.noActivity);
  mapper->setMapping(ui.highlightActivity, ui.highlightActivity);
  mapper->setMapping(ui.newMessageActivity, ui.newMessageActivity);
  mapper->setMapping(ui.otherActivity, ui.otherActivity);

  //Chatview tab:
  connect(ui.errorMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.errorMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.noticeMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.noticeMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.plainMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.plainMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.serverMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.serverMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.actionMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.actionMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.joinMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.joinMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.kickMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.kickMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.modeMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.modeMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.partMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.partMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.quitMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.quitMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.renameMessageFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.renameMessageBG, SIGNAL(clicked()), mapper, SLOT(map()));

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

  //Message Layout tab:
  connect(ui.timestampFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.timestampBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.senderFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.senderBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.nickFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.nickBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.hostmaskFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.hostmaskBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.channelnameFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.channelnameBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.modeFlagsFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.modeFlagsBG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.urlFG, SIGNAL(clicked()), mapper, SLOT(map()));
  connect(ui.urlBG, SIGNAL(clicked()), mapper, SLOT(map()));

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

  connect(mapper, SIGNAL(mapped(QWidget *)), this, SLOT(chooseColor(QWidget *)));

  //disable unused buttons:
  ui.errorMessageBG->setEnabled(false);
  ui.noticeMessageBG->setEnabled(false);
  ui.plainMessageBG->setEnabled(false);
  ui.serverMessageBG->setEnabled(false);
  ui.actionMessageBG->setEnabled(false);
  ui.joinMessageBG->setEnabled(false);
  ui.kickMessageBG->setEnabled(false);
  ui.modeMessageBG->setEnabled(false);
  ui.partMessageBG->setEnabled(false);
  ui.quitMessageBG->setEnabled(false);
  ui.renameMessageBG->setEnabled(false);

  ui.timestampBG->setEnabled(false);
  ui.senderBG->setEnabled(false);
  ui.nickFG->setEnabled(false);
  ui.nickBG->setEnabled(false);
  ui.hostmaskFG->setEnabled(false);
  ui.hostmaskBG->setEnabled(false);
  ui.channelnameFG->setEnabled(false);
  ui.channelnameBG->setEnabled(false);
  ui.modeFlagsFG->setEnabled(false);
  ui.modeFlagsBG->setEnabled(false);
  ui.urlFG->setEnabled(false);
  ui.urlBG->setEnabled(false);
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
}

void ColorSettingsPage::defaultBufferview() {
  ui.inactiveActivity->setColor(QColor(Qt::gray));
  ui.noActivity->setColor(QColor(Qt::black));
  ui.highlightActivity->setColor(QColor(Qt::magenta));
  ui.newMessageActivity->setColor(QColor(Qt::green));
  ui.otherActivity->setColor(QColor(Qt::darkGreen));

  widgetHasChanged();
  bufferviewPreview();
}

void ColorSettingsPage::defaultServerActivity() {
  ui.errorMessageFG->setColor(QtUi::style()->format(UiStyle::ErrorMsg, Settings::Default).foreground().color());
  ui.errorMessageBG->setColor(QColor("white"));
  ui.noticeMessageFG->setColor(QtUi::style()->format(UiStyle::NoticeMsg, Settings::Default).foreground().color());
  ui.noticeMessageBG->setColor(QColor("white"));
  ui.plainMessageFG->setColor(QtUi::style()->format(UiStyle::PlainMsg, Settings::Default).foreground().color());
  ui.plainMessageBG->setColor(QColor("white"));
  ui.serverMessageFG->setColor(QtUi::style()->format(UiStyle::ServerMsg, Settings::Default).foreground().color());
  ui.serverMessageBG->setColor(QColor("white"));

  widgetHasChanged();
  chatviewPreview();
}

void ColorSettingsPage::defaultUserActivity() {
  ui.actionMessageFG->setColor(QtUi::style()->format(UiStyle::ActionMsg, Settings::Default).foreground().color());
  ui.actionMessageBG->setColor(QColor("white"));
  ui.joinMessageFG->setColor(QtUi::style()->format(UiStyle::JoinMsg, Settings::Default).foreground().color());
  ui.joinMessageBG->setColor(QColor("white"));
  ui.kickMessageFG->setColor(QtUi::style()->format(UiStyle::KickMsg, Settings::Default).foreground().color());
  ui.kickMessageBG->setColor(QColor("white"));
  ui.modeMessageFG->setColor(QtUi::style()->format(UiStyle::ModeMsg, Settings::Default).foreground().color());
  ui.modeMessageBG->setColor(QColor("white"));
  ui.partMessageFG->setColor(QtUi::style()->format(UiStyle::PartMsg, Settings::Default).foreground().color());
  ui.partMessageBG->setColor(QColor("white"));
  ui.quitMessageFG->setColor(QtUi::style()->format(UiStyle::QuitMsg, Settings::Default).foreground().color());
  ui.quitMessageBG->setColor(QColor("white"));
  ui.renameMessageFG->setColor(QtUi::style()->format(UiStyle::RenameMsg, Settings::Default).foreground().color());
  ui.renameMessageBG->setColor(QColor("white"));

  widgetHasChanged();
  chatviewPreview();
}

void ColorSettingsPage::defaultMessage() {
  ui.timestampFG->setColor(QtUi::style()->format(UiStyle::Timestamp, Settings::Default).foreground().color());
  ui.senderFG->setColor(QtUi::style()->format(UiStyle::Sender, Settings::Default).foreground().color());

  ui.timestampBG->setColor(QColor("white"));
  ui.senderBG->setColor(QColor("white"));
  ui.nickFG->setColor(QColor("black"));
  ui.nickBG->setColor(QColor("white"));
  ui.hostmaskFG->setColor(QColor("black"));
  ui.hostmaskBG->setColor(QColor("white"));
  ui.channelnameFG->setColor(QColor("black"));
  ui.channelnameBG->setColor(QColor("white"));
  ui.modeFlagsFG->setColor(QColor("black"));
  ui.modeFlagsBG->setColor(QColor("white"));
  ui.urlFG->setColor(QColor("black"));
  ui.urlBG->setColor(QColor("white"));

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

void ColorSettingsPage::load() {
  QtUiSettings s("QtUi/Colors");
  settings["InactiveActivity"] = s.value("inactiveActivity", QVariant(QColor(Qt::gray)));
  ui.inactiveActivity->setColor(settings["InactiveActivity"].value<QColor>());
  settings["NoActivity"] = s.value("noActivity", QVariant(QColor(Qt::black)));
  ui.noActivity->setColor(settings["NoActivity"].value<QColor>());
  settings["HighlightActivity"] = s.value("highlightActivity", QVariant(QColor(Qt::magenta)));
  ui.highlightActivity->setColor(settings["HighlightActivity"].value<QColor>());
  settings["NewMessageActivity"] = s.value("newMessageActivity", QVariant(QColor(Qt::green)));
  ui.newMessageActivity->setColor(settings["NewMessageActivity"].value<QColor>());
  settings["OtherActivity"] = s.value("otherActivity", QVariant(QColor(Qt::darkGreen)));
  ui.otherActivity->setColor(settings["OtherActivity"].value<QColor>());

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
  settings["ErrorMessageBG"] = s.value("errorMessageBG", QVariant(QColor("white")));
  ui.errorMessageBG->setColor(settings["ErrorMessageBG"].value<QColor>());
  settings["JoinMessageBG"] = s.value("joinMessageBG", QVariant(QColor("white")));
  ui.joinMessageBG->setColor(settings["JoinMessageBG"].value<QColor>());
  settings["KickMessageBG"] = s.value("kickMessageBG", QVariant(QColor("white")));
  ui.kickMessageBG->setColor(settings["KickMessageBG"].value<QColor>());
  settings["ModeMessageBG"] = s.value("modeMessageBG", QVariant(QColor("white")));
  ui.modeMessageBG->setColor(settings["ModeMessageBG"].value<QColor>());
  settings["NoticeMessageBG"] = s.value("noticeMessageBG", QVariant(QColor("white")));
  ui.noticeMessageBG->setColor(settings["NoticeMessageBG"].value<QColor>());
  settings["PartMessageBG"] = s.value("partMessageBG", QVariant(QColor("white")));
  ui.partMessageBG->setColor(settings["PartMessageBG"].value<QColor>());
  settings["PlainMessageBG"] = s.value("plainMessageBG", QVariant(QColor("white")));
  ui.plainMessageBG->setColor(settings["PlainMessageBG"].value<QColor>());
  settings["QuitMessageBG"] = s.value("quitMessageBG", QVariant(QColor("white")));
  ui.quitMessageBG->setColor(settings["QuitMessageBG"].value<QColor>());
  settings["RenameMessageBG"] = s.value("renameMessageBG", QVariant(QColor("white")));
  ui.renameMessageBG->setColor(settings["RenameMessageBG"].value<QColor>());
  settings["ServerMessageBG"] = s.value("serverMessageBG", QVariant(QColor("white")));
  ui.serverMessageBG->setColor(settings["ServerMessageBG"].value<QColor>());

  ui.timestampFG->setColor(QtUi::style()->format(UiStyle::Timestamp).foreground().color());
  ui.senderFG->setColor(QtUi::style()->format(UiStyle::Sender).foreground().color());

  settings["TimestampBG"] = s.value("timestampBG", QVariant(QColor("white")));
  ui.timestampBG->setColor(settings["TimestampBG"].value<QColor>());
  settings["SenderBG"] = s.value("senderBG", QVariant(QColor("white")));
  ui.senderBG->setColor(settings["SenderBG"].value<QColor>());
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

  setChangedState(false);

  bufferviewPreview();
  chatviewPreview();
}

void ColorSettingsPage::save() {
  QtUiSettings s("QtUi/Colors");
  s.setValue("inactiveActivity", ui.inactiveActivity->color());
  s.setValue("noActivity", ui.noActivity->color());
  s.setValue("highlightActivity", ui.highlightActivity->color());
  s.setValue("newMessageActivity", ui.newMessageActivity->color());
  s.setValue("otherActivity", ui.otherActivity->color());

  saveColor(UiStyle::ErrorMsg, ui.errorMessageFG->color());
  s.setValue("errorMessageBG", ui.errorMessageBG->color());
  saveColor(UiStyle::NoticeMsg, ui.noticeMessageFG->color());
  s.setValue("noticeMessageBG", ui.noticeMessageBG->color());
  saveColor(UiStyle::PlainMsg, ui.plainMessageFG->color());
  s.setValue("plainMessageBG", ui.plainMessageBG->color());
  saveColor(UiStyle::ServerMsg, ui.serverMessageFG->color());
  s.setValue("serverMessageBG", ui.serverMessageBG->color());
  saveColor(UiStyle::ActionMsg, ui.actionMessageFG->color());
  s.setValue("actionMessageBG", ui.actionMessageBG->color());
  saveColor(UiStyle::JoinMsg, ui.joinMessageFG->color());
  s.setValue("joinMessageBG", ui.joinMessageBG->color());
  saveColor(UiStyle::KickMsg, ui.kickMessageFG->color());
  s.setValue("kickMessageBG", ui.kickMessageBG->color());
  saveColor(UiStyle::ModeMsg, ui.modeMessageFG->color());
  s.setValue("modeMessageBG", ui.modeMessageBG->color());
  saveColor(UiStyle::PartMsg, ui.partMessageFG->color());
  s.setValue("partMessageBG", ui.partMessageBG->color());
  saveColor(UiStyle::QuitMsg, ui.quitMessageFG->color());
  s.setValue("quitMessageBG", ui.quitMessageBG->color());
  saveColor(UiStyle::RenameMsg, ui.renameMessageFG->color());
  s.setValue("renameMessageBG", ui.renameMessageBG->color());

  saveColor(UiStyle::Timestamp, ui.timestampFG->color());
  saveColor(UiStyle::Sender, ui.senderFG->color());

  s.setValue("timestampBG", ui.timestampBG->color());
  s.setValue("senderBG", ui.senderBG->color());
  s.setValue("nickFG", ui.nickFG->color());
  s.setValue("nickBG", ui.nickBG->color());
  s.setValue("hostmaskFG", ui.hostmaskFG->color());
  s.setValue("hostmaskBG", ui.hostmaskBG->color());
  s.setValue("channelnameFG", ui.channelnameFG->color());
  s.setValue("channelnameBG", ui.channelnameBG->color());
  s.setValue("modeFlagsFG", ui.modeFlagsFG->color());
  s.setValue("modeFlagsBG", ui.modeFlagsBG->color());
  s.setValue("urlFG", ui.urlFG->color());
  s.setValue("urlBG", ui.urlBG->color());

  saveColor(UiStyle::FgCol00, ui.color0->color());
  saveColor(UiStyle::FgCol01, ui.color1->color());
  saveColor(UiStyle::FgCol02, ui.color2->color());
  saveColor(UiStyle::FgCol03, ui.color3->color());
  saveColor(UiStyle::FgCol04, ui.color4->color());
  saveColor(UiStyle::FgCol05, ui.color5->color());
  saveColor(UiStyle::FgCol06, ui.color6->color());
  saveColor(UiStyle::FgCol07, ui.color7->color());
  saveColor(UiStyle::FgCol08, ui.color8->color());
  saveColor(UiStyle::FgCol09, ui.color9->color());
  saveColor(UiStyle::FgCol10, ui.color10->color());
  saveColor(UiStyle::FgCol11, ui.color11->color());
  saveColor(UiStyle::FgCol12, ui.color12->color());
  saveColor(UiStyle::FgCol13, ui.color13->color());
  saveColor(UiStyle::FgCol14, ui.color14->color());
  saveColor(UiStyle::FgCol15, ui.color15->color());

  saveColor(UiStyle::BgCol00, ui.color0->color());
  saveColor(UiStyle::BgCol01, ui.color1->color());
  saveColor(UiStyle::BgCol02, ui.color2->color());
  saveColor(UiStyle::BgCol03, ui.color3->color());
  saveColor(UiStyle::BgCol04, ui.color4->color());
  saveColor(UiStyle::BgCol05, ui.color5->color());
  saveColor(UiStyle::BgCol06, ui.color6->color());
  saveColor(UiStyle::BgCol07, ui.color7->color());
  saveColor(UiStyle::BgCol08, ui.color8->color());
  saveColor(UiStyle::BgCol09, ui.color9->color());
  saveColor(UiStyle::BgCol10, ui.color10->color());
  saveColor(UiStyle::BgCol11, ui.color11->color());
  saveColor(UiStyle::BgCol12, ui.color12->color());
  saveColor(UiStyle::BgCol13, ui.color13->color());
  saveColor(UiStyle::BgCol14, ui.color14->color());
  saveColor(UiStyle::BgCol15, ui.color15->color());

  load(); //TODO: remove when settings hash map is unnescessary
  setChangedState(false);
}

void ColorSettingsPage::saveColor(UiStyle::FormatType formatType, const QColor &color) {
  QTextCharFormat format = QtUi::style()->format(formatType);
  format.setForeground(QBrush(color));
  QtUi::style()->setFormat(formatType, format, Settings::Custom);
}

void ColorSettingsPage::widgetHasChanged() {
  bool changed = testHasChanged();
  if(changed != hasChanged()) setChangedState(changed);
}

bool ColorSettingsPage::testHasChanged() {
  if(settings["InactiveActivity"].value<QColor>() != ui.inactiveActivity->color()) return true;
  if(settings["NoActivity"].value<QColor>() != ui.noActivity->color()) return true;
  if(settings["HighlightActivity"].value<QColor>() != ui.highlightActivity->color()) return true;
  if(settings["NewMessageActivity"].value<QColor>() != ui.newMessageActivity->color()) return true;
  if(settings["OtherActivity"].value<QColor>() != ui.otherActivity->color()) return true;

  if(QtUi::style()->format(UiStyle::ErrorMsg).foreground().color() != ui.errorMessageFG->color()) return true;
  if(settings["ErrorMessageBG"].value<QColor>() != ui.errorMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::NoticeMsg).foreground().color() != ui.noticeMessageFG->color()) return true;
  if(settings["NoticeMessageBG"].value<QColor>() != ui.noticeMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::PlainMsg).foreground().color() != ui.plainMessageFG->color()) return true;
  if(settings["PlainMessageBG"].value<QColor>() != ui.plainMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::ServerMsg).foreground().color() != ui.serverMessageFG->color()) return true;
  if(settings["ServerMessageBG"].value<QColor>() != ui.serverMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::ActionMsg).foreground().color() != ui.actionMessageFG->color()) return true;
  if(settings["ActionMessageBG"].value<QColor>() != ui.actionMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::JoinMsg).foreground().color() != ui.joinMessageFG->color()) return true;
  if(settings["JoinMessageBG"].value<QColor>() != ui.joinMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::KickMsg).foreground().color() != ui.kickMessageFG->color()) return true;
  if(settings["KickMessageBG"].value<QColor>() != ui.kickMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::ModeMsg).foreground().color() != ui.modeMessageFG->color()) return true;
  if(settings["ModeMessageBG"].value<QColor>() != ui.modeMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::PartMsg).foreground().color() != ui.partMessageFG->color()) return true;
  if(settings["PartMessageBG"].value<QColor>() != ui.partMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::QuitMsg).foreground().color() != ui.quitMessageFG->color()) return true;
  if(settings["QuitMessageBG"].value<QColor>() != ui.quitMessageBG->color()) return true;
  if(QtUi::style()->format(UiStyle::RenameMsg).foreground().color() != ui.renameMessageFG->color()) return true;
  if(settings["RenameMessageBG"].value<QColor>() != ui.renameMessageBG->color()) return true;

  if(QtUi::style()->format(UiStyle::Timestamp).foreground().color() != ui.timestampFG->color()) return true;
  if(QtUi::style()->format(UiStyle::Sender).foreground().color() != ui.senderFG->color()) return true;

  if(settings["TimestampBG"].value<QColor>() != ui.timestampBG->color()) return true;
  if(settings["SenderBG"].value<QColor>() != ui.senderBG->color()) return true;
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
  bufferviewPreview();
  chatviewPreview();
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

  QList<QTreeWidgetItem *> items;
  items.append(new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("Status Buffer"))));

  QTreeWidgetItem *inactive = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#inactive channel")));
  items.append(inactive);
  inactive->setForeground(0, QBrush(ui.inactiveActivity->color()));

  QTreeWidgetItem *noActivity = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#channel with no activity")));
  items.append(noActivity);
  noActivity->setForeground(0, QBrush(ui.noActivity->color()));

  QTreeWidgetItem *highlightActivity = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#channel with highlight")));
  items.append(highlightActivity);
  highlightActivity->setForeground(0, QBrush(ui.highlightActivity->color()));

  QTreeWidgetItem *newMessageActivity = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#channel with new message")));
  items.append(newMessageActivity);
  newMessageActivity->setForeground(0, QBrush(ui.newMessageActivity->color()));

  QTreeWidgetItem *otherActivity = new QTreeWidgetItem((QTreeWidget*)0, QStringList(QString("#channel with other activity")));
  items.append(otherActivity);
  otherActivity->setForeground(0, QBrush(ui.otherActivity->color()));

  topLevelItem->insertChildren(0, items);
  ui.bufferviewPreview->expandItem(topLevelItem);
}

