/***************************************************************************
 *   Copyright (C) 2005-2018 by the Quassel Project                        *
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
    : AbstractNotificationBackend(parent)
{
    NotificationSettings notificationSettings;
    notificationSettings.notify("QtMultimedia/Enabled", this, SLOT(enabledChanged(const QVariant &)));
    notificationSettings.notify("QtMultimedia/AudioFile", this, SLOT(audioFileChanged(const QVariant &)));

    createMediaObject(notificationSettings.value("QtMultimedia/AudioFile", QString()).toString());

    _enabled = notificationSettings.value("QtMultimedia/Enabled", true).toBool();
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
    if (file.isEmpty()) {
        _media.reset();
        return;
    }

    _media.reset(new QMediaPlayer);
    _media->setMedia(QUrl::fromLocalFile(file));
}


/***************************************************************************/

QtMultimediaNotificationBackend::ConfigWidget::ConfigWidget(QWidget *parent)
    : SettingsPage("Internal", "QtMultimediaNotification", parent)
{
    ui.setupUi(this);
    ui.enabled->setIcon(QIcon::fromTheme("media-playback-start"));
    ui.play->setIcon(QIcon::fromTheme("media-playback-start"));
    ui.open->setIcon(QIcon::fromTheme("document-open"));

    _audioAvailable = (QMediaPlayer().availability() == QMultimedia::Available);

    connect(ui.enabled, SIGNAL(toggled(bool)), SLOT(widgetChanged()));
    connect(ui.filename, SIGNAL(textChanged(const QString &)), SLOT(widgetChanged()));
}


void QtMultimediaNotificationBackend::ConfigWidget::widgetChanged()
{
    if (! _audioAvailable) {
        ui.play->setEnabled(ui.enabled->isChecked());
        ui.open->setEnabled(false);
        ui.filename->setEnabled(false);
        ui.filename->setText({});
    }
    else {
        ui.play->setEnabled(ui.enabled->isChecked() && !ui.filename->text().isEmpty());

        bool changed = (_enabled != ui.enabled->isChecked() || _filename != ui.filename->text());

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
    ui.filename->setText({});
    widgetChanged();
}


void QtMultimediaNotificationBackend::ConfigWidget::load()
{
    NotificationSettings s;
    _enabled = s.value("QtMultimedia/Enabled", false).toBool();
    _filename = s.value("QtMultimedia/AudioFile", QString()).toString();

    ui.enabled->setChecked(_enabled);
    ui.filename->setText(_filename);

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
            _audioPreview.reset(new QMediaPlayer);
            _audioPreview->setMedia(QUrl::fromLocalFile(ui.filename->text()));
            _audioPreview->play();
        }
    }
    else
        QApplication::beep();
}
