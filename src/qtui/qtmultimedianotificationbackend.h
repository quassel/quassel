/***************************************************************************
 *   Copyright (C) 2005-2022 by the Quassel Project                        *
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

#include <memory>

#include <QAudioOutput>
#include <QMediaPlayer>

#include "abstractnotificationbackend.h"
#include "settingspage.h"

#include "ui_qtmultimedianotificationconfigwidget.h"

class QtMultimediaNotificationBackend : public AbstractNotificationBackend
{
    Q_OBJECT

public:
    QtMultimediaNotificationBackend(QObject* parent = nullptr);

    void notify(const Notification&) override;
    void close(uint notificationId) override;
    SettingsPage* createConfigWidget() const override;

private slots:
    void enabledChanged(const QVariant&);
    void audioFileChanged(const QVariant&);
    void createMediaObject(const QString& name);

private:
    class ConfigWidget;

    bool _enabled;
    std::unique_ptr<QMediaPlayer> _media;
    std::unique_ptr<QAudioOutput> _audioOutput;
};

class QtMultimediaNotificationBackend::ConfigWidget : public SettingsPage
{
    Q_OBJECT

public:
    ConfigWidget(QWidget* parent = nullptr);

    void save() override;
    void load() override;
    bool hasDefaults() const override;
    void defaults() override;

private slots:
    void widgetChanged();
    void on_open_clicked();
    void on_play_clicked();

private:
    Ui::QtMultimediaNotificationConfigWidget ui;

    bool _enabled;
    bool _audioAvailable;
    QString _filename;
    std::unique_ptr<QMediaPlayer> _audioPreview;
    std::unique_ptr<QAudioOutput> _audioPreviewOutput;
};
