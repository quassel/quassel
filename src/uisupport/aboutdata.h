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

#pragma once

#include <QList>
#include <QLocale>
#include <QString>

#ifdef HAVE_KF5
#  include <KCoreAddons/KAboutData>
#endif


/**
 * Represents a contributor or author for Quassel.
 *
 * This is used to show a list of contributors in the About Quassel dialog.
 */
class AboutPerson
{
public:
    /**
     * Constructor.
     *
     * @param[in] name The person's name (in the form "Firstname Surname")
     * @param[in] nick The person's nickname, if applicable
     * @param[in] task Things the person does or has done for the project
     * @param[in] emailAddress The person's email address, if applicable
     * @param[in] translatedLanguage The language the person helped translate (only applicable for translators)
     */
    AboutPerson(const QString &name, const QString &nick, const QString &task, const QString &emailAddress = QString(), QLocale::Language translatedLanguage = QLocale::C);

    /**
     * Gets the person's name.
     *
     * @returns The person's name
     */
    QString name() const;

    /**
     * Gets the person's nick.
     *
     * @returns The person's nick
     */
    QString nick() const;

    /**
     * Gets the person's task.
     *
     * @returns The person's task
     */
    QString task() const;

    /**
     * Gets the person's e-mail address.
     *
     * @returns The person's e-mail address
     */
    QString emailAddress() const;

    /**
     * Gets the language this person helped translate.
     *
     * @returns The language this person helped translate
     */
    QLocale::Language translatedLanguage() const;

    /**
     * Gets the person's formatted name and nick.
     *
     * @returns The person's name and nick formatted for combined output
     */
    QString prettyName() const;

private:
    QString _name;               ///< The person's name
    QString _nick;               ///< The person's nick
    QString _task;               ///< The person's task
    QString _emailAddress;       ///< The person's email address
    QLocale::Language _language; ///< The language the person helps translate
};


/**
 * Holds a list of authors, contributors and translators.
 *
 * This class is meant to hold the list of people who contributed to Quassel, used for displaying
 * the About Quassel dialog. Additionally, this class can provide a KAboutData object to be shown
 * if KDE integration is enabled.
 */
class AboutData : public QObject
{
    Q_OBJECT
public:
    /**
     * Default constructor.
     *
     * @param[in] parent The parent object, if applicable
     */
    AboutData(QObject *parent = nullptr);

    /**
     * Adds an author to the list of contributors.
     *
     * Authors are people who contributed a significant amount of code to Quassel.
     *
     * @param[in] author The author to add
     * @returns A reference to this AboutData instance
     */
    AboutData &addAuthor(const AboutPerson &author);

    /**
     * Adds a list of authors to the list of contributors.
     *
     * This method allows the use of a brace initializer in order to easily add a long list of
     * people.
     *
     * @param[in] authors A list of authors to add
     * @returns A reference to this AboutData instance
     */
    AboutData &addAuthors(std::initializer_list<AboutPerson> authors);

    /**
     * Adds a contributor.
     *
     * @param[in] author The contributor to add
     * @returns A reference to this AboutData instance
     */
    AboutData &addCredit(const AboutPerson &credit);

    /**
     * Adds a list of contributors.
     *
     * This method allows the use of brace initializers in order to easily add a long list of
     * people.
     *
     * @param[in] authors A list of contributors to add
     * @returns A reference to this AboutData instance
     */
    AboutData &addCredits(std::initializer_list<AboutPerson> credits);

    /**
     * Gets the list of authors stored in this AboutData instance.
     *
     * @returns A list of authors
     */
    QList<AboutPerson> authors() const;

    /**
     * Gets the list of non-author contributors stored in this AboutData instance.
     *
     * @returns A list of contributors
     */
    QList<AboutPerson> credits() const;

#ifdef HAVE_KF5
    /**
     * Creates a KAboutData instance based on the contents of this AboutData instance.
     *
     * @returns A KAboutData instance holding the list of contributors as well as any additional
     *          data required for KAboutDialog and friends
     */
    KAboutData kAboutData() const;
#endif

    /**
     * Fills the given AboutData instance with data relevant for Quassel itself.
     *
     * This method adds a (hardcoded) list of contributors to the given AboutData instance.
     *
     * @param[in,out] aboutData An existing AboutData instance to add Quassel's contributors to
     */
    static void setQuasselPersons(AboutData *aboutData);

private:
    QList<AboutPerson> _authors;  ///< The list of authors
    QList<AboutPerson> _credits;  ///< The list of other contributors
};
