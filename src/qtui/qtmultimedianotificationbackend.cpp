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
#include <QIcon>
#include <QUrl>

#include "qtmultimedianotificationbackend.h"

#include "clientsettings.h"
#include "mainwin.h"
#include "qtui.h"

QtMultimediaNotificationBackend::QtMultimediaNotificationBackend(QObject *parent)
    : AbstractNotificationBackend(parent),
    _media(0)
{
    NotificationSettings notificationSettings;
    notificationSettings.notify("QtMultimedia/Enabled", this, SLOT(enabledChanged(const QVariant &)));
    notificationSettings.notify("QtMultimedia/AudioFile", this, SLOT(audioFileChanged(const QVariant &)));

    createMediaObject(notificationSettings.value("QtMultimedia/AudioFile", QString()).toString());

    _enabled = notificationSettings.value("QtMultimedia/Enabled", true).toBool();
}


QtMultimediaNotificationBackend::~QtMultimediaNotificationBackend()
{
    if (_media)
        delete _media;
}


void QtMultimediaNotificationBackend::notify(const Notification &notification)
{
    if (_enabled && (notification.type == Highlight || notification.type == PrivMsg)) {
        if (_media && _media->availability() == QMultimedia::Available) {
            _media->stop();
            _media->play();
        }
        else
            QApplication::beep();
    }
}


void QtMultimediaNotificationBackend::close(uint notificationId)
{
    Q_UNUSED(notificationId);
}


void QtMultimediaNotificationBackend::enabledChanged(const QVariant &v)
{
    _enabled = v.toBool();
}


void QtMultimediaNotificationBackend::audioFileChanged(const QVariant &v)
{
    createMediaObject(v.toString());
}


SettingsPage *QtMultimediaNotificationBackend::createConfigWidget() const
{
    return new ConfigWidget();
}


void QtMultimediaNotificationBackend::createMediaObject(const QString &file)
{
    if (_media)
        delete _media;

    if (file.isEmpty()) {
        _media = 0;
        return;
    }

    _media = new QMediaPlayer;
    _media->setMedia(QUrl::fromLocalFile(file));
}


/***************************************************************************/

QtMultimediaNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent)
    : SettingsPage("Internal", "QtMultimediaNotification", parent),
    audioPreview(0)
{
    ui.setupUi(this);
    ui.enabled->setIcon(QIcon::fromTheme("media-playback-start"));
    ui.play->setIcon(QIcon::fromTheme("media-playback-start"));
    ui.open->setIcon(QIcon::fromTheme("document-open"));

    QMediaPlayer *player = new QMediaPlayer;
    _audioAvailable = player->availability() == QMultimedia::Available;
    delete player;

    connect(ui.enabled, SIGNAL(toggled(bool)), SLOT(widgetChanged()));
    connect(ui.filename, SIGNAL(textChanged(const QString &)), SLOT(widgetChanged()));
}


QtMultimediaNotificationBackend::ConfigWidget::~ConfigWidget()
{
    if (audioPreview)
        delete audioPreview;
}


void QtMultimediaNotificationBackend::ConfigWidget::widgetChanged()
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


bool QtMultimediaNotificationBackend::ConfigWidget::hasDefaults() const
{
    return true;
}


void QtMultimediaNotificationBackend::ConfigWidget::defaults()
{
    ui.enabled->setChecked(false);
    ui.filename->setText(QString());
    widgetChanged();
}


void QtMultimediaNotificationBackend::ConfigWidget::load()
{
    NotificationSettings s;
    enabled = s.value("QtMultimedia/Enabled", false).toBool();
    filename = s.value("QtMultimedia/AudioFile", QString()).toString();

    ui.enabled->setChecked(enabled);
    ui.filename->setText(filename);

    setChangedState(false);
}


void QtMultimediaNotificationBackend::ConfigWidget::save()
{
    NotificationSettings s;
    s.setValue("QtMultimedia/Enabled", ui.enabled->isChecked());
    s.setValue("QtMultimedia/AudioFile", ui.filename->text());
    load();
}


void QtMultimediaNotificationBackend::ConfigWidget::on_open_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select Audio File"));
    if (!file.isEmpty()) {
        ui.filename->setText(file);
        ui.play->setEnabled(true);
        widgetChanged();
    }
}


void QtMultimediaNotificationBackend::ConfigWidget::on_play_clicked()
{
    if (_audioAvailable) {
        if (!ui.filename->text().isEmpty()) {
            if (audioPreview)
                delete audioPreview;

            audioPreview = new QMediaPlayer;
            audioPreview->setMedia(QUrl::fromLocalFile(ui.filename->text()));
            audioPreview->play();
        }
    }
    else
        QApplication::beep();
}
