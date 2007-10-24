/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtGui module of the Qt eXTension library
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of th Common Public License, version 1.0, as published by
** IBM.
**
** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.
**
** You should have received a copy of the CPL along with this file.
** See the LICENSE file and the cpl1.0.txt file included with the source
** distribution for more information. If you did not receive a copy of the
** license, contact the Qxt Foundation.
**
** <http://libqxt.sourceforge.net>  <foundation@libqxt.org>
**
****************************************************************************/
#include "qxtcheckcombobox.h"
#include "qxtcheckcombobox_p.h"
#include <QStyleOptionButton>
#include <QMouseEvent>
#include <QLineEdit>
#include <QTimer>

QxtCheckComboBoxPrivate::QxtCheckComboBoxPrivate()
{
    separator = QLatin1String(",");
}

void QxtCheckComboBoxPrivate::hidePopup()
{
    qxt_p().hidePopup();
}

void QxtCheckComboBoxPrivate::updateCheckedItems()
{
    checkedItems.clear();
    for (int i = 0; i < qxt_p().model()->rowCount(); ++i)
    {
        const QModelIndex& index = qxt_p().model()->index(i, 0);
        const QVariant& data = index.data(Qt::CheckStateRole);
        const Qt::CheckState state = static_cast<Qt::CheckState>(data.toInt());
        if (state == Qt::Checked)
        {
            checkedItems += index.data().toString();
        }
    }

    if (checkedItems.count() > 0)
        qxt_p().lineEdit()->setText(checkedItems.join(separator));
    else
        qxt_p().lineEdit()->setText(defaultText);

    // TODO: find a way to recalculate a meaningful size hint

    emit qxt_p().checkedItemsChanged(checkedItems);
}

QxtCheckComboView::QxtCheckComboView(QWidget* parent)
        : QListView(parent), mode(QxtCheckComboBox::CheckIndicator)
{}

QxtCheckComboView::~QxtCheckComboView()
{}

bool QxtCheckComboView::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object);
    if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
        const QModelIndex& index = indexAt(mouse->pos());
        if (index.isValid())
        {
            bool change = false;
            switch (mode)
            {
            case QxtCheckComboBox::CheckIndicator:
                change = handleIndicatorRelease(mouse, index);
                break;

            case QxtCheckComboBox::CheckWholeItem:
                change = handleItemRelease(mouse, index);
                break;

            default:
                qWarning("QxtCheckComboView::eventFilter(): unknown mode");
                break;
            }

            if (change)
            {
                // the check state is about to change, bypass
                // combobox and deliver the event just for the listview
                QListView::mouseReleaseEvent(mouse);
            }
            else
            {
                // otherwise it's ok to close
                emit hideRequested();
            }
            return true;
        }
    }
    return false;
}

bool QxtCheckComboView::handleIndicatorRelease(QMouseEvent* event, const QModelIndex& index)
{
    // check if the mouse was released over the checkbox
    QStyleOptionButton option;
    option.QStyleOption::operator=(viewOptions());
    option.rect = visualRect(index);
    const QRect& rect = style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &option);
    return rect.contains(event->pos());
}

bool QxtCheckComboView::handleItemRelease(QMouseEvent* event, const QModelIndex& index)
{
    // check if the mouse was released outside the checkbox
    QStyleOptionButton option;
    option.QStyleOption::operator=(viewOptions());
    option.rect = visualRect(index);
    const QRect& rect = style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &option);
    if (!rect.contains(event->pos()))
    {
        Qt::CheckState state = (Qt::CheckState) index.data(Qt::CheckStateRole).toInt();
        switch (state)
        {
        case Qt::Unchecked:
            state = Qt::Checked;
            break;

        case Qt::Checked:
            state = Qt::Unchecked;
            break;

        default:
            qWarning("QxtCheckComboView::handleItemRelease(): partially checked item");
            break;
        }
        model()->setData(index, state, Qt::CheckStateRole);
    }
    return true;
}

QxtCheckComboModel::QxtCheckComboModel(QObject* parent)
        : QStandardItemModel(0, 1, parent) // rows,cols
{
}

QxtCheckComboModel::~QxtCheckComboModel()
{}

Qt::ItemFlags QxtCheckComboModel::flags(const QModelIndex& index) const
{
    return QStandardItemModel::flags(index) | Qt::ItemIsUserCheckable;
}

QVariant QxtCheckComboModel::data(const QModelIndex& index, int role) const
{
    QVariant value =  QStandardItemModel::data(index, role);
    if (role == Qt::CheckStateRole && !value.isValid())
        value = Qt::Unchecked;
    return value;
}

bool QxtCheckComboModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    bool ok = QStandardItemModel::setData(index, value, role);
    if (ok)
    {
        if (role == Qt::CheckStateRole)
        {
            emit checkStateChanged();
        }
    }
    return ok;
}

/*!
    \class QxtCheckComboBox QxtCheckComboBox
    \ingroup QxtGui
    \brief An extended QComboBox with checkable items.

    QxtComboBox is a specialized combo box with checkable items.
    Checked items are collected together in the line edit.

    \image html qxtcheckcombobox.png "QxtCheckComboBox in Plastique style."
 */

/*!
    \enum QxtCheckComboBox::CheckMode

    This enum describes the check mode.

    \sa QxtCheckComboBox::checkMode
 */

/*!
    \var QxtCheckComboBox::CheckMode QxtCheckComboBox::CheckIndicator

    The check state changes only via the check indicator (like in item views).
 */

/*!
    \var QxtCheckComboBox::CheckMode QxtCheckComboBox::CheckWholeItem

    The check state changes via the whole item (like with a combo box).
 */

/*!
    \fn QxtCheckComboBox::checkedItemsChanged(const QStringList& items)

    This signal is emitted whenever the checked items have been changed.
 */

/*!
    Constructs a new QxtCheckComboBox with \a parent.
 */
QxtCheckComboBox::QxtCheckComboBox(QWidget* parent) : QComboBox(parent)
{
    QXT_INIT_PRIVATE(QxtCheckComboBox);
    QxtCheckComboModel* model = new QxtCheckComboModel(this);
    QxtCheckComboView*  view  = new QxtCheckComboView(this);
    qxt_d().view = view;
    setModel(model);
    setView(view);

    // these 2 lines below are important and must be
    // applied AFTER QComboBox::setView() because
    // QComboBox installs its own filter on the view
    view->installEventFilter(view);			// <--- !!!
    view->viewport()->installEventFilter(view);	// <--- !!!

    // read-only contents
    QLineEdit* lineEdit = new QLineEdit(this);
    lineEdit->setReadOnly(true);
    setLineEdit(lineEdit);

    connect(view, SIGNAL(hideRequested()), &qxt_d(), SLOT(hidePopup()));
    connect(model, SIGNAL(checkStateChanged()), &qxt_d(), SLOT(updateCheckedItems()));
    QTimer::singleShot(0, &qxt_d(), SLOT(updateCheckedItems()));
}

/*!
    Destructs the combo box.
 */
QxtCheckComboBox::~QxtCheckComboBox()
{}

/*!
    Returns the check state of the item at \a index.
 */
Qt::CheckState QxtCheckComboBox::itemCheckState(int index) const
{
    return static_cast<Qt::CheckState>(itemData(index, Qt::CheckStateRole).toInt());
}

/*!
    Sets the check state of the item at \a index to \a state.
 */
void QxtCheckComboBox::setItemCheckState(int index, Qt::CheckState state)
{
    setItemData(index, state, Qt::CheckStateRole);
}

/*!
    \property QxtCheckComboBox::checkedItems
    \brief This property holds the checked items.
 */
QStringList QxtCheckComboBox::checkedItems() const
{
    return qxt_d().checkedItems;
}

void QxtCheckComboBox::setCheckedItems(const QStringList& items)
{
    // not the most efficient solution but most likely nobody
    // will put too many items into a combo box anyway so...
    foreach (const QString& text, items)
    {
        const int index = findText(text);
        setItemCheckState(index, index != -1 ? Qt::Checked : Qt::Unchecked);
    }
}

/*!
    \property QxtCheckComboBox::defaultText
    \brief This property holds the default text.

    The default text is shown when there are no checked items.
    The default value is an empty string.
 */
QString QxtCheckComboBox::defaultText() const
{
    return qxt_d().defaultText;
}

void QxtCheckComboBox::setDefaultText(const QString& text)
{
    if (qxt_d().defaultText != text)
    {
        qxt_d().defaultText = text;
        qxt_d().updateCheckedItems();
    }
}

/*!
    \property QxtCheckComboBox::separator
    \brief This property holds the default separator.

    The checked items are joined together with the separator string.
    The default value is a comma (",").
 */
QString QxtCheckComboBox::separator() const
{
    return qxt_d().separator;
}

void QxtCheckComboBox::setSeparator(const QString& separator)
{
    if (qxt_d().separator != separator)
    {
        qxt_d().separator = separator;
        qxt_d().updateCheckedItems();
    }
}

/*!
    \property QxtCheckComboBox::checkMode
    \brief This property holds the check mode.

    The check mode describes item checking behaviour.
    The default value is \b QxtCheckComboBox::CheckIndicator.

    \sa QxtCheckComboBox::CheckMode
 */
QxtCheckComboBox::CheckMode QxtCheckComboBox::checkMode() const
{
    return qxt_d().view->mode;
}

void QxtCheckComboBox::setCheckMode(QxtCheckComboBox::CheckMode mode)
{
    if (qxt_d().view->mode != mode)
    {
        qxt_d().view->mode = mode;
    }
}

void QxtCheckComboBox::keyPressEvent(QKeyEvent* event)
{
    if (event->key() != Qt::Key_Up && event->key() != Qt::Key_Down)
    {
        QComboBox::keyPressEvent(event);
    }
    else
    {
        showPopup();
    }
}

void QxtCheckComboBox::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() != Qt::Key_Up && event->key() != Qt::Key_Down)
    {
        QComboBox::keyReleaseEvent(event);
    }
    else
    {
        showPopup();
    }
}
