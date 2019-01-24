/***************************************************************************
 *   Copyright (C) 2005-2019 by the Quassel Project                        *
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

#include "phononnotificationbackend.h"

#include <QFileDialog>
#include <QUrl>

#include <phonon/mediaobject.h>
#include <phonon/backendcapabilities.h>

#include "clientsettings.h"
#include "icon.h"
#include "mainwin.h"
#include "qtui.h"

PhononNotificationBackend::PhononNotificationBackend(QObject *parent)
    : AbstractNotificationBackend(parent),
    _media(0)
{
    NotificationSettings notificationSettings;
    notificationSettings.notify("Phonon/Enabled", this, SLOT(enabledChanged(const QVariant &)));
    notificationSettings.notify("Phonon/AudioFile", this, SLOT(audioFileChanged(const QVariant &)));

    createMediaObject(notificationSettings.value("Phonon/AudioFile", QString()).toString());

    _enabled = notificationSettings.value("Phonon/Enabled", true).toBool();
    _audioAvailable = !Phonon::BackendCapabilities::availableAudioOutputDevices().isEmpty();
}


PhononNotificationBackend::~PhononNotificationBackend()
{
    if (_media)
        delete _media;
}


void PhononNotificationBackend::notify(const Notification &notification)
{
    if (_enabled && (notification.type == Highlight || notification.type == PrivMsg)) {
        if (_audioAvailable && _media) {
            _media->stop();
            _media->play();
        }
        else
            QApplication::beep();
    }
}


void PhononNotificationBackend::close(uint notificationId)
{
    Q_UNUSED(notificationId);
}


void PhononNotificationBackend::enabledChanged(const QVariant &v)
{
    _enabled = v.toBool();
}


void PhononNotificationBackend::audioFileChanged(const QVariant &v)
{
    createMediaObject(v.toString());
}


SettingsPage *PhononNotificationBackend::createConfigWidget() const
{
    return new ConfigWidget();
}


void PhononNotificationBackend::createMediaObject(const QString &file)
{
    if (_media)
        delete _media;

    if (file.isEmpty()) {
        _media = 0;
        return;
    }

    _media = Phonon::createPlayer(Phonon::NotificationCategory, Phonon::MediaSource(QUrl::fromLocalFile(file)));
}


/***************************************************************************/

PhononNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent)
    : SettingsPage("Internal", "PhononNotification", parent),
    audioPreview(0)
{
    ui.setupUi(this);
    _audioAvailable = !Phonon::BackendCapabilities::availableAudioOutputDevices().isEmpty();
    ui.enabled->setIcon(icon::get("media-playback-start"));
    ui.play->setIcon(icon::get("media-playback-start"));
    ui.open->setIcon(icon::get("document-open"));

    connect(ui.enabled, SIGNAL(toggled(bool)), SLOT(widgetChanged()));
    connect(ui.filename, SIGNAL(textChanged(const QString &)), SLOT(widgetChanged()));
}


PhononNotificationBackend::ConfigWidget::~ConfigWidget()
{
    if (audioPreview)
        delete audioPreview;
}


void PhononNotificationBackend::ConfigWidget::widgetChanged()
{
    if (! _audioAvailable) {
        ui.play->setEnabled(ui.enabled->isChecked());
        ui.open->setEnabled(false);
        ui.filename->setEnabled(false);
        ui.filename->setText(QString());
    }
    else {
        ui.play->setEnabled(ui.enabled->isChecked() && !ui.filename->text().isEmpty());

        bool changed = (enabled != ui.enabled->isChecked() || filename != ui.filename->text());

        if (changed != hasChanged())
            setChangedState(changed);
    }
}


bool PhononNotificationBackend::ConfigWidget::hasDefaults() const
{
    return true;
}


void PhononNotificationBackend::ConfigWidget::defaults()
{
    ui.enabled->setChecked(false);
    ui.filename->setText(QString());
    widgetChanged();
}


void PhononNotificationBackend::ConfigWidget::load()
{
    NotificationSettings s;
    enabled = s.value("Phonon/Enabled", false).toBool();
    filename = s.value("Phonon/AudioFile", QString()).toString();

    ui.enabled->setChecked(enabled);
    ui.filename->setText(filename);

    setChangedState(false);
}


void PhononNotificationBackend::ConfigWidget::save()
{
    NotificationSettings s;
    s.setValue("Phonon/Enabled", ui.enabled->isChecked());
    s.setValue("Phonon/AudioFile", ui.filename->text());
    load();
}


void PhononNotificationBackend::ConfigWidget::on_open_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select Audio File"));
    if (!file.isEmpty()) {
        ui.filename->setText(file);
        ui.play->setEnabled(true);
        widgetChanged();
    }
}


void PhononNotificationBackend::ConfigWidget::on_play_clicked()
{
    if (_audioAvailable) {
        if (!ui.filename->text().isEmpty()) {
            if (audioPreview)
                delete audioPreview;

            audioPreview = Phonon::createPlayer(Phonon::NotificationCategory, Phonon::MediaSource(QUrl::fromLocalFile(ui.filename->text())));
            audioPreview->play();
        }
    }
    else
        QApplication::beep();
}
