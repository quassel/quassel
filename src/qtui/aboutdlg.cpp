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

#include "aboutdlg.h"
#include "global.h"

AboutDlg::AboutDlg(QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);

  ui.versionLabel->setText(QString("<b>Version %1</b><br>Protcol version: %2<br>Built: %3 %4").arg(Global::quasselVersion)
			   .arg(Global::protocolVersion)
			   .arg(Global::quasselBuildDate).arg(Global::quasselBuildTime));
  ui.aboutTextBrowser->setHtml(about());
  ui.authorTextBrowser->setHtml(authors());
  ui.contributorTextBrowser->setHtml(contributors());
  ui.thanksToTextBrowser->setHtml(thanksTo());

}

QString AboutDlg::about() const {
  QString res;
  res = tr("<b>A modern, distributed IRC Client</b><br><br>"
           "&copy;2005-2008 by the Quassel Project<br>"
           "<a href=\"http://quassel-irc.org\">http://quassel-irc.org</a><br>"
           "<a href=\"irc://irc.freenode.net/quassel\">#quassel</a> on <a href=\"http://www.freenode.net\">Freenode</a><br><br>"
           "Quassel IRC is dual-licensed under <a href=\"http://www.gnu.org/licenses/gpl-2.0.txt\">GPLv2</a> and "
                "<a href=\"http://www.gnu.org/licenses/gpl-3.0.txt\">GPLv3</a>.<br>"
           "Most icons are &copy; by the <a href=\"http://www.oxygen-icons.org\">Oxygen Team</a> and used under the "
                "<a href=\"http://www.gnu.org/licenses/lgpl.html\">LGPL</a>.<br><br>"
           "Please use <a href=\"http://bugs.quassel-irc.org\">http://bugs.quassel-irc.org</a> to report bugs."
          );

  return res;
}

QString AboutDlg::authors() const {
  QString res;
  res = tr("Quassel IRC is mainly developed by:") +
        "<dl>"
        "<dt><b>Manuel \"Sputnick\" Nickschas</b></dt><dd><a href=\"mailto:sput@quassel-irc.org\">sput@quassel-irc.org</a><br>"
             "Project Founder, Lead Developer</dd>"
        "<dt><b>Marcus \"EgS\" Eggenberger</b></dt><dd><a href=\"mailto:egs@quassel-irc.org\">egs@quassel-irc.org</a><br>"
             "Project Motivator, Lead Developer, Mac Maintainer</dd>"
        "<dt><b>Alexander \"phon\" von Renteln</b></dt><dd><a href=\"mailto:phon@quassel-irc.org\">phon@quassel-irc.org</a><br>"
             "Developer, Windows Maintainer</dd>"
        "</dl>";

  return res;
}

QString AboutDlg::contributors() const {
  QString res;
  res = tr("We would like to thank the following contributors (in alphabetical order) and everybody we forgot to mention here:") + "<br>"
           "<dl>"
           "<dt><b>Marco \"kaffeedoktor\" Genise</b></dt><dd><a href=\"mailto:kaffeedoktor@quassel-irc.org\">kaffeedoktor@quassel-irc.org</a><br>"
                  "Ideas, Hacking, Motivation</dd>"
           "<dt><b>John \"nox-Hand\" Hand</b></dt><dd>Gorgeous Application and Tray Icons</dd>"
           "<dt><b>Jonas \"Dante\" Heese</b></dt><dd>Project Founder, ebuilder</dd>"
           "<dt><b>Diego \"Flameeyes\" Petten&ograve;</b></dt><dd>Gentoo Maintainer</dd>"
           "<dt><b>Harald \"apachelogger\" Sitter</b></dt><dd>{k|U}buntu packager, Motivator, Promoter</dd>"
           "<dt><b>Daniel \"son\" Steinmetz</b></dt><dd>Early Beta Tester and Bughunter (on Vista&trade;!)</dd>"
           "<dt><b>Adam \"adamt\" Tulinius</b></dt><dd>Early Beta Tester and Bughunter, Danish Translation</dd>"
           "<dt><b>Pavel \"int\" Volkovitskiy</b></dt><dd>Early Beta Tester and Bughunter</dd>"
           "</dl>";

  return res;
}

QString AboutDlg::thanksTo() const {
  QString res;
  res = tr("Special thanks goes to:<br>"
           "<dl>"
           "<dt><b>John \"nox-Hand\" Hand</b></dt>"
              "<dd>for great artwork and the Quassel logo/icon</dt>"
           "<dt><b><a href=\"http://www.oxygen-icons.org\">The Oxygen Team</a></b></dt>"
              "<dd>for creating most of the other shiny icons you see throughout Quassel</dd>"
           "<dt><b><a href=\"http://www.trolltech.com\">Trolltech</a></b></dt>"
              "<dd>for creating Qt and Qtopia, and for sponsoring development of Quasseltopia with Greenphones and more</dd>"
          );

  return res;
}
