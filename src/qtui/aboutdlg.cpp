/***************************************************************************
 *   Copyright (C) 2005-2015 by the Quassel Project                        *
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

#include "aboutdlg.h"

#include <QDateTime>
#include <QIcon>

#include "aboutdata.h"
#include "quassel.h"

AboutDlg::AboutDlg(QWidget *parent)
    : QDialog(parent)
    , _aboutData(new AboutData(this))
{
    AboutData::setQuasselPersons(_aboutData);

    ui.setupUi(this);
    ui.quasselLogo->setPixmap(QIcon(":/icons/quassel-64.png").pixmap(64)); // don't let the icon theme affect our logo here

    ui.versionLabel->setText(QString(tr("<b>Version:</b> %1<br><b>Version date:</b> %2<br><b>Protocol version:</b> %3"))
        .arg(Quassel::buildInfo().fancyVersionString)
        .arg(Quassel::buildInfo().commitDate)
        .arg(Quassel::buildInfo().protocolVersion));
    ui.aboutTextBrowser->setHtml(about());
    ui.authorTextBrowser->setHtml(authors());
    ui.contributorTextBrowser->setHtml(contributors());
    ui.thanksToTextBrowser->setHtml(thanksTo());

    setWindowIcon(QIcon::fromTheme("quassel", QIcon(":/icons/quassel.png")));
}


QString AboutDlg::about() const
{
    QString res;
    res = tr("<b>A modern, distributed IRC Client</b><br><br>"
             "&copy;%1 by the Quassel Project<br>"
             "<a href=\"http://quassel-irc.org\">http://quassel-irc.org</a><br>"
             "<a href=\"irc://irc.freenode.net/quassel\">#quassel</a> on <a href=\"http://www.freenode.net\">Freenode</a><br><br>"
             "Quassel IRC is dual-licensed under <a href=\"http://www.gnu.org/licenses/gpl-2.0.txt\">GPLv2</a> and "
             "<a href=\"http://www.gnu.org/licenses/gpl-3.0.txt\">GPLv3</a>.<br>"
             "Most icons are &copy; by the <a href=\"http://www.oxygen-icons.org\">Oxygen Team</a> and used under the "
             "<a href=\"http://www.gnu.org/licenses/lgpl.html\">LGPL</a>.<br><br>"
             "Please use <a href=\"http://bugs.quassel-irc.org\">http://bugs.quassel-irc.org</a> to report bugs."
        ).arg("2005-2015");

    return res;
}


QString AboutDlg::authors() const
{
    QString res;
    res = tr("Quassel IRC is mainly developed by:") + "<dl>";
    for (const auto &person : _aboutData->authors()) {
        res.append("<dt><b>" + person.prettyName() + "</b></dt><dd>");
        if (!person.emailAddress().isEmpty())
            res.append("<a href=\"mailto:" + person.emailAddress() + "\">" + person.emailAddress() + "</a><br>");
        res.append("<i>" + person.task() + "</i><br></dd>");
    }
    res.append("</dl>");
    return res;
}


QString AboutDlg::contributors() const
{
    QString res;
    res = tr("We would like to thank the following contributors (in alphabetical order) and everybody we forgot to mention here:") + "<br><dl>";
    for (const auto &person : _aboutData->credits()) {
        res.append("<dt><b>" + person.prettyName() + "</b></dt><dd><i>" + person.task() + "</i><br></dd>");
    }
    res.append("</dl>" + tr("...and anybody else finding and reporting bugs, giving feedback, helping others and being part of the community!"));

    return res;
}


QString AboutDlg::thanksTo() const
{
    QString res;
    res = tr("Special thanks goes to:<br>"
             "<dl>"
             "<dt><img src=\":/pics/quassel-eye.png\">&nbsp;<b>John \"nox\" Hand</b></dt>"
             "<dd><i>for the original Quassel icon - The All-Seeing Eye</i><br></dt>"
             "<dt><img src=\":/pics/oxygen.png\">&nbsp;<b><a href=\"http://www.oxygen-icons.org\">The Oxygen Team</a></b></dt>"
             "<dd><i>for creating all the artwork you see throughout Quassel</i><br></dd>"
             "<dt><img src=\":/pics/qt-logo-32.png\">&nbsp;<b><a href=\"http://www.trolltech.com\">Qt Software formerly known as Trolltech</a></b></dt>"
             "<dd><i>for creating Qt and Qtopia, and for sponsoring development of QuasselTopia with Greenphones and more</i><br></dd>"
             "<dt><a href=\"http://www.nokia.com\"><img src=\":/pics/nokia.png\"></a></b></dt>"
             "<dd><i>for sponsoring development of Quassel Mobile with N810s</i></dd>"
        );

    return res;
}
