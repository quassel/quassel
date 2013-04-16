/***************************************************************************
 *   Copyright (C) 2005-2013 by the Quassel Project                        *
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

#include <QDateTime>

#include "aboutdlg.h"
#include "icon.h"
#include "iconloader.h"
#include "quassel.h"

AboutDlg::AboutDlg(QWidget *parent) : QDialog(parent)
{
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


QString AboutDlg::about() const
{
    QString res;
    res = tr("<b>A modern, distributed IRC Client</b><br><br>"
             "&copy;2005-2013 by the Quassel Project<br>"
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


QString AboutDlg::authors() const
{
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


QString AboutDlg::contributors() const
{
    QString res;
    res = tr("We would like to thank the following contributors (in alphabetical order) and everybody we forgot to mention here:")
          + QString::fromUtf8("<br>"
                              "<dl>"
                              "<dt><b>Daniel \"al\" Albers</b></dt><dd>Master Of Translation, many fixes and enhancements</dd>"
                              "<dt><b>Liudas Alisauskas</b></dt><dd>Lithuanian translation</dd>"
                              "<dt><b>Terje \"tan\" Andersen</b></dt><dd>Norwegian translation, documentation</dd>"
                              "<dt><b>Jens \"amiconn\" Arnold</b></dt><dd>Postgres migration fixes</dd>"
                              "<dt><b>Adolfo Jayme Barrientos</b></dt><dd>Spanish translation</dd>"
                              "<dt><b>Rafael \"EagleScreen\" Belmonte</b></dt><dd>Spanish translation</dd>"
                              "<dt><b>Sergiu Bivol</b></dt><dd>Romanian translation</dd>"
                              "<dt><b>Bruno Brigras</b></dt><dd>Crash fixes</dd>"
                              "<dt><b>Theo \"tampakrap\" Chatzimichos</b></dt><dd>Greek translation</dd>"
                              "<dt><b>Yuri Chornoivan</b></dt><dd>Ukrainian translation</dd>"
                              "<dt><b>Tomáš \"scarabeus\" Chvátal</b></dt><dd>Czech translation</dd>"
                              "<dt><b>\"Condex\"</b></dt><dd>Galician translation</dd>"
                              "<dt><b>Joshua \"tvakah\" Corbin</b></dt><dd>Various fixes</dd>"
                              "<dt><b>\"cordata\"</b></dt><dd>Esperanto translation</dd>"
                              "<dt><b>Matthias \"pennywise\" Coy</b></dt><dd>German translation</dd>"
                              "<dt><b>\"derpella\"</b></dt><dd>Polish translation</dd>"
                              "<dt><b>\"Dorian\"</b></dt><dd>French translation</dd>"
                              "<dt><b>Chris \"stitch\" Fuenty</b></dt><dd>SASL support</dd>"
                              "<dt><b>Kevin \"KRF\" Funk</b></dt><dd>German translation</dd>"
                              "<dt><b>Fabiano \"elbryan\" Francesconi</b></dt><dd>Italian translation</dd>"
                              "<dt><b>Leo Franchi</b></dt><dd>OSX improvements</dd>"
                              "<dt><b>Sebastien Fricker</b></dt><dd>Audio backend improvements</dd>"
                              "<dt><b>Aurélien \"agateau\" Gâteau</b></dt><dd>Message Indicator support</dd>"
                              "<dt><b>Marco \"kaffeedoktor\" Genise</b></dt><dd>Ideas, hacking, motivation</dd>"
                              "<dt><b>Felix \"debfx\" Geyer</b></dt><dd>Certificate handling improvements</dd>"
                              "<dt><b>Volkan Gezer</b></dt><dd>Turkish translation</dd>"
                              "<dt><b>Sjors \"dazjorz\" Gielen</b></dt><dd>Fixes</dd>"
                              "<dt><b>Sebastian \"seezer\" Goth</b></dt><dd>Many improvements and features</dd>"
                              "<dt><b>Michael \"brot\" Groh</b></dt><dd>German translation, fixes</dd>"
                              "<dt><b>\"Gryllida\"</b></dt><dd>IRC parser improvements</dd>"
                              "<dt><b>H. İbrahim \"igungor\" Güngör</b></dt><dd>Turkish translation</dd>"
                              "<dt><b>Jiri Grönroos</b></dt><dd>Finnish translation</dd>"
                              "<dt><b>Edward Hades</b></dt><dd>Russian translation</dd>"
                              "<dt><b>John \"nox\" Hand</b></dt><dd>Former All-Seeing Eye logo</dd>"
                              "<dt><b>Jonas \"Dante\" Heese</b></dt><dd>Project founder, various improvements</dd>"
                              "<dt><b>Thomas \"Datafreak\" Hogh</b></dt><dd>Windows builder</dd>"
                              "<dt><b>Johannes \"j0hu\" Huber</b></dt><dd>Many fixes and features, bug triaging</dd>"
                              "<dt><b>Theofilos Intzoglou</b></dt><dd>Greek translation</dd>"
                              "<dt><b>Jovan Jojkić</b></dt><dd>Serbian translation</dd>"
                              "<dt><b>Scott \"ScottK\" Kitterman<b></dt><dd>Kubuntu nightly packager, (packaging/build system) bughunter</dd>"
                              "<dt><b>Paul \"Haudrauf\" Klumpp</b></dt><dd>Initial design and mainwindow layout</dd>"
                              "<dt><b>Maia Kozheva</b></dt><dd>Russian translation</dd>"
                              "<dt><b>Tae-Hoon Kwon</b></dt><dd>Korean translation</dd>"
                              "<dt><b>\"Larso\"</b></dt><dd>Finnish translation</dd>"
                              "<dt><b>Patrick \"bonsaikitten\" Lauer</b></dt><dd>Gentoo packaging</dd>"
                              "<dt><b>Chris \"Fish-Face\" Le Sueur</b></dt><dd>Various fixes and improvements</dd>"
                              "<dt><b>Hendrik \"nevcairiel\" Leppkes</b></dt><dd>Various features</dd>"
                              "<dt><b>Jason Lynch</b></dt><dd>Bugfixes</dd>"
                              "<dt><b>Martin \"m4yer\" Mayer</b></dt><dd>German translation</dd>"
                              "<dt><b>Daniel \"hydrogen\" Meltzer</b></dt><dd>Various fixes and improvements</dd>"
                              "<dt><b>Daniel E. Moctezuma</b></dt><dd>Japanese translation</dd>"
                              "<dt><b>Chris \"kode54\" Moeller</b></dt><dd>Various fixes and improvements</dd>"
                              "<dt><b>Thomas Müller</b></dt><dd>Fixes, Debian packaging</dd>"
                              "<dt><b>Gábor \"ELITE_x\" Németh</b></dt><dd>Hungarian translation</dd>"
                              "<dt><b>Per Nielsen</b></dt><dd>Danish translation</dd>"
                              "<dt><b>Marco \"Quizzlo\" Paolone</b></dt><dd>Italian translation</dd>"
                              "<dt><b>Bas \"Tucos\" Pape</b></dt><dd>Many fixes and improvements, bug and patch triaging, tireless community support</dd>"
                              "<dt><b>Bruno Patri</b></dt><dd>French translation</dd>"
                              "<dt><b>Drew \"LinuxDolt\" Patridge</b></dt><dd>BluesTheme stylesheet</dd>"
                              "<dt><b>Celeste \"seele\" Paul</b></dt><dd>Usability Queen</dd>"
                              "<dt><b>Vit Pelcak</b></dt><dd>Czech translation</dd>"
                              "<dt><b>Regis \"ZRegis\" Perrin</b></dt><dd>French translation</dd>"
                              "<dt><b>Diego \"Flameeyes\" Petten&ograve;</b></dt><dd>Gentoo maintainer, build system improvements</dd>"
                              "<dt><b>Simon Philips</b></dt><dd>Dutch translation</dd>"
                              "<dt><b>Daniel \"billie\" Pielmeier</b></dt><dd>Gentoo maintainer</dd>"
                              "<dt><b>Nuno \"pinheiro\" Pinheiro</b></dt><dd>Tons of Oxygen icons including our application icon</dd>"
                              "<dt><b>David Planella</b></dt><dd>Translation system fixes</dd>"
                              "<dt><b>Jure \"JLP\" Repinc</b></dt><dd>Slovenian translation</dd>"
                              "<dt><b>Patrick \"TheOneRing\" von Reth</b></dt><dd>MinGW support, Windows packager</dd>"
                              "<dt><b>Dirk \"MarcLandis\" Rettschlag</b></dt><dd>Various fixes and new features</dd>"
                              "<dt><b>Miguel Revilla</b></dt><dd>Spanish translation</dd>"
                              "<dt><b>Jaak Ristioja</b></dt><dd>Fixes</dd>"
                              "<dt><b>Henning \"honk\" Rohlfs</b></dt><dd>Various fixes</dd>"
                              "<dt><b>Stella \"differentreality\" Rouzi</b></dt><dd>Greek translation</dd>"
                              "<dt><b>\"salnx\"</b></dt><dd>Highlight configuration improvements</dd>"
                              "<dt><b>Martin \"sandsmark\" Sandsmark</b></dt><dd>Core fixes, Quasseldroid</dd>"
                              "<dt><b>David Sansome</b></dt><dd>OSX Notification Center support</dd>"
                              "<dt><b>Dennis \"DevUrandom\" Schridde</b></dt><dd>D-Bus notifications</dd>"
                              "<dt><b>Jussi \"jussi01\" Schultink</b></dt><dd>Tireless tester, {ku|U}buntu tester and lobbyist, liters of delicious Finnish alcohol</dd>"
                              "<dt><b>Tim \"xAFFE\" Schumacher</b></dt><dd>Fixes and feedback</dd>"
                              "<dt><b>\"sfionov\"</b></dt><dd>Russian translation</dd>"
                              "<dt><b>Harald \"apachelogger\" Sitter</b></dt><dd>{ku|U}buntu packager, motivator, promoter</dd>"
                              "<dt><b>Stefanos Sofroniou</b></dt><dd>Greek translation</dd>"
                              "<dt><b>Rüdiger \"ruediger\" Sonderfeld</b></dt><dd>Emacs keybindings</dd>"
                              "<dt><b>Alexander Stein</b></dt><dd>Tray icon fix</dd>"
                              "<dt><b>Daniel \"son\" Steinmetz</b></dt><dd>Early beta tester and bughunter (on Vista&trade;!)</dd>"
                              "<dt><b>Jesper Thomschütz</b></dt><dd>Various fixes</dd>"
                              "<dt><b>\"ToBeFree\"</b></dt><dd>German translation</dd>"
                              "<dt><b>Edward \"Aides\" Toroshchin</b></dt><dd>Russian translation</dd>"
                              "<dt><b>Adam \"adamt\" Tulinius</b></dt><dd>Early beta tester and bughunter, Danish translation</dd>"
                              "<dt><b>Frederik M.J. \"freqmod\" Vestre</b></dt><dd>Norwegian translation</dd>"
                              "<dt><b>Atte Virtanen</b></dt><dd>Finnish translation</dd>"
                              "<dt><b>Pavel \"int\" Volkovitskiy</b></dt><dd>Early beta tester and bughunter</dd>"
                              "<dt><b>Roscoe van Wyk</b></dt><dd>Fixes</dd>"
                              "<dt><b>Zé</b></dt><dd>Portuguese translation</dd>"
                              "<dt><b>Benjamin \"zbenjamin\" Zeller</b></dt><dd>Windows build system fixes</dd>"
                              "<dt><b>\"zeugma\"</b></dt><dd>Turkish translation</dd>"
                              "</dl><br>"
                              "...and anybody else finding and reporting bugs, giving feedback, helping others and being part of the community!");

    return res;
}


QString AboutDlg::thanksTo() const
{
    QString res;
    res = tr("Special thanks goes to:<br>"
             "<dl>"
             "<dt><img src=\":/pics/quassel-eye.png\">&nbsp;<b>John \"nox\" Hand</b></dt>"
             "<dd>for the original Quassel icon - The All-Seeing Eye</dt>"
             "<dt><img src=\":/pics/oxygen.png\">&nbsp;<b><a href=\"http://www.oxygen-icons.org\">The Oxygen Team</a></b></dt>"
             "<dd>for creating all the artwork you see throughout Quassel</dd>"
             "<dt><img src=\":/pics/qt-logo-32.png\">&nbsp;<b><a href=\"http://www.trolltech.com\">Qt Software formerly known as Trolltech</a></b></dt>"
             "<dd>for creating Qt and Qtopia, and for sponsoring development of QuasselTopia with Greenphones and more</dd>"
             "<dt><a href=\"http://www.nokia.com\"><img src=\":/pics/nokia.png\"></a></b></dt>"
             "<dd>for keeping Qt alive, and for sponsoring development of Quassel Mobile with N810s</dd>"
        );

    return res;
}
