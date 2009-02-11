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

#include <QDateTime>

#include "aboutdlg.h"
#include "icon.h"
#include "iconloader.h"
#include "quassel.h"

AboutDlg::AboutDlg(QWidget *parent) : QDialog(parent) {
  ui.setupUi(this);
  ui.quasselLogo->setPixmap(DesktopIcon("quassel", IconLoader::SizeHuge));

  ui.versionLabel->setText(QString(tr("<b>Version:</b> %1<br><b>Protocol version:</b> %2<br><b>Built:</b> %3"))
                           .arg(Quassel::buildInfo().fancyVersionString)
			   .arg(Quassel::buildInfo().protocolVersion)
			   .arg(Quassel::buildInfo().buildDate));
  ui.aboutTextBrowser->setHtml(about());
  ui.authorTextBrowser->setHtml(authors());
  ui.contributorTextBrowser->setHtml(contributors());
  ui.thanksToTextBrowser->setHtml(thanksTo());

  setWindowIcon(Icon("quassel"));
}

QString AboutDlg::about() const {
  QString res;
  res = tr("<b>A modern, distributed IRC Client</b><br><br>"
           "&copy;2005-2009 by the Quassel Project<br>"
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
  res = tr("We would like to thank the following contributors (in alphabetical order) and everybody we forgot to mention here:")
      + QString::fromUtf8("<br>"
           "<dl>"
           "<dt><b>Daniel \"al\" Albers</b></dt><dd>German translation, various fixes and enhancements</dd>"
           "<dt><b>Terje \"tan\" Andersen</b></dt><dd>Norwegian translation, documentation</dd>"
           "<dt><b>Tomas \"scarabeus\" Chvatal</b></dt><dd>Czech translation</dd>"
           "<dt><b>Joshua \"tvakah\" Corbin</b></dt><dd>Various fixes</dd>"
           "<dt><b>Kevin \"KRF\" Funk</b></dt><dd>German translation</dd>"
           "<dt><b>Marco \"kaffeedoktor\" Genise</b></dt><dd>Ideas, hacking, motivation</dd>"
           "<dt><b>Sebastian \"seezer\" Goth</b></dt><dd>Various improvements and features</dd>"
           "<dt><b>H. İbrahim \"igungor\" Güngör</b></dt><dd>Turkish translation</dd>"
           "<dt><b>Edward \"Aides\" Toroshchin</b></dt><dd>Russian translation</dd>"
           "<dt><b>John \"nox-Hand\" Hand</b></dt><dd>Gorgeous application and tray icons</dd>"
           "<dt><b>Jonas \"Dante\" Heese</b></dt><dd>Project founder, various improvements</dd>"
           "<dt><b>Scott \"ScottK\" Kitterman<b></dt><dd>Kubuntu nightly packager, (packaging/build system) bughunter</dd>"
           "<dt><b>Paul \"Haudrauf\" Klumpp</b></dt><dd>Initial design and mainwindow layout</dd>"
           "<dt><b>Gábor \"ELITE_x\" Németh</b></dt><dd>Hungarian translation</dd>"
           "<dt><b>Celeste \"seele\" Paul</b></dt><dd>Usability Queen</dd>"
           "<dt><b>Regis \"ZRegis\" Perrin</b></dt><dd>French translation</dd>"
           "<dt><b>Diego \"Flameeyes\" Petten&ograve;</b></dt><dd>Gentoo maintainer, build system improvements</dd>"
           "<dt><b>Daniel \"billie\" Pielmeier</b></dt><dd>Gentoo maintainer</dd>"
           "<dt><b>Nuno \"pinheiro\" Pinheiro</b></dt><dd>Tons of Oxygen icons including our application icon</dd>"
           "<dt><b>Jure \"JLP\" Repinc</b></dt><dd>Slovenian translation</dd>"
           "<dt><b>Dennis \"DevUrandom\" Schridde</b></dt><dd>D-Bus notifications</dd>"
           "<dt><b>Jussi \"jussi01\" Schultink</b></dt><dd>Tireless tester, {ku|U}buntu tester and lobbyist, liters of delicious Finnish alcohol</dd>"
           "<dt><b>Tim \"xAFFE\" Schumacher</b></dt><dd>Fixes and feedback</dd>"
           "<dt><b>Harald \"apachelogger\" Sitter</b></dt><dd>{k|U}buntu packager, motivator, promoter</dd>"
           "<dt><b>Daniel \"son\" Steinmetz</b></dt><dd>Early beta tester and bughunter (on Vista&trade;!)</dd>"
           "<dt><b>Adam \"adamt\" Tulinius</b></dt><dd>Early beta tester and bughunter, Danish translation</dd>"
           "<dt><b>Pavel \"int\" Volkovitskiy</b></dt><dd>Early beta tester and bughunter</dd>"
           "</dl><br>"
           "...and anybody else finding and reporting bugs, giving feedback, helping others and being part of the community!");

  return res;
}

QString AboutDlg::thanksTo() const {
  QString res;
  res = tr("Special thanks goes to:<br>"
           "<dl>"
           "<dt><b>John \"nox\" Hand</b></dt>"
              "<dd>for great artwork and the Quassel logo/icon</dt>"
           "<dt><b><a href=\"http://www.oxygen-icons.org\">The Oxygen Team</a></b></dt>"
              "<dd>for creating most of the other shiny icons you see throughout Quassel</dd>"
           "<dt><b><a href=\"http://www.trolltech.com\">Qt Software formerly known as Trolltech</a></b></dt>"
              "<dd>for creating Qt and Qtopia, and for sponsoring development of QuasselTopia with Greenphones and more</dd>"
           "<dt><b><a href=\"http://www.nokia.com\">Nokia</a></b></dt>"
              "<dd>for keeping Qt alive, and for sponsoring development of Quassel Mobile with N810s</dd>"
          );

  return res;
}
