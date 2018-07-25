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

#include "inputwidget.h"

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QRect>

#include "action.h"
#include "actioncollection.h"
#include "bufferview.h"
#include "client.h"
#include "icon.h"
#include "ircuser.h"
#include "networkmodel.h"
#include "qtui.h"
#include "qtuisettings.h"
#include "tabcompleter.h"

const int leftMargin = 3;

InputWidget::InputWidget(QWidget *parent)
    : AbstractItemView(parent),
    _networkId(0)
{
    ui.setupUi(this);
    connect(ui.ownNick, SIGNAL(activated(QString)), this, SLOT(changeNick(QString)));

    layout()->setAlignment(ui.ownNick, Qt::AlignBottom);
    layout()->setAlignment(ui.inputEdit, Qt::AlignBottom);
    layout()->setAlignment(ui.showStyleButton, Qt::AlignBottom);
    layout()->setAlignment(ui.styleFrame, Qt::AlignBottom);

    setStyleOptionsExpanded(false);

    setFocusProxy(ui.inputEdit);
    ui.ownNick->setFocusProxy(ui.inputEdit);

    ui.ownNick->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui.ownNick->installEventFilter(new MouseWheelFilter(this));
    ui.inputEdit->installEventFilter(this);

    ui.inputEdit->setMinHeight(1);
    ui.inputEdit->setMaxHeight(5);
    ui.inputEdit->setMode(MultiLineEdit::MultiLine);
    ui.inputEdit->setPasteProtectionEnabled(true);

    ui.boldButton->setIcon(icon::get("format-text-bold"));
    ui.italicButton->setIcon(icon::get("format-text-italic"));
    ui.underlineButton->setIcon(icon::get("format-text-underline"));
    ui.clearButton->setIcon(icon::get("edit-clear"));
    ui.encryptionIconLabel->hide();

    _colorMenu = new QMenu();
    _colorFillMenu = new QMenu();

    QStringList names;
    names << tr("White") << tr("Black") << tr("Dark blue") << tr("Dark green") << tr("Red") << tr("Dark red") << tr("Dark magenta")  << tr("Orange")
          << tr("Yellow") << tr("Green") << tr("Dark cyan") << tr("Cyan") << tr("Blue") << tr("Magenta") << tr("Dark gray") << tr("Light gray");

    QPixmap pix(16, 16);
    for (int i = 0; i < inputLine()->mircColorMap().count(); i++) {
        pix.fill(inputLine()->mircColorMap().values()[i]);
        _colorMenu->addAction(pix, names[i])->setData(inputLine()->mircColorMap().keys()[i]);
        _colorFillMenu->addAction(pix, names[i])->setData(inputLine()->mircColorMap().keys()[i]);
    }

    pix.fill(Qt::transparent);
    _colorMenu->addAction(pix, tr("Clear Color"))->setData("");
    _colorFillMenu->addAction(pix, tr("Clear Color"))->setData("");

    ui.textcolorButton->setMenu(_colorMenu);
    // Set the default action to clear color (last added action)
    ui.textcolorButton->setDefaultAction(_colorMenu->actions().last());
    connect(_colorMenu, SIGNAL(triggered(QAction *)), this, SLOT(colorChosen(QAction *)));

    ui.highlightcolorButton->setMenu(_colorFillMenu);
    // Set the default action to clear fill color (last added action)
    ui.highlightcolorButton->setDefaultAction(_colorFillMenu->actions().last());
    connect(_colorFillMenu, SIGNAL(triggered(QAction *)), this, SLOT(colorHighlightChosen(QAction *)));

    // Needs to be done after adding the menu, otherwise the icon mysteriously vanishes until clicked
    ui.textcolorButton->setIcon(icon::get("format-text-color"));
    ui.highlightcolorButton->setIcon(icon::get("format-fill-color"));

    // Show/hide style button
    connect(ui.showStyleButton, SIGNAL(toggled(bool)), this, SLOT(setStyleOptionsExpanded(bool)));

    // Clear formatting button
    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFormat()));

    new TabCompleter(ui.inputEdit);

    UiStyleSettings fs("Fonts");
    fs.notify("UseCustomInputWidgetFont", this, SLOT(setUseCustomFont(QVariant)));
    fs.notify("InputWidget", this, SLOT(setCustomFont(QVariant)));
    if (fs.value("UseCustomInputWidgetFont", false).toBool())
        setCustomFont(fs.value("InputWidget", QFont()));

    UiSettings s("InputWidget");

    s.notify("EnableEmacsMode", this, SLOT(setEnableEmacsMode(QVariant)));
    setEnableEmacsMode(s.value("EnableEmacsMode", false));

    s.notify("ShowNickSelector", this, SLOT(setShowNickSelector(QVariant)));
    setShowNickSelector(s.value("ShowNickSelector", true));

    s.notify("ShowStyleButtons", this, SLOT(setShowStyleButtons(QVariant)));
    setShowStyleButtons(s.value("ShowStyleButtons", true));

    s.notify("EnablePerChatHistory", this, SLOT(setEnablePerChatHistory(QVariant)));
    setEnablePerChatHistory(s.value("EnablePerChatHistory", true));

    s.notify("MaxNumLines", this, SLOT(setMaxLines(QVariant)));
    setMaxLines(s.value("MaxNumLines", 5));

    s.notify("EnableScrollBars", this, SLOT(setScrollBarsEnabled(QVariant)));
    setScrollBarsEnabled(s.value("EnableScrollBars", true));

    s.notify("EnableLineWrap", this, SLOT(setLineWrapEnabled(QVariant)));
    setLineWrapEnabled(s.value("EnableLineWrap", true));

    s.notify("EnableMultiLine", this, SLOT(setMultiLineEnabled(QVariant)));
    setMultiLineEnabled(s.value("EnableMultiLine", true));

    ActionCollection *coll = QtUi::actionCollection();

    Action *activateInputline = coll->add<Action>("FocusInputLine");
    connect(activateInputline, SIGNAL(triggered()), SLOT(setFocus()));
    activateInputline->setText(tr("Focus Input Line"));
    activateInputline->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));

    connect(inputLine(), SIGNAL(textEntered(QString)), SLOT(onTextEntered(QString)), Qt::QueuedConnection); // make sure the line is already reset, bug #984
    connect(inputLine(), SIGNAL(currentCharFormatChanged(QTextCharFormat)), this, SLOT(currentCharFormatChanged(QTextCharFormat)));
}


InputWidget::~InputWidget()
{
}


void InputWidget::setUseCustomFont(const QVariant &v)
{
    if (v.toBool()) {
        UiStyleSettings fs("Fonts");
        setCustomFont(fs.value("InputWidget"));
    }
    else
        setCustomFont(QFont());
}


void InputWidget::setCustomFont(const QVariant &v)
{
    QFont font = v.value<QFont>();
    if (font.family().isEmpty())
        font = QApplication::font();
    // we don't want font styles as this conflics with mirc code richtext editing
    font.setBold(false);
    font.setItalic(false);
    font.setUnderline(false);
    font.setStrikeOut(false);
    ui.inputEdit->setCustomFont(font);
}


void InputWidget::setEnableEmacsMode(const QVariant &v)
{
    ui.inputEdit->setEmacsMode(v.toBool());
}


void InputWidget::setShowNickSelector(const QVariant &v)
{
    ui.ownNick->setVisible(v.toBool());
}


void InputWidget::setShowStyleButtons(const QVariant &v)
{
    ui.showStyleButton->setVisible(v.toBool());
}


void InputWidget::setEnablePerChatHistory(const QVariant &v)
{
    _perChatHistory = v.toBool();
}


void InputWidget::setMaxLines(const QVariant &v)
{
    ui.inputEdit->setMaxHeight(v.toInt());
}


void InputWidget::setScrollBarsEnabled(const QVariant &v)
{
    ui.inputEdit->setScrollBarsEnabled(v.toBool());
}


void InputWidget::setLineWrapEnabled(const QVariant &v)
{
    ui.inputEdit->setLineWrapEnabled(v.toBool());
}


void InputWidget::setMultiLineEnabled(const QVariant &v)
{
    ui.inputEdit->setMode(v.toBool() ? MultiLineEdit::MultiLine : MultiLineEdit::SingleLine);
}


bool InputWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return false;

    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    // keys from BufferView should be sent to (and focus) the input line
    BufferView *view = qobject_cast<BufferView *>(watched);
    if (view) {
        if (keyEvent->text().length() == 1 && !(keyEvent->modifiers() & (Qt::ControlModifier ^ Qt::AltModifier))) { // normal key press
            QChar c = keyEvent->text().at(0);
            if (c.isLetterOrNumber() || c.isSpace() || c.isPunct() || c.isSymbol()) {
                setFocus();
                QCoreApplication::sendEvent(inputLine(), keyEvent);
                return true;
            }
        }
        return false;
    }
    else if (watched == ui.inputEdit) {
        if (keyEvent->matches(QKeySequence::Find)) {
            QAction *act = GraphicalUi::actionCollection()->action("ToggleSearchBar");
            if (act) {
                act->toggle();
                return true;
            }
        }
        return false;
    }
    return false;
}


void InputWidget::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    BufferId currentBufferId = current.data(NetworkModel::BufferIdRole).value<BufferId>();
    BufferId previousBufferId = previous.data(NetworkModel::BufferIdRole).value<BufferId>();

    if (_perChatHistory) {
        //backup
        historyMap[previousBufferId].history = inputLine()->history();
        historyMap[previousBufferId].tempHistory = inputLine()->tempHistory();
        historyMap[previousBufferId].idx = inputLine()->idx();
        historyMap[previousBufferId].inputLine = inputLine()->html();

        //restore
        inputLine()->setHistory(historyMap[currentBufferId].history);
        inputLine()->setTempHistory(historyMap[currentBufferId].tempHistory);
        inputLine()->setIdx(historyMap[currentBufferId].idx);
        inputLine()->setHtml(historyMap[currentBufferId].inputLine);
        inputLine()->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);

        // FIXME this really should be in MultiLineEdit (and the const int on top removed)
        QTextBlockFormat format = inputLine()->textCursor().blockFormat();
        format.setLeftMargin(leftMargin); // we want a little space between the frame and the contents
        inputLine()->textCursor().setBlockFormat(format);
    }

    NetworkId networkId = current.data(NetworkModel::NetworkIdRole).value<NetworkId>();
    if (networkId == _networkId)
        return;

    setNetwork(networkId);
    updateNickSelector();
    updateEnabledState();
}


void InputWidget::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QItemSelectionRange changedArea(topLeft, bottomRight);
    if (changedArea.contains(selectionModel()->currentIndex())) {
        updateEnabledState();

        bool encrypted = false;

        IrcChannel *chan = qobject_cast<IrcChannel *>(Client::bufferModel()->data(selectionModel()->currentIndex(), NetworkModel::IrcChannelRole).value<QObject *>());
        if (chan)
            encrypted = chan->encrypted();

        IrcUser *user = qobject_cast<IrcUser *>(Client::bufferModel()->data(selectionModel()->currentIndex(), NetworkModel::IrcUserRole).value<QObject *>());
        if (user)
            encrypted = user->encrypted();

        if (encrypted)
            ui.encryptionIconLabel->show();
        else
            ui.encryptionIconLabel->hide();
    }
}


void InputWidget::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    NetworkId networkId;
    QModelIndex child;
    for (int row = start; row <= end; row++) {
        child = model()->index(row, 0, parent);
        if (NetworkModel::NetworkItemType != child.data(NetworkModel::ItemTypeRole).toInt())
            continue;
        networkId = child.data(NetworkModel::NetworkIdRole).value<NetworkId>();
        if (networkId == _networkId) {
            setNetwork(0);
            updateNickSelector();
            return;
        }
    }
}


void InputWidget::updateEnabledState()
{
// FIXME: Find a visualization for this that does not disable the widget!
//        Disabling kills global action shortcuts, plus users sometimes need/want to enter text
//        even in inactive channels.
#if 0
    QModelIndex currentIndex = selectionModel()->currentIndex();

    const Network *net = Client::networkModel()->networkByIndex(currentIndex);
    bool enabled = false;
    if (net) {
        // disable inputline if it's a channelbuffer we parted from or...
        enabled = (currentIndex.data(NetworkModel::ItemActiveRole).value<bool>() || (currentIndex.data(NetworkModel::BufferTypeRole).toInt() != BufferInfo::ChannelBuffer));
        // ... if we're not connected to the network at all
        enabled &= net->isConnected();
    }

    ui.inputEdit->setEnabled(enabled);
#endif
}


const Network *InputWidget::currentNetwork() const
{
    return Client::network(_networkId);
}


BufferInfo InputWidget::currentBufferInfo() const
{
    return selectionModel()->currentIndex().data(NetworkModel::BufferInfoRole).value<BufferInfo>();
};


void InputWidget::applyFormatActiveColor()
{
    if (!ui.textcolorButton->defaultAction()) {
        return;
    }
    colorChosen(ui.textcolorButton->defaultAction());
}


void InputWidget::applyFormatActiveColorFill()
{
    if (!ui.highlightcolorButton->defaultAction()) {
        return;
    }
    colorHighlightChosen(ui.highlightcolorButton->defaultAction());
}


void InputWidget::toggleFormatBold()
{
    setFormatBold(!ui.boldButton->isChecked());
}


void InputWidget::toggleFormatItalic()
{
    setFormatItalic(!ui.italicButton->isChecked());
}


void InputWidget::toggleFormatUnderline()
{
    setFormatUnderline(!ui.underlineButton->isChecked());
}


void InputWidget::clearFormat()
{
    // Clear all formatting for selection (not global)
    setFormatClear(false);
}


void InputWidget::setNetwork(NetworkId networkId)
{
    if (_networkId == networkId)
        return;

    const Network *previousNet = Client::network(_networkId);
    if (previousNet) {
        disconnect(previousNet, 0, this, 0);
        if (previousNet->me())
            disconnect(previousNet->me(), 0, this, 0);
    }

    _networkId = networkId;

    const Network *network = Client::network(networkId);
    if (network) {
        connect(network, SIGNAL(identitySet(IdentityId)), this, SLOT(setIdentity(IdentityId)));
        connectMyIrcUser();
        setIdentity(network->identity());
    }
    else {
        setIdentity(0);
        _networkId = 0;
    }
}


void InputWidget::connectMyIrcUser()
{
    const Network *network = currentNetwork();
    if (network->me()) {
        connect(network->me(), SIGNAL(nickSet(const QString &)), this, SLOT(updateNickSelector()));
        connect(network->me(), SIGNAL(userModesSet(QString)), this, SLOT(updateNickSelector()));
        connect(network->me(), SIGNAL(userModesAdded(QString)), this, SLOT(updateNickSelector()));
        connect(network->me(), SIGNAL(userModesRemoved(QString)), this, SLOT(updateNickSelector()));
        connect(network->me(), SIGNAL(awaySet(bool)), this, SLOT(updateNickSelector()));
        disconnect(network, SIGNAL(myNickSet(const QString &)), this, SLOT(connectMyIrcUser()));
        updateNickSelector();
    }
    else {
        connect(network, SIGNAL(myNickSet(const QString &)), this, SLOT(connectMyIrcUser()));
    }
}


void InputWidget::setIdentity(IdentityId identityId)
{
    if (_identityId == identityId)
        return;

    const Identity *previousIdentity = Client::identity(_identityId);
    if (previousIdentity)
        disconnect(previousIdentity, 0, this, 0);

    _identityId = identityId;

    const Identity *identity = Client::identity(identityId);
    if (identity) {
        connect(identity, SIGNAL(nicksSet(QStringList)), this, SLOT(updateNickSelector()));
    }
    else {
        _identityId = 0;
    }
    updateNickSelector();
}


void InputWidget::updateNickSelector() const
{
    ui.ownNick->clear();

    const Network *net = currentNetwork();
    if (!net)
        return;

    const Identity *identity = Client::identity(net->identity());
    if (!identity) {
        qWarning() << "InputWidget::updateNickSelector(): can't find Identity for Network" << net->networkId() << "IdentityId:" << net->identity();
        return;
    }

    int nickIdx;
    QStringList nicks = identity->nicks();
    if ((nickIdx = nicks.indexOf(net->myNick())) == -1) {
        nicks.prepend(net->myNick());
        nickIdx = 0;
    }

    if (nicks.isEmpty())
        return;

    IrcUser *me = net->me();
    if (me) {
        nicks[nickIdx] = net->myNick();
        if (!me->userModes().isEmpty())
            nicks[nickIdx] += QString(" (+%1)").arg(me->userModes());
    }

    ui.ownNick->addItems(nicks);

    if (me && me->isAway())
        ui.ownNick->setItemData(nickIdx, icon::get({"im-user-away", "user-away"}), Qt::DecorationRole);

    ui.ownNick->setCurrentIndex(nickIdx);
}


void InputWidget::changeNick(const QString &newNick) const
{
    const Network *net = currentNetwork();
    if (!net || net->isMyNick(newNick))
        return;

    // we reset the nick selecter as we have no confirmation yet, that this will succeed.
    // if the action succeeds it will be properly updated anyways.
    updateNickSelector();
    Client::userInput(BufferInfo::fakeStatusBuffer(net->networkId()), QString("/NICK %1").arg(newNick));
}


void InputWidget::onTextEntered(const QString &text)
{
    Client::userInput(currentBufferInfo(), text);

    // Remove formatting from entered text
    // TODO: Offer a way to convert pasted text to mIRC formatting codes
    setFormatClear(true);
}


void InputWidget::setFormatClear(const bool global)
{
    // Apply formatting
    QTextCharFormat fmt;
    fmt.setFontWeight(QFont::Normal);
    fmt.setFontUnderline(false);
    fmt.setFontItalic(false);
    fmt.clearForeground();
    fmt.clearBackground();
    if (global) {
        inputLine()->setCurrentCharFormat(fmt);
    } else {
        setFormatOnSelection(fmt);
    }

    // Make sure UI state follows
    ui.boldButton->setChecked(false);
    ui.italicButton->setChecked(false);
    ui.underlineButton->setChecked(false);
}


void InputWidget::setFormatBold(const bool bold)
{
    // Apply formatting
    QTextCharFormat fmt;
    fmt.setFontWeight(bold ? QFont::Bold : QFont::Normal);
    mergeFormatOnSelection(fmt);
    // Make sure UI state follows
    ui.boldButton->setChecked(bold);
}


void InputWidget::setFormatItalic(const bool italic)
{
    // Apply formatting
    QTextCharFormat fmt;
    fmt.setFontItalic(italic);
    mergeFormatOnSelection(fmt);
    // Make sure UI state follows
    ui.italicButton->setChecked(italic);
}


void InputWidget::setFormatUnderline(const bool underline)
{
    // Apply formatting
    QTextCharFormat fmt;
    fmt.setFontUnderline(underline);
    mergeFormatOnSelection(fmt);
    // Make sure UI state follows
    ui.underlineButton->setChecked(underline);
}


void InputWidget::mergeFormatOnSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = inputLine()->textCursor();
    cursor.mergeCharFormat(format);
    inputLine()->mergeCurrentCharFormat(format);
}


void InputWidget::setFormatOnSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = inputLine()->textCursor();
    cursor.setCharFormat(format);
    inputLine()->setCurrentCharFormat(format);
}


QTextCharFormat InputWidget::getFormatOfWordOrSelection()
{
    QTextCursor cursor = inputLine()->textCursor();
    return cursor.charFormat();
}


void InputWidget::setStyleOptionsExpanded(bool expanded)
{
    ui.styleFrame->setVisible(expanded);
    if (expanded) {
        ui.showStyleButton->setArrowType(Qt::LeftArrow);
        ui.showStyleButton->setToolTip(tr("Hide formatting options"));
    } else {
        ui.showStyleButton->setArrowType(Qt::RightArrow);
        ui.showStyleButton->setToolTip(tr("Show formatting options"));
    }
}


void InputWidget::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
}


void InputWidget::on_boldButton_clicked(bool checked)
{
    setFormatBold(checked);
}


void InputWidget::on_underlineButton_clicked(bool checked)
{
    setFormatUnderline(checked);
}


void InputWidget::on_italicButton_clicked(bool checked)
{
    setFormatItalic(checked);
}


void InputWidget::fontChanged(const QFont &f)
{
    ui.boldButton->setChecked(f.bold());
    ui.italicButton->setChecked(f.italic());
    ui.underlineButton->setChecked(f.underline());
}


void InputWidget::colorChosen(QAction *action)
{
    QTextCharFormat fmt;
    QColor color;
    if (action->data().value<QString>() == "") {
        color = Qt::transparent;
        fmt = getFormatOfWordOrSelection();
        fmt.clearForeground();
        setFormatOnSelection(fmt);
    }
    else {
        color = QColor(inputLine()->rgbColorFromMirc(action->data().value<QString>()));
        fmt.setForeground(color);
        mergeFormatOnSelection(fmt);
    }
    ui.textcolorButton->setDefaultAction(action);
    ui.textcolorButton->setIcon(createColorToolButtonIcon(icon::get("format-text-color"), color));
}


void InputWidget::colorHighlightChosen(QAction *action)
{
    QTextCharFormat fmt;
    QColor color;
    if (action->data().value<QString>() == "") {
        color = Qt::transparent;
        fmt = getFormatOfWordOrSelection();
        fmt.clearBackground();
        setFormatOnSelection(fmt);
    }
    else {
        color = QColor(inputLine()->rgbColorFromMirc(action->data().value<QString>()));
        fmt.setBackground(color);
        mergeFormatOnSelection(fmt);
    }
    ui.highlightcolorButton->setDefaultAction(action);
    ui.highlightcolorButton->setIcon(createColorToolButtonIcon(icon::get("format-fill-color"), color));
}


void InputWidget::on_showStyleButton_toggled(bool checked)
{
    setStyleOptionsExpanded(checked);
}


QIcon InputWidget::createColorToolButtonIcon(const QIcon &icon, const QColor &color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    QPixmap image = icon.pixmap(16, 16);
    QRect target(0, 0, 16, 14);
    QRect source(0, 0, 16, 14);
    painter.fillRect(QRect(0, 14, 16, 16), color);
    painter.drawPixmap(target, image, source);

    return QIcon(pixmap);
}


// MOUSE WHEEL FILTER
MouseWheelFilter::MouseWheelFilter(QObject *parent)
    : QObject(parent)
{
}


bool MouseWheelFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() != QEvent::Wheel)
        return QObject::eventFilter(obj, event);
    else
        return true;
}
