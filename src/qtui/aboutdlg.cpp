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

  ui.versionLabel->setText(QString("<b>Version %1, Build >= %2 (%3)</b>").arg(Global::quasselVersion).arg(Global::quasselBuild).arg(Global::quasselDate));
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
        "<dt>Manuel \"Sputnick\" Nickschas</dt><dd><a href=\"mailto:sput@quassel-irc.org\">sput@quassel-irc.org</a><br>"
             "Project Founder, Lead Developer</dd><br>"
        "<dt>Marcus \"EgS\" Eggenberger</dt><dd><a href=\"mailto:egs@quassel-irc.org\">egs@quassel-irc.org</a><br>"
             "Project Motivator, Lead Developer, Mac Maintainer</dd><br>"
        "<dt>Alexander \"phon\" von Renteln</dt><dd><a href=\"mailto:alex@phon.name\">alex@phon.name</a><br>"
             "Developer, Windows Maintainer</dd>"
        "</dl>";

  return res;
}

QString AboutDlg::contributors() const {
  QString res;
  res = tr("We would like to thank the following contributors (in alphabetical order) and everybody we forgot to mention here:") + "<br>"
           "<dl>"
           "<dt>Marco \"kaffeedoktor\" Genise</dt><dd><a href=\"mailto:kaffeedoktor@quassel-irc.org\">kaffeedoktor@quassel-irc.org</a><br>"
                  "Ideas, Hacking, Motivation</dd><br>"
           "<dt>Jonas \"Dante\" Heese</dt><dd>Project Founder, ebuilder</dd><br>"
           "<dt>Daniel \"son\" Steinmetz</dt><dd>Early Beta Tester and Bughunter (on Vista&trade;!)</dd><br>"
           "<dt>Adam \"adamt\" Tulinius</dt><dd>Early Beta Tester and Bughunter, Danish Translation</dd><br>"
           "<dt>Pavel \"int\" Volkovitskiy</dt><dd>Early Beta Tester and Bughunter</dd><br>"
           "</dl>";

  return res;
}

QString AboutDlg::thanksTo() const {
  QString res;
  res = tr("Special thanks goes to:<br>"
           "<dl>"
           "<dt><a href=\"http://www.oxygen-icons.org\">The Oxygen Team</a></dt>"
              "<dd>for creating most of the shiny icons you see throughout Quassel</dd><br>"
           "<dt><a href=\"http://www.trolltech.com\">Trolltech</a></dt>"
              "<dd>for creating Qt and Qtopia, and for sponsoring development of Quasseltopia with Greenphones and more</dd>"
          );

  return res;
}
