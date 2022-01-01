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

#include <QUuid>

#ifdef HAVE_KF5
#    include <KXmlGui/KMainWindow>
#else
#    include <QMainWindow>
#endif

#include "qtui.h"
#include "titlesetter.h"
#include "uisettings.h"

class ActionCollection;
class BufferHotListFilter;
class BufferView;
class BufferViewConfig;
class ChatMonitorView;
class ClientBufferViewConfig;
class CoreAccount;
class CoreConnectionStatusWidget;
class BufferViewDock;
class BufferWidget;
class InputWidget;
class MsgProcessorStatusWidget;
class NickListWidget;
class SystemTray;
class TopicWidget;

class QLabel;
class QMenu;
class QMessageBox;
class QToolBar;

class KHelpMenu;

//!\brief The main window of Quassel's QtUi.
class MainWin
#ifdef HAVE_KDE
    : public KMainWindow
{
#else
    : public QMainWindow
{
#endif
    Q_OBJECT

public:
    MainWin(QWidget* parent = nullptr);

    void init();

    void addBufferView(ClientBufferViewConfig* config);
    BufferView* allBuffersView() const;
    BufferView* activeBufferView() const;

    inline BufferWidget* bufferWidget() const { return _bufferWidget; }
    inline SystemTray* systemTray() const { return _systemTray; }

    bool event(QEvent* event) override;

    static void flagRemoteCoreOnly(QObject* object) { object->setProperty("REMOTE_CORE_ONLY", true); }
    static bool isRemoteCoreOnly(QObject* object) { return object->property("REMOTE_CORE_ONLY").toBool(); }

    void saveStateToSettings(UiSettings&);
    void restoreStateFromSettings(UiSettings&);

    // We need to override this to add the show/hide menu bar option
    QMenu* createPopupMenu() override;

public slots:
    void showStatusBarMessage(const QString& message);
    void hideCurrentBuffer();
    void nextBufferView();      //!< Activate the next bufferview
    void previousBufferView();  //!< Activate the previous bufferview
    void nextBuffer();
    void previousBuffer();

    void showMigrationWarning(bool show);

    void onExitRequested(const QString& reason);

protected:
    void closeEvent(QCloseEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

protected slots:
    void connectedToCore();
    void setConnectedState();
    void disconnectedFromCore();
    void setDisconnectedState();

private slots:
    void addBufferView(int bufferViewConfigId);
    void awayLogDestroyed();
    void removeBufferView(int bufferViewConfigId);
    void currentBufferChanged(BufferId);
    void messagesInserted(const QModelIndex& parent, int start, int end);
    void showAboutDlg();

    /**
     * Show the channel list dialog for the network, optionally searching by channel name
     *
     * @param networkId        Network ID for associated network
     * @param channelFilters   Partial channel name to search for, or empty to show all
     * @param listImmediately  If true, immediately list channels, otherwise just show dialog
     */
    void showChannelList(NetworkId netId = {}, const QString& channelFilters = {}, bool listImmediately = false);

    void showNetworkConfig(NetworkId netId = NetworkId());
    void showCoreConnectionDlg();
    void showCoreConfigWizard(const QVariantList&, const QVariantList&);
    void showCoreInfoDlg();
    void showAwayLog();
    void showSettingsDlg();
    void showNotificationsDlg();
    void showIgnoreList(QString newRule = QString());
    void showShortcutsDlg();
    void showPasswordChangeDlg();
    void showNewTransferDlg(const QUuid& transferId);
    void onFullScreenToggled();

    void doAutoConnect();

    void handleCoreConnectionError(const QString& errorMsg);
    void userAuthenticationRequired(CoreAccount*, bool* valid, const QString& errorMessage);
    void handleNoSslInClient(bool* accepted);
    void handleNoSslInCore(bool* accepted);
    void handleSslErrors(const QSslSocket* socket, bool* accepted, bool* permanently);

    void onConfigureNetworksTriggered();
    void onConfigureViewsTriggered();
    void onLockLayoutToggled(bool lock);

    /**
     * Apply the active color to the input widget selected or typed text
     *
     * @seealso InputWidget::applyFormatActiveColor()
     */
    void onFormatApplyColorTriggered();

    /**
     * Apply the active fill color to the input widget selected or typed text background
     *
     * @seealso InputWidget::applyFormatActiveColorFill()
     */
    void onFormatApplyColorFillTriggered();

    /**
     * Toggle the boldness of the input widget selected or typed text
     *
     * @seealso InputWidget::toggleFormatBold()
     */
    void onFormatBoldTriggered();

    /**
     * Toggle the italicness of the input widget selected or typed text
     *
     * @seealso InputWidget::toggleFormatItalic()
     */
    void onFormatItalicTriggered();

    /**
     * Toggle the underlining of the input widget selected or typed text
     *
     * @seealso InputWidget::toggleFormatUnderline()
     */
    void onFormatUnderlineTriggered();

     /**
     * Toggle the strikethrough of the input widget selected or typed text
     *
     * @seealso InputWidget::toggleFormatStrikethrough()
     */
    void onFormatStrikethroughTriggered();

    /**
     * Clear the formatting of the input widget selected or typed text
     *
     * @seealso InputWidget::clearFormat()
     */
    void onFormatClearTriggered();

    void onJumpHotBufferTriggered();
    void onBufferSearchTriggered();
    void onDebugNetworkModelTriggered();
    void onDebugBufferViewOverlayTriggered();
    void onDebugMessageModelTriggered();
    void onDebugHotListTriggered();
    void onDebugLogTriggered();
    void onShowResourceTreeTriggered();

    void bindJumpKey();
    void onJumpKey();

    void clientNetworkCreated(NetworkId);
    void clientNetworkRemoved(NetworkId);
    void clientNetworkUpdated();
    void connectOrDisconnectFromNet();

    void saveMenuBarStatus(bool enabled);
    void saveStatusBarStatus(bool enabled);
    void saveMainToolBarStatus(bool enabled);

    void loadLayout();
    void saveLayout();

    void bufferViewToggled(bool enabled);
    void bufferViewVisibilityChanged(bool visible);
    void changeActiveBufferView(bool backwards);
    void changeActiveBufferView(int bufferViewId);

signals:
    void connectToCore(const QVariantMap& connInfo);
    void disconnectFromCore();

private:
#ifdef HAVE_KDE
    KHelpMenu* _kHelpMenu;
#endif

    MsgProcessorStatusWidget* _msgProcessorStatusWidget;
    CoreConnectionStatusWidget* _coreConnectionStatusWidget;
    SystemTray* _systemTray;

    TitleSetter _titleSetter;

    void setupActions();
    void setupBufferWidget();
    void setupMenus();
    void setupNickWidget();
    void setupChatMonitor();
    void setupInputWidget();
    void setupTopicWidget();
    void setupTransferWidget();
    void setupViewMenuTail();
    void setupStatusBar();
    void setupSystray();
    void setupTitleSetter();
    void setupToolBars();
    void setupHotList();

    void updateIcon();
    void enableMenus();

    QList<BufferViewDock*> _bufferViews;
    BufferWidget* _bufferWidget;
    NickListWidget* _nickListWidget;
    InputWidget* _inputWidget;
    ChatMonitorView* _chatMonitorView;
    TopicWidget* _topicWidget;

    QAction* _fullScreenAction{nullptr};
    QMenu *_fileMenu, *_networksMenu, *_viewMenu, *_bufferViewsMenu, *_settingsMenu, *_helpMenu, *_helpDebugMenu;
    QMenu* _toolbarMenu;
    QToolBar *_mainToolBar, *_chatViewToolBar, *_nickToolBar;

    QWidget* _awayLog{nullptr};

    QMessageBox* _migrationWarning{nullptr};

    bool _layoutLoaded{false};

    QSize _normalSize;  //!< Size of the non-maximized window
    QPoint _normalPos;  //!< Position of the non-maximized window

    BufferHotListFilter* _bufferHotList;
    QHash<int, BufferId> _jumpKeyMap;
    int _activeBufferViewIndex{-1};

    bool _aboutToQuit{false};  // closeEvent can occur multiple times on OSX

    friend class QtUi;
};
