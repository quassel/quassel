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

#include <QFileDialog>
#include <QUrl>

#include <QAudioDeviceInfo>

#include "audionotificationbackend.h"

#include "clientsettings.h"
#include "iconloader.h"
#include "mainwin.h"
#include "qtui.h"

AudioNotificationBackend::AudioNotificationBackend(QObject *parent)
    : AbstractNotificationBackend(parent),
    _media(0)
{
    NotificationSettings notificationSettings;
    notificationSettings.notify("Audio/Enabled", this, SLOT(enabledChanged(const QVariant &)));
    notificationSettings.notify("Audio/AudioFile", this, SLOT(audioFileChanged(const QVariant &)));

    createMediaObject(notificationSettings.value("Audio/AudioFile", QString()).toString());

    _enabled = notificationSettings.value("Audio/Enabled", true).toBool();
    
    _audioAvailable = !QAudioDeviceInfo::availableDevices(QAudio::AudioOutput).isEmpty();
}


AudioNotificationBackend::~AudioNotificationBackend()
{
    if (_media)
        delete _media;
}


void AudioNotificationBackend::notify(const Notification &notification)
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


void AudioNotificationBackend::close(uint notificationId)
{
    Q_UNUSED(notificationId);
}


void AudioNotificationBackend::enabledChanged(const QVariant &v)
{
    _enabled = v.toBool();
}


void AudioNotificationBackend::audioFileChanged(const QVariant &v)
{
    createMediaObject(v.toString());
}


SettingsPage *AudioNotificationBackend::createConfigWidget() const
{
    return new ConfigWidget();
}


void AudioNotificationBackend::createMediaObject(const QString &file)
{
    if (_media)
        delete _media;

    if (file.isEmpty()) {
        _media = 0;
        return;
    }

    _media = new QMediaPlayer;
    _media->setMedia(QUrl::fromLocalFile(file));
    _media->setVolume(100);
}


/***************************************************************************/

AudioNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent)
    : SettingsPage("Internal", "AudioNotification", parent),
    audioPreview(0)
{
    ui.setupUi(this);
    
    _audioAvailable = !QAudioDeviceInfo::availableDevices(QAudio::AudioOutput).isEmpty();

    ui.enabled->setIcon(SmallIcon("media-playback-start"));
    ui.play->setIcon(SmallIcon("media-playback-start"));
    ui.open->setIcon(SmallIcon("document-open"));

    connect(ui.enabled, SIGNAL(toggled(bool)), SLOT(widgetChanged()));
    connect(ui.filename, SIGNAL(textChanged(const QString &)), SLOT(widgetChanged()));
}


AudioNotificationBackend::ConfigWidget::~ConfigWidget()
{
    if (audioPreview)
        delete audioPreview;
}


void AudioNotificationBackend::ConfigWidget::widgetChanged()
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


bool AudioNotificationBackend::ConfigWidget::hasDefaults() const
{
    return true;
}


void AudioNotificationBackend::ConfigWidget::defaults()
{
    ui.enabled->setChecked(false);
    ui.filename->setText(QString());
    widgetChanged();
}


void AudioNotificationBackend::ConfigWidget::load()
{
    NotificationSettings s;
    enabled = s.value("Audio/Enabled", false).toBool();
    filename = s.value("Audio/AudioFile", QString()).toString();

    ui.enabled->setChecked(enabled);
    ui.filename->setText(filename);

    setChangedState(false);
}


void AudioNotificationBackend::ConfigWidget::save()
{
    NotificationSettings s;
    s.setValue("Audio/Enabled", ui.enabled->isChecked());
    s.setValue("Audio/AudioFile", ui.filename->text());
    load();
}


void AudioNotificationBackend::ConfigWidget::on_open_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select Audio File"));
    if (!file.isEmpty()) {
        ui.filename->setText(file);
        ui.play->setEnabled(true);
        widgetChanged();
    }
}


void AudioNotificationBackend::ConfigWidget::on_play_clicked()
{
    if (_audioAvailable) {
        if (!ui.filename->text().isEmpty()) {
            if (audioPreview)
                delete audioPreview;

            audioPreview = new QMediaPlayer;
            audioPreview->setMedia(QUrl::fromLocalFile(ui.filename->text()));
            audioPreview->setVolume(100);
            audioPreview->play();
        }
    }
    else
        QApplication::beep();
}
