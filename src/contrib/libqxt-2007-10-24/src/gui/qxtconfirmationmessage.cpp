/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtGui module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/
#include "qxtconfirmationmessage.h"

#include <QCoreApplication>
#if QT_VERSION >= 0x040200
#include <QDialogButtonBox>
#endif // QT_VERSION
#include <QPushButton>
#include <QGridLayout>
#include <QCheckBox>

static const QLatin1String DEFAULT_ORGANIZATION("QxtGui");
static const QLatin1String DEFAULT_APPLICATION("QxtConfirmationMessage");

class QxtConfirmationMessagePrivate : public QxtPrivate<QxtConfirmationMessage>
{
public:
    QXT_DECLARE_PUBLIC(QxtConfirmationMessage);
    void init(const QString& message = QString());
    QString key() const;
    static QString key(const QString& title, const QString& text, const QString& informativeText = QString());
    int showAgain();
    void doNotShowAgain(int result);
    static void reset(const QString& title, const QString& text, const QString& informativeText);

    QCheckBox* confirm;
    static QString path;
    static QSettings::Scope scope;
};

QString QxtConfirmationMessagePrivate::path;
QSettings::Scope QxtConfirmationMessagePrivate::scope = QSettings::UserScope;

void QxtConfirmationMessagePrivate::init(const QString& message)
{
#if QT_VERSION >= 0x040200
    confirm = new QCheckBox(&qxt_p());
    if (!message.isNull())
        confirm->setText(message);
    else
        confirm->setText(QxtConfirmationMessage::tr("Do not show again."));

    QGridLayout* grid = qobject_cast<QGridLayout*>(qxt_p().layout());
    QDialogButtonBox* buttons = qFindChild<QDialogButtonBox*>(&qxt_p());
    if (grid && buttons)
    {
        const int idx = grid->indexOf(buttons);
        int row, column, rowSpan, columnSpan = 0;
        grid->getItemPosition(idx, &row, &column, &rowSpan, &columnSpan);
        QLayoutItem* buttonsItem = grid->takeAt(idx);
        grid->addWidget(confirm, row, column, rowSpan, columnSpan, Qt::AlignLeft | Qt::AlignTop);
        grid->addItem(buttonsItem, ++row, column, rowSpan, columnSpan);
    }
#endif // QT_VERSION
}

QString QxtConfirmationMessagePrivate::key() const
{
#if QT_VERSION >= 0x040200
    return key(qxt_p().windowTitle(), qxt_p().text(), qxt_p().informativeText());
#else
    return key(qxt_p().windowTitle(), qxt_p().text());
#endif // QT_VERSION
}

QString QxtConfirmationMessagePrivate::key(const QString& title, const QString& text, const QString& informativeText)
{
    const QString all = title + text + informativeText;
    const QByteArray data = all.toLocal8Bit();
    return QString::number(qChecksum(data.constData(), data.length()));
}

int QxtConfirmationMessagePrivate::showAgain()
{
    QString organization = QCoreApplication::organizationName();
    QString application  = QCoreApplication::applicationName();
    if (organization.isEmpty())
        organization = DEFAULT_ORGANIZATION;
    if (application.isEmpty())
        application = DEFAULT_APPLICATION;
    QSettings settings(scope, organization, application);
    if (!path.isEmpty())
        settings.beginGroup(path);
    return settings.value(key(), -1).toInt();
}

void QxtConfirmationMessagePrivate::doNotShowAgain(int result)
{
    QString organization = QCoreApplication::organizationName();
    QString application  = QCoreApplication::applicationName();
    if (organization.isEmpty())
        organization = DEFAULT_ORGANIZATION;
    if (application.isEmpty())
        application = DEFAULT_APPLICATION;
    QSettings settings(scope, organization, application);
    if (!path.isEmpty())
        settings.beginGroup(path);
    settings.setValue(key(), result);
}

void QxtConfirmationMessagePrivate::reset(const QString& title, const QString& text, const QString& informativeText)
{
    QString organization = QCoreApplication::organizationName();
    QString application  = QCoreApplication::applicationName();
    if (organization.isEmpty())
        organization = DEFAULT_ORGANIZATION;
    if (application.isEmpty())
        application = DEFAULT_APPLICATION;
    QSettings settings(scope, organization, application);
    if (!path.isEmpty())
        settings.beginGroup(path);
    settings.remove(key(title, text, informativeText));
}

/*!
    \class QxtConfirmationMessage QxtConfirmationMessage
    \ingroup QxtGui
    \brief A confirmation message.

    QxtConfirmationMessage is a confirmation message with checkable
    <b>"Do not show again."</b> option. A checked and accepted confirmation
    message is no more shown until reseted.

    Example usage:
    \code
    void MainWindow::closeEvent(QCloseEvent* event)
    {
        static const QString text(tr("Are you sure you want to quit?"));
        if (QxtConfirmationMessage::confirm(this, tr("Confirm"), text) == QMessageBox::No)
            event->ignore();
    }
    \endcode

    \image html qxtconfirmationmessage.png "QxtConfirmationMessage in action."

    \note \b QCoreApplication::organizationName and \b QCoreApplication::applicationName
    are used for storing settings. In case these properties are empty, \b "QxtGui" and
    \b "QxtConfirmationMessage" are used, respectively.

    \note Requires Qt 4.2 or newer.
 */

/*!
    Constructs a new QxtConfirmationMessage with \a parent.
 */
QxtConfirmationMessage::QxtConfirmationMessage(QWidget* parent)
        : QMessageBox(parent)
{
    QXT_INIT_PRIVATE(QxtConfirmationMessage);
    qxt_d().init();
}

/*!
    Constructs a new QxtConfirmationMessage with \a icon, \a title, \a text, \a confirmation, \a buttons, \a parent and \a flags.
 */
#if QT_VERSION >= 0x040200
QxtConfirmationMessage::QxtConfirmationMessage(QMessageBox::Icon icon, const QString& title, const QString& text, const QString& confirmation,
        QMessageBox::StandardButtons buttons, QWidget* parent, Qt::WindowFlags flags)
        : QMessageBox(icon, title, text, buttons, parent, flags)
{
    QXT_INIT_PRIVATE(QxtConfirmationMessage);
    qxt_d().init(confirmation);
}
#endif // QT_VERSION

/*!
    Destructs the confirmation message.
 */
QxtConfirmationMessage::~QxtConfirmationMessage()
{}

// QMessageBox::StandardButton showNewMessageBox() (qmessagebox.cpp)
#if QT_VERSION >= 0x040200
QMessageBox::StandardButton QxtConfirmationMessage::confirm(QWidget* parent,
        const QString& title, const QString& text, const QString& confirmation,
        QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    QxtConfirmationMessage msgBox(QMessageBox::NoIcon, title, text, confirmation, QMessageBox::NoButton, parent);
    QDialogButtonBox* buttonBox = qFindChild<QDialogButtonBox*>(&msgBox);
    Q_ASSERT(buttonBox != 0);

    uint mask = QMessageBox::FirstButton;
    while (mask <= QMessageBox::LastButton)
    {
        uint sb = buttons & mask;
        mask <<= 1;
        if (!sb)
            continue;
        QPushButton* button = msgBox.addButton((QMessageBox::StandardButton)sb);
        // Choose the first accept role as the default
        if (msgBox.defaultButton())
            continue;
        if ((defaultButton == QMessageBox::NoButton && buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
                || (defaultButton != QMessageBox::NoButton && sb == uint(defaultButton)))
            msgBox.setDefaultButton(button);
    }
    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;
    return msgBox.standardButton(msgBox.clickedButton());
}
#endif // QT_VERSION

/*!
    \property QxtConfirmationMessage::confirmationText
    \brief This property holds the confirmation text

    The default value is <b>"Do not show again."</b>
 */
QString QxtConfirmationMessage::confirmationText() const
{
    return qxt_d().confirm->text();
}

void QxtConfirmationMessage::setConfirmationText(const QString& confirmation)
{
    qxt_d().confirm->setText(confirmation);
}

/*!
    \return The scope used for storing settings.

    The default value is \b QSettings::UserScope.
 */
QSettings::Scope QxtConfirmationMessage::settingsScope()
{
    return QxtConfirmationMessagePrivate::scope;
}

/*!
    Sets the scope used for storing settings.
 */
void QxtConfirmationMessage::setSettingsScope(QSettings::Scope scope)
{
    QxtConfirmationMessagePrivate::scope = scope;
}

/*!
    \return The path used for storing settings.

    The default value is an empty string.
 */
QString QxtConfirmationMessage::settingsPath()
{
    return QxtConfirmationMessagePrivate::path;
}

/*!
    Sets the path used for storing settings.
 */
void QxtConfirmationMessage::setSettingsPath(const QString& path)
{
    QxtConfirmationMessagePrivate::path = path;
}

/*!
    Shows the confirmation message if necessary. The confirmation message is not
    shown in case <b>"Do not show again."</b> has been checked while the same
    confirmation message was earlierly accepted.

    A confirmation message is identified by the combination of title,
    \b QMessageBox::text and optional \b QMessageBox::informativeText.

    A clicked button with role \b QDialogButtonBox::AcceptRole or
    \b QDialogButtonBox::YesRole is considered as "accepted".

    \warning This function does not reimplement but shadows \b QMessageBox::exec().

    \sa QWidget::windowTitle, QMessageBox::text, QMessageBox::informativeText
 */
int QxtConfirmationMessage::exec()
{
    int res = qxt_d().showAgain();
    if (res == -1)
        res = QMessageBox::exec();
    return res;
}

void QxtConfirmationMessage::done(int result)
{
#if QT_VERSION >= 0x040200
    QDialogButtonBox* buttons = qFindChild<QDialogButtonBox*>(this);
    Q_ASSERT(buttons != 0);

    int role = buttons->buttonRole(clickedButton());
    if (qxt_d().confirm->isChecked() &&
            (role == QDialogButtonBox::AcceptRole || role == QDialogButtonBox::YesRole))
    {
        qxt_d().doNotShowAgain(result);
    }
#endif // QT_VERSION
    QMessageBox::done(result);
}

/*!
    Resets confirmation message with given \a title, \a text and optional
    \a informativeText. A reseted confirmation message is shown again until
    user checks <b>"Do not show again."</b> and accepts the confirmation message.
 */
void QxtConfirmationMessage::reset(const QString& title, const QString& text, const QString& informativeText)
{
    QxtConfirmationMessagePrivate::reset(title, text, informativeText);
}

/*!
    Resets this instance of QxtConfirmationMessage. A reseted confirmation
    message is shown again until user checks <b>"Do not show again."</b> and
    accepts the confirmation message.
 */
void QxtConfirmationMessage::reset()
{
#if QT_VERSION >= 0x040200
    QxtConfirmationMessagePrivate::reset(windowTitle(), text(), informativeText());
#endif // QT_VERSION
}
