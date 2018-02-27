/***************************************************************************
 *   Copyright (C) 2005-2016 by the Quassel Project                        *
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

#include "aboutdata.h"

#include <QImage>

#include "quassel.h"


AboutPerson::AboutPerson(const QString &name, const QString &nick, const QString &task, const QString &emailAddress, QLocale::Language translatedLanguage)
    : _name(name)
    , _nick(nick)
    , _task(task)
    , _emailAddress(emailAddress)
    , _language(translatedLanguage)
{

}


QString AboutPerson::name() const
{
    return _name;
}


QString AboutPerson::nick() const
{
    return _nick;
}


QString AboutPerson::task() const
{
    return _task;
}


QString AboutPerson::emailAddress() const
{
    return _emailAddress;
}


QLocale::Language AboutPerson::translatedLanguage() const
{
    return _language;
}


QString AboutPerson::prettyName() const
{
    if (!name().isEmpty() && !nick().isEmpty())
        return name() + " (" + nick() + ')';

    if (name().isEmpty() && !nick().isEmpty())
        return nick();

    return name();
}


/**************************************************************************************************/


AboutData::AboutData(QObject *parent)
    : QObject(parent)
{

}


QList<AboutPerson> AboutData::authors() const
{
    return _authors;
}


QList<AboutPerson> AboutData::credits() const
{
    return _credits;
}


AboutData &AboutData::addAuthor(const AboutPerson &author)
{
    _authors.append(author);
    return *this;
}


AboutData &AboutData::addAuthors(std::initializer_list<AboutPerson> authors)
{
    _authors.append(authors);
    return *this;
}


AboutData &AboutData::addCredit(const AboutPerson &credit)
{
    _credits.append(credit);
    return *this;
}


AboutData &AboutData::addCredits(std::initializer_list<AboutPerson> credits)
{
    _credits.append(credits);
    return *this;
}

#ifdef HAVE_KF5

KAboutData AboutData::kAboutData() const
{
    KAboutData aboutData(
        Quassel::buildInfo().applicationName,
        tr("Quassel IRC"),
        Quassel::buildInfo().plainVersionString
    );
    aboutData.addLicense(KAboutLicense::GPL_V2);
    aboutData.addLicense(KAboutLicense::GPL_V3);
    aboutData.setShortDescription(tr("A modern, distributed IRC client"));
    aboutData.setProgramLogo(QVariant::fromValue(QImage(":/pics/quassel-logo.png")));
    aboutData.setBugAddress("https://bugs.quassel-irc.org/projects/quassel-irc/issues/new");
    aboutData.setOrganizationDomain(Quassel::buildInfo().organizationDomain.toUtf8());

    for (const auto &person : authors()) {
        aboutData.addAuthor(person.prettyName(), person.task(), person.emailAddress());
    }

    for (const auto &person : credits()) {
        aboutData.addCredit(person.prettyName(), person.task(), person.emailAddress());
    }

    return aboutData;
}

#endif


/**************************************************************************************************/

/*
 * NOTE: The list of contributors was retrieved from the Git history, but sometimes things fall
 *       through the cracks... especially for translations, we don't have an easy way to track
 *       contributors' names.
 *       If you find wrong data for yourself, want your nickname and/or mail addresses added or
 *       removed, or feel left out or unfairly credited, please don't hesitate to let us know! We
 *       do want to credit everyone who has contributed to Quassel development.
 */

void AboutData::setQuasselPersons(AboutData *aboutData)
{
    aboutData->addAuthors({
        { "Manuel Nickschas", "Sputnick", tr("Project Founder, Lead Developer"), "sputnick@quassel-irc.org" },
        { "Marcus Eggenberger", "EgS", tr("Project Motivator, Lead Developer"), "egs@quassel-irc.org" },
        { "Alexander von Renteln", "phon", tr("Former Lead Developer"), "phon@quassel-irc.org" },
        { "Daniel Albers", "al", tr("Master of Translation, many fixes and enhancements, Travis support") },
        { "Sebastian Goth", "seezer", tr("Many features, fixes and improvements") },
        { "Bas Pape", "Tucos", tr("Many fixes and improvements, bug and patch triaging, community support") },
        { "Shane Synan", "digitalcircuit", tr("IRCv3 support, documentation, many other improvements, outstanding PRs") },
    });

    aboutData->addCredits({
        { "A. V. Lukyanov", "", tr("OSX UI improvements") },
        { "Adam Harwood", "2kah", tr("Chatview improvements") },
        { "Adam Tulinius", "adamt", tr("Early beta tester and bughunter, Danish translation"), "", QLocale::Danish },
        { "Adolfo Jayme Barrientos", "", tr("Spanish translation"), "", QLocale::Spanish },
        { "Alexander Stein", "", tr("Tray icon fix") },
        { "Alf Gaida", "agaida", tr("Language improvements") },
        { "Allan Jude", "", tr("Documentation improvements") },
        { "Andrew Brown", "", tr("Fixes") },
        { "Arthur Titeica", "roentgen", tr("Romanian translation"), "", QLocale::Romanian },
        { "Atte Virtanen", "", tr("Finnish translation"), "", QLocale::Finnish },
        { "Aurélien Gâteau", "agateau", tr("Message indicator support") },
        { "Awad Mackie", "firesock", tr("Chatview improvements") },
        { "Armin K", "", tr("Build system fix") },
        { "", "Ayonix", tr("Build system fix") },
        { "Benjamin Zeller", "zbenjamin", tr("Windows build system fixes") },
        { "Ben Rosser", "", tr("AppData metadata") },
        { "Bernhard Scheirle", "", tr("Nicer tooltips, spell check and other improvements") },
        { "Bruno Brigras", "", tr("Crash fixes") },
        { "Bruno Patri", "", tr("French translation"), "", QLocale::French },
        { "Celeste Paul", "seele", tr("Usability review") },
        { "Chris Fuenty", "stitch", tr("SASL support") },
        { "Chris Holland", "Shade / Zren", tr("Various improvements") },
        { "Chris Le Sueur", "Fish-Face", tr("Various fixes and improvements") },
        { "Chris Moeller", "kode54", tr("Various fixes and improvements") },
        { "", "Condex", tr("Galician translation"), "", QLocale::Galician },
        { "", "cordata", tr("Esperanto translation"), "", QLocale::Esperanto },
        { "Daniel E. Moctezuma", "", tr("Japanese translation"), "", QLocale::Japanese },
        { "Daniel Meltzer", "hydrogen", tr("Various fixes and improvements") },
        { "Daniel Pielmeier", "billie", tr("Gentoo maintainer") },
        { "Daniel Schaal", "", tr("Certificate handling improvements") },
        { "Daniel Steinmetz", "son", tr("Early beta tester and bughunter (on Vista™!)") },
        { "David Planella", "", tr("Translation system fixes") },
        { "David Sansome", "", tr("OSX Notification Center support") },
        { "David Roden", "Bombe", tr("Fixes") },
        { "Deniz Türkoglu", "", tr("Mac fixes") },
        { "Dennis Schridde", "devurandom", tr("D-Bus notifications") },
        { "", "derpella", tr("Polish translation"), "", QLocale::Polish },
        { "Diego Pettenò", "Flameeyes", tr("Build system improvements") },
        { "Dirk Rettschlag", "MarcLandis", tr("Formatting support and other input line improvements, many other fixes") },
        { "", "Dorian", tr("French translation"), "", QLocale::French },
        { "Drew Patridge", "LinuxDolt", tr("BluesTheme stylesheet") },
        { "Edward Hades", "", tr("Russian translation"), "", QLocale::Russian },
        { "Fabiano Francesconi", "elbryan", tr("Italian translation"), "", QLocale::Italian },
        { "Felix Geyer", "debfx", tr("Certificate handling improvements") },
        { "Florent Castelli", "", tr("Sanitize topic handling") },
        { "Frederik M.J. Vestre", "freqmod", tr("Norwegian translation"), "", QLocale::Norwegian },
        { "Gábor Németh", "ELITE_x", tr("Hungarian translation"), "", QLocale::Hungarian },
        { "Gryllida A", "gry", tr("IRC parser improvements") },
        { "H. İbrahim Güngör", "igungor", tr("Turkish translation"), "", QLocale::Turkish },
        { "Hannah von Reth", "TheOneRing", tr("Windows build support and Appveyor maintenance, snorenotify backend") },
        { "Harald Fernengel", "harryF", tr("Initial Qt5 support") },
        { "Harald Sitter", "apachelogger", tr("{Ku|U}buntu packager, motivator, promoter") },
        { "Hendrik Leppkes", "nevcairiel", tr("Various features and improvements") },
        { "Henning Rohlfs", "honk", tr("Various fixes") },
        { "J-P Nurmi", "", tr("Various fixes") },
        { "Jaak Ristioja", "", tr("Bugfixes") },
        { "Jan Alexander Steffens", "heftig", tr("Fixes") },
        { "Janne Koschinski", "justJanne", tr("QuasselDroid and Java wizardess, documentation, bugfixes, many valuable technical discussions") },
        { "Jason Joyce", "", tr("Python improvements") },
        { "Jason Lynch", "", tr("Bugfixes") },
        { "Jens Arnold", "amiconn", tr("Postgres migration fixes") },
        { "Jerome Leclanche", "Adys", tr("Context menu fixes") },
        { "Jesper Thomschütz", "", tr("Various fixes") },
        { "Jiri Grönroos", "", tr("Finnish translation"), "", QLocale::Finnish },
        { "Johannes Huber", "johu", tr("Many fixes and improvements, bug triaging") },
        { "John Hand", "nox", tr("Original \"All-Seeing Eye\" logo") },
        { "Jonas Heese", "Dante", tr("Project founder, various improvements") },
        { "Joshua T Corbin", "tvakah", tr("Various fixes") },
        { "Jovan Jojkić", "", tr("Serbian translation"), "", QLocale::Serbian },
        { "Jure Repinc", "JLP", tr("Slovenian translation"), "", QLocale::Slovenian },
        { "Jussi Schultink", "jussi01", tr("Tireless tester, {Ku|U}buntu tester and lobbyist, liters of delicious Finnish alcohol") },
        { "K. Ernest Lee", "iFire", tr("Qt5 porting help, Travis CI setup") },
        { "Kevin Funk", "KRF", tr("German translation"), "", QLocale::German },
        { "Kimmo Huoman", "kipe", tr("Buffer merge improvements") },
        { "Konstantin Bläsi", "", tr("Fixes") },
        { "", "Larso", tr("Finnish translation"), "", QLocale::Finnish },
        { "Lasse Liehu", "", tr("Finnish translation"), "", QLocale::Finnish },
        { "Leo Franchi", "", tr("OSX improvements") },
        { "Liudas Alisauskas", "", tr("Lithuanian translation"), "", QLocale::Lithuanian },
        { "Luke Faraone", "", tr("Documentation fixes") },
        { "Maia Kozheva", "", tr("Russian translation"), "", QLocale::Russian },
        { "Marco Genise", "kaffeedoktor", tr("Ideas, hacking, initial motivation") },
        { "Marco Paolone", "Quizzlo", tr("Italian translation"), "", QLocale::Italian },
        { "Martin Mayer", "m4yer", tr("German translation"), "", QLocale::German },
        { "Martin Sandsmark", "sandsmark", tr("Many fixes and improvements, Sonnet support, QuasselDroid") },
        { "Matthias Coy", "pennywise", tr("German translation"), "", QLocale::German },
        { "Mattia Basaglia", "", tr("Fixes") },
        { "Michael Groh", "brot", tr("German translation, fixes"), "", QLocale::German },
        { "Michael Kedzierski", "ycros", tr("Mac fixes") },
        { "Michael Marley", "mamarley", tr("Many fixes and improvements; Ubuntu PPAs") },
        { "Miguel Revilla", "", tr("Spanish translation"), "", QLocale::Spanish },
        { "Nuno Pinheiro", "", tr("Tons of Oxygen icons including the Quassel logo") },
        { "Patrick Lauer", "bonsaikitten", tr("Gentoo maintainer") },
        { "Paul Klumpp", "Haudrauf", tr("Initial design and main window layout") },
        { "Pavel Volkovitskiy", "int", tr("Early beta tester and bughunter") },
        { "Per Nielsen", "", tr("Danish translation"), "", QLocale::Danish },
        { "Pete Beardmore", "elbeardmorez", tr("Linewrap for input line") },
        { "Petr Bena", "", tr("Performance improvements and cleanups") },
        { "Pierre-Hugues Husson", "", tr("/print command") },
        { "Pierre Schweitzer", "", tr("Performance improvements") },
        { "Ramanathan Sivagurunathan", "", tr("Bugfixes") },
        { "Regis Perrin", "ZRegis", tr("French translation"), "", QLocale::French },
        { "Rolf Eike Beer", "", tr("Build system fixes") },
        { "Roscoe van Wyk", "", tr("Bugfixes") },
        { "Rüdiger Sonderfeld", "ruediger", tr("Emacs keybindings") },
        { "Sai Nane", "esainane", tr("Various fixes") },
        { "", "salnx", tr("Highlight configuration improvements") },
        { "Scott Kitterman", "ScottK", tr("Kubuntu packager, (packaging/build system) bughunter") },
        { "Sebastian Meyer", "", tr("Bugfixes") },
        { "Sebastien Fricker", "", tr("Audio backend improvements") },
        { "Ryan Bales", "selabnayr", tr("Improvements") },
        { "", "sfionov", tr("Russian translation"), "", QLocale::Russian },
        { "Simon Philips", "", tr("Dutch translation"), "", QLocale::Dutch },
        { "Sjors Gielen", "dazjorz", tr("Bugfixes") },
        { "Stefanos Sofroniou", "", tr("Greek translation"), "", QLocale::Greek },
        { "Stella Rouzi", "differentreality", tr("Greek translation"), "", QLocale::Greek },
        { "Rafael Belmonte", "EagleScreen", tr("Spanish translation"), "", QLocale::Spanish },
        { "Raul Salinas-Monteagudo", "", tr("Fixes") },
        { "Rolf Eike Beer", "DerDakon", tr("Build system improvements") },
        { "Rolf Michael Bislin", "romibi", tr("Windows build support, automated OSX builds in Travis, various improvements") },
        { "Sergiu Bivol", "", tr("Romanian translation"), "", QLocale::Romanian },
        { "Tae-Hoon Kwon", "", tr("Korean translation"), "", QLocale::Korean },
        { "Terje Andersen", "tan", tr("Norwegian translation, documentation") },
        { "Theo Chatzimichos", "tampakrap", tr("Greek translation"), "", QLocale::Greek },
        { "Theofilos Intzoglou", "", tr("Greek translation"), "", QLocale::Greek },
        { "Thomas Hogh", "Datafreak", tr("Former Windows builder") },
        { "Thomas Müller", "", tr("Fixes, Debian packaging") },
        { "Tim Schumacher", "xAFFE", tr("Fixes and feedback") },
        { "", "ToBeFree", tr("German translation"), "", QLocale::German },
        { "Tomáš Chvátal", "scarabeus", tr("Czech translation"), "", QLocale::Czech },
        { "Veeti Paananen", "", tr("Certificate handling improvements") },
        { "Vit Pelcak", "", tr("Czech translation"), "", QLocale::Czech },
        { "Volkan Gezer", "", tr("Turkish translation"), "", QLocale::Turkish },
        { "Weng Xuetian", "wengxt", tr("Build system fix") },
        { "Yaohan Chen", "hagabaka", tr("Network detection improvements") },
        { "Yuri Chornoivan", "", tr("Ukrainian translation"), "", QLocale::Ukrainian },
        { "Zé", "", tr("Portuguese translation"), "", QLocale::Portuguese },
        { "", "zeugma", tr("Turkish translation"), "", QLocale::Turkish }
    });
}
