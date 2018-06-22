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

#include <QUuid>

#ifdef HAVE_KDE4
#  include <KMainWindow>
#elif defined HAVE_KF5
#  include <KXmlGui/KMainWindow>
#else
#  include <QMainWindow>
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
    : public KMainWindow {
#else
    : public QMainWindow {
#endif
    Q_OBJECT

public:
    MainWin(QWidget *parent = 0);
    virtual ~MainWin();

    void init();

    void addBufferView(ClientBufferViewConfig *config);
    BufferView *allBuffersView() const;
    BufferView *activeBufferView() const;

    inline BufferWidget *bufferWidget() const { return _bufferWidget; }
    inline SystemTray *systemTray() const { return _systemTray; }

    bool event(QEvent *event);

    static void flagRemoteCoreOnly(QObject *object) { object->setProperty("REMOTE_CORE_ONLY", true); }
    static bool isRemoteCoreOnly(QObject *object) { return object->property("REMOTE_CORE_ONLY").toBool(); }

    void saveStateToSettings(UiSettings &);
    void restoreStateFromSettings(UiSettings &);

    // We need to override this to add the show/hide menu bar option
    virtual QMenu *createPopupMenu();

public slots:
    void showStatusBarMessage(const QString &message);
    void hideCurrentBuffer();
    void nextBufferView();     //!< Activate the next bufferview
    void previousBufferView(); //!< Activate the previous bufferview
    void nextBuffer();
    void previousBuffer();

    void showMigrationWarning(bool show);

    //! Quit application
    void quit();

protected:
    void closeEvent(QCloseEvent *event);
    void moveEvent(QMoveEvent *event);
    void resizeEvent(QResizeEvent *event);

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
    void messagesInserted(const QModelIndex &parent, int start, int end);
    void showAboutDlg();

    /**
     * Show the channel list dialog for the network, optionally searching by channel name
     *
     * @param networkId       Network ID for associated network
     * @param channelFilters  Partial channel name to search for, or empty to show all
     */
    void showChannelList(NetworkId netId = {}, const QString &channelFilters = {});
    void showNetworkConfig(NetworkId netId = NetworkId());
    void showCoreConnectionDlg();
    void showCoreConfigWizard(const QVariantList &, const QVariantList &);
    void showCoreInfoDlg();
    void showAwayLog();
    void showSettingsDlg();
    void showNotificationsDlg();
    void showIgnoreList(QString newRule = QString());
    void showShortcutsDlg();
    void showPasswordChangeDlg();
    void showNewTransferDlg(const QUuid &transferId);
    void onFullScreenToggled();

    void doAutoConnect();

    void handleCoreConnectionError(const QString &errorMsg);
    void userAuthenticationRequired(CoreAccount *, bool *valid, const QString &errorMessage);
    void handleNoSslInClient(bool *accepted);
    void handleNoSslInCore(bool *accepted);
#ifdef HAVE_SSL
    void handleSslErrors(const QSslSocket *socket, bool *accepted, bool *permanently);
#endif

    void on_actionConfigureNetworks_triggered();
    void on_actionConfigureViews_triggered();
    void on_actionLockLayout_toggled(bool lock);

    /**
     * Apply the active color to the input widget selected or typed text
     *
     * @seealso InputWidget::applyFormatActiveColor()
     */
    void on_inputFormatApplyColor_triggered();

    /**
     * Apply the active fill color to the input widget selected or typed text background
     *
     * @seealso InputWidget::applyFormatActiveColorFill()
     */
    void on_inputFormatApplyColorFill_triggered();

    /**
     * Toggle the boldness of the input widget selected or typed text
     *
     * @seealso InputWidget::toggleFormatBold()
     */
    void on_inputFormatBold_triggered();

    /**
     * Toggle the italicness of the input widget selected or typed text
     *
     * @seealso InputWidget::toggleFormatItalic()
     */
    void on_inputFormatItalic_triggered();

    /**
     * Toggle the underlining of the input widget selected or typed text
     *
     * @seealso InputWidget::toggleFormatUnderline()
     */
    void on_inputFormatUnderline_triggered();

    /**
     * Clear the formatting of the input widget selected or typed text
     *
     * @seealso InputWidget::clearFormat()
     */
    void on_inputFormatClear_triggered();

    void on_jumpHotBuffer_triggered();
    void on_bufferSearch_triggered();
    void on_actionDebugNetworkModel_triggered();
    void on_actionDebugBufferViewOverlay_triggered();
    void on_actionDebugMessageModel_triggered();
    void on_actionDebugHotList_triggered();
    void on_actionDebugLog_triggered();

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
    void connectToCore(const QVariantMap &connInfo);
    void disconnectFromCore();

private:
#ifdef HAVE_KDE
    KHelpMenu *_kHelpMenu;
#endif

    MsgProcessorStatusWidget *_msgProcessorStatusWidget;
    CoreConnectionStatusWidget *_coreConnectionStatusWidget;
    SystemTray *_systemTray;

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

    QList<BufferViewDock *> _bufferViews;
    BufferWidget *_bufferWidget;
    NickListWidget *_nickListWidget;
    InputWidget *_inputWidget;
    ChatMonitorView *_chatMonitorView;
    TopicWidget *_topicWidget;

    QAction *_fullScreenAction;
    QMenu *_fileMenu, *_networksMenu, *_viewMenu, *_bufferViewsMenu, *_settingsMenu, *_helpMenu, *_helpDebugMenu;
    QMenu *_toolbarMenu;
    QToolBar *_mainToolBar, *_chatViewToolBar, *_nickToolBar;

    QWidget *_awayLog;

    QMessageBox* _migrationWarning{nullptr};

    bool _layoutLoaded;

    QSize _normalSize; //!< Size of the non-maximized window
    QPoint _normalPos; //!< Position of the non-maximized window

    BufferHotListFilter *_bufferHotList;
    QHash<int, BufferId> _jumpKeyMap;
    int _activeBufferViewIndex;

    bool _aboutToQuit; //closeEvent can occur multiple times on OSX

    friend class QtUi;
};
