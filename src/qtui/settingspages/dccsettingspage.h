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

#pragma once

#include "dccconfig.h"
#include "settingspage.h"
#include "ui_dccsettingspage.h"

/**
 * A settingspage for configuring DCC.
 */
class DccSettingsPage : public SettingsPage
{
    Q_OBJECT

public:
    /**
     * Constructor.
     *
     * @param[in] parent QObject parent
     */
    DccSettingsPage(QWidget *parent = nullptr);

    /// See base class docs
    bool hasDefaults() const override;

public slots:
    // See base class docs
    void save() override;
    void load() override;
    void defaults() override;

private:
    /**
     * Whether the client's DccConfig is valid
     *
     * @returns true if the client is connected and its DccConfig instance synchronized
     */
    bool isClientConfigValid() const;

    /**
     * Set the client config
     *
     * @param[in] config The client's config. Must be be valid or a nullptr.
     */
    void setClientConfig(DccConfig *config);

    // See base class docs
    QVariant loadAutoWidgetValue(const QString &widgetName) override;
    void saveAutoWidgetValue(const QString &widgetName, const QVariant &value) override;

private slots:
    /**
     * Updates the enabled state according to the current config.
     */
    void updateWidgetStates();

    /**
     * Checks if the current unsaved config differs from the client's and sets state accordingly.
     */
    void widgetHasChanged();

    /**
     * Called if the client's config was changed (e.g. if the connection state changed).
     */
    void onClientConfigChanged();

private:
    Ui::DccSettingsPage ui;              ///< The UI object
    DccConfig *_clientConfig {nullptr};  ///< Pointer to the client's config (nullptr if not synchronized/available)
    DccConfig _localConfig;              ///< Local config reflecting the widget states
};
