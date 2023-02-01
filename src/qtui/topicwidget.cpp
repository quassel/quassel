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

#include "topicwidget.h"

#include "client.h"
#include "graphicalui.h"
#include "icon.h"
#include "networkmodel.h"
#include "uisettings.h"
#include "uistyle.h"
#include "util.h"

TopicWidget::TopicWidget(QWidget* parent)
    : AbstractItemView(parent)
{
    ui.setupUi(this);
    ui.topicEditButton->setIcon(icon::get("edit-rename"));
    ui.topicLineEdit->setLineWrapEnabled(true);
    ui.topicLineEdit->installEventFilter(this);

    connect(ui.topicLabel, &StyledLabel::clickableActivated, this, &TopicWidget::clickableActivated);
    connect(ui.topicLineEdit, &MultiLineEdit::noTextEntered, this, &TopicWidget::on_topicLineEdit_textEntered);

    UiSettings s("TopicWidget");
    s.notify("DynamicResize", this, &TopicWidget::updateResizeMode);
    s.notify("ResizeOnHover", this, &TopicWidget::updateResizeMode);
    updateResizeMode();

    UiStyleSettings fs("Fonts");
    fs.notify("UseCustomTopicWidgetFont", this, &TopicWidget::setUseCustomFont);
    fs.notify("TopicWidget", this, selectOverload<const QVariant&>(&TopicWidget::setCustomFont));
    if (fs.value("UseCustomTopicWidgetFont", false).toBool())
        setCustomFont(fs.value("TopicWidget", QFont()));

    _mouseEntered = false;
    _readonly = false;
}

void TopicWidget::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);
    setTopic(current);
}

void TopicWidget::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    QItemSelectionRange changedArea(topLeft, bottomRight);
    QModelIndex currentTopicIndex = selectionModel()->currentIndex().sibling(selectionModel()->currentIndex().row(), 1);
    if (changedArea.contains(currentTopicIndex))
        setTopic(selectionModel()->currentIndex());
}

void TopicWidget::setUseCustomFont(const QVariant& v)
{
    if (v.toBool()) {
        UiStyleSettings fs("Fonts");
        setCustomFont(fs.value("TopicWidget").value<QFont>());
    }
    else
        setCustomFont(QFont());
}

void TopicWidget::setCustomFont(const QVariant& v)
{
    UiStyleSettings fs("Fonts");
    if (!fs.value("UseCustomTopicWidgetFont", false).toBool())
        return;

    setCustomFont(v.value<QFont>());
}

void TopicWidget::setCustomFont(const QFont& f)
{
    QFont font = f;
    if (font.family().isEmpty())
        font = QApplication::font();

    ui.topicLineEdit->setCustomFont(font);
    ui.topicLabel->setCustomFont(font);
}

void TopicWidget::setTopic(const QModelIndex& index)
{
    QString newtopic;
    bool readonly = true;

    BufferId id = index.data(NetworkModel::BufferIdRole).value<BufferId>();
    if (id.isValid()) {
        QModelIndex index0 = index.sibling(index.row(), 0);
        const Network* network = Client::network(Client::networkModel()->networkId(id));

        switch (Client::networkModel()->bufferType(id)) {
        case BufferInfo::StatusBuffer:
            if (network) {
                newtopic = QString("%1 (%2) | %3 | %4")
                               .arg(network->networkName().toHtmlEscaped())
                               .arg(network->currentServer().toHtmlEscaped())
                               .arg(tr("Users: %1").arg(network->ircUsers().count()))
                               .arg(tr("Lag: %1 msecs").arg(network->latency()));
            }
            else {
                newtopic = index0.data(Qt::DisplayRole).toString();
            }
            break;

        case BufferInfo::ChannelBuffer:
            newtopic = index.sibling(index.row(), 1).data().toString();
            readonly = false;
            break;

        case BufferInfo::QueryBuffer: {
            QString nickname = index0.data(Qt::DisplayRole).toString();
            if (network) {
                const IrcUser* user = network->ircUser(nickname);
                if (user) {
                    newtopic = QString("%1%2%3 | %4@%5")
                                   .arg(nickname)
                                   .arg(user->userModes().isEmpty() ? QString() : QString(" (+%1)").arg(user->userModes()))
                                   .arg(user->realName().isEmpty() ? QString() : QString(" | %1").arg(user->realName()))
                                   .arg(user->user())
                                   .arg(user->host());
                }
                else {  // no such user
                    newtopic = nickname;
                }
            }
            else {  // no valid Network-Obj.
                newtopic = nickname;
            }
            break;
        }
        default:
            newtopic = index0.data(Qt::DisplayRole).toString();
        }
    }

    QString sanitizedNewTopic = sanitizeTopic(newtopic);
    if (readonly != _readonly || sanitizedNewTopic != _topic)
    {
        _topic = sanitizedNewTopic;
        _readonly = readonly;

        ui.topicEditButton->setVisible(!_readonly);
        ui.topicLabel->setText(_topic);
        ui.topicLineEdit->setPlainText(_topic);
        switchPlain();
    }
}

void TopicWidget::setReadOnly(const bool& readonly)
{
    if (_readonly == readonly)
        return;

    _readonly = readonly;
}

void TopicWidget::updateResizeMode()
{
    StyledLabel::ResizeMode mode = StyledLabel::NoResize;
    UiSettings s("TopicWidget");
    if (s.value("DynamicResize", true).toBool()) {
        if (s.value("ResizeOnHover", true).toBool())
            mode = StyledLabel::ResizeOnHover;
        else
            mode = StyledLabel::DynamicResize;
    }

    ui.topicLabel->setResizeMode(mode);
}

void TopicWidget::clickableActivated(const Clickable& click)
{
    NetworkId networkId = selectionModel()->currentIndex().data(NetworkModel::NetworkIdRole).value<NetworkId>();
    UiStyle::StyledString sstr = GraphicalUi::uiStyle()->styleString(GraphicalUi::uiStyle()->mircToInternal(_topic),
                                                                     UiStyle::FormatType::PlainMsg);
    click.activate(networkId, sstr.plainText);
}

void TopicWidget::on_topicLineEdit_textEntered()
{
    QModelIndex currentIdx = currentIndex();
    if (currentIdx.isValid() && currentIdx.data(NetworkModel::BufferTypeRole) == BufferInfo::ChannelBuffer) {
        BufferInfo bufferInfo = currentIdx.data(NetworkModel::BufferInfoRole).value<BufferInfo>();
        if (ui.topicLineEdit->text().isEmpty())
            Client::userInput(bufferInfo, QString("/quote TOPIC %1 :").arg(bufferInfo.bufferName()));
        else
            Client::userInput(bufferInfo, QString("/topic %1").arg(ui.topicLineEdit->text()));
    }
    switchPlain();
}

void TopicWidget::on_topicEditButton_clicked()
{
    switchEditable();
}

void TopicWidget::switchEditable()
{
    ui.stackedWidget->setCurrentIndex(1);
    ui.topicLineEdit->setFocus();
    ui.topicLineEdit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
    updateGeometry();
}

void TopicWidget::switchPlain()
{
    ui.stackedWidget->setCurrentIndex(0);
    ui.topicLineEdit->setPlainText(_topic);
    updateGeometry();
    emit switchedPlain();
}

// filter for the input widget to switch back to normal mode
bool TopicWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::FocusOut && !_mouseEntered) {
        switchPlain();
        return true;
    }

    if (event->type() == QEvent::Enter) {
        _mouseEntered = true;
    }

    if (event->type() == QEvent::Leave) {
        _mouseEntered = false;
    }

    if (event->type() != QEvent::KeyRelease)
        return QObject::eventFilter(obj, event);

    auto* keyEvent = static_cast<QKeyEvent*>(event);

    if (keyEvent->key() == Qt::Key_Escape) {
        switchPlain();
        return true;
    }

    return false;
}

QString TopicWidget::sanitizeTopic(const QString& topic)
{
    // Normally, you don't have new lines in topic messages
    // But the use of "plain text" functionnality from Qt replaces
    // some unicode characters with a new line, which then triggers
    // a stack overflow later

    QString result(topic);
    for(int i = 0; i< result.length(); i++)
    {
        QChar a = result.at(i);
        if (a.isPrint()) 
            continue;
        else
        result[i] = QChar('?');
    }

    return result;
}
