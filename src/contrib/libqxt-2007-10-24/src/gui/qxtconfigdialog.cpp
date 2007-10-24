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
#include "qxtconfigdialog.h"
#include "qxtconfigdialog_p.h"
#if QT_VERSION >= 0x040200
#include <QDialogButtonBox>
#else // QT_VERSION >= 0x040200
#include <QHBoxLayout>
#include <QPushButton>
#endif // QT_VERSION
#include <QStackedWidget>
#include <QGridLayout>
#include <QListWidget>
#include <QPainter>

QxtConfigListWidget::QxtConfigListWidget(QWidget* parent) : QListWidget(parent)
{
    setItemDelegate(new QxtConfigDelegate(this));
    viewport()->setAttribute(Qt::WA_Hover, true);
}

void QxtConfigListWidget::invalidate()
{
    hint = QSize();
    updateGeometry();
}

QSize QxtConfigListWidget::minimumSizeHint() const
{
    return sizeHint();
}

QSize QxtConfigListWidget::sizeHint() const
{
    if (!hint.isValid())
    {
        const QStyleOptionViewItem options = viewOptions();
        const bool vertical = (flow() == QListView::TopToBottom);
        for (int i = 0; i < count(); ++i)
        {
            const QSize size = itemDelegate()->sizeHint(options, model()->index(i, 0));
            if (i != 0)
                hint = hint.expandedTo(size);
            if (vertical)
                hint += QSize(0, size.height());
            else
                hint += QSize(size.width(), 0);
        }
        hint += QSize(2 * frameWidth(), 2 * frameWidth());
    }
    return hint;
}

bool QxtConfigListWidget::hasHoverEffect() const
{
    return static_cast<QxtConfigDelegate*>(itemDelegate())->hover;
}

void QxtConfigListWidget::setHoverEffect(bool enabled)
{
    static_cast<QxtConfigDelegate*>(itemDelegate())->hover = enabled;
}

void QxtConfigListWidget::scrollContentsBy(int dx, int dy)
{
    // prevent scrolling
    Q_UNUSED(dx);
    Q_UNUSED(dy);
}

QxtConfigDelegate::QxtConfigDelegate(QObject* parent)
        : QItemDelegate(parent), hover(true)
{}

void QxtConfigDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    if (hover)
    {
        QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;

        if (option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
        }
        else if ((option.state & QStyle::State_MouseOver) && (option.state & QStyle::State_Enabled))
        {
            QColor color = option.palette.color(cg, QPalette::Highlight).light();
            if (color == option.palette.color(cg, QPalette::Base))
                color = option.palette.color(cg, QPalette::AlternateBase);
            painter->fillRect(option.rect, color);
        }

        opt.showDecorationSelected = false;
        opt.state &= ~QStyle::State_HasFocus;
    }
    QItemDelegate::paint(painter, opt, index);
}

void QxtConfigDialogPrivate::init(QxtConfigDialog::IconPosition position)
{
    QxtConfigDialog* p = &qxt_p();
    grid = new QGridLayout(p);
    list = new QxtConfigListWidget(p);
    stack = new QStackedWidget(p);
    pos = position;
    QObject::connect(list, SIGNAL(currentRowChanged(int)), stack, SLOT(setCurrentIndex(int)));
    QObject::connect(stack, SIGNAL(currentChanged(int)), p, SIGNAL(currentIndexChanged(int)));

#if QT_VERSION >= 0x040200
    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, p);
    QObject::connect(buttons, SIGNAL(accepted()), p, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), p, SLOT(reject()));
#else // QT_VERSION >= 0x040200
    buttons = new QWidget(p);
    QHBoxLayout* layout = new QHBoxLayout(buttons);
    QPushButton* okButton = new QPushButton(QxtConfigDialog::tr("&OK"));
    QPushButton* cancelButton = new QPushButton(QxtConfigDialog::tr("&Cancel"));
    QObject::connect(okButton, SIGNAL(clicked()), p, SLOT(accept()));
    QObject::connect(cancelButton, SIGNAL(clicked()), p, SLOT(reject()));
    layout->addStretch();
    layout->addWidget(okButton);
    layout->addWidget(cancelButton);
#endif

    initList();
    relayout();
}

void QxtConfigDialogPrivate::initList()
{
    // no scroll bars
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // prevent editing
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // convenient navigation
    list->setTabKeyNavigation(true);
    // no dnd
    list->setAcceptDrops(false);
    list->setDragEnabled(false);
    // list fine tuning
    list->setMovement(QListView::Static);
    list->setWrapping(false);
    list->setResizeMode(QListView::Fixed);
    list->setViewMode(QListView::IconMode);
    // list->setWordWrap(false); 4.2
    // list->setSortingEnabled(false); 4.2
}

void QxtConfigDialogPrivate::relayout()
{
    // freeze
    grid->setEnabled(false);

    // clear
    while (grid->takeAt(0));

    // relayout
    switch (pos)
    {
    case QxtConfigDialog::North:
        // +-----------+
        // |   Icons   |
        // +-----------|
        // |   Stack   |
        // +-----------|
        // |  Buttons  |
        // +-----------+
        grid->addWidget(list, 0, 0);
        grid->addWidget(stack, 1, 0);
        grid->addWidget(buttons, 3, 0);
        break;

    case QxtConfigDialog::West:
        // +---+-------+
        // | I |       |
        // | c |       |
        // | o | Stack |
        // | n |       |
        // | s |       |
        // +---+-------+
        // |  Buttons  |
        // +-----------+
        grid->addWidget(list, 0, 0);
        grid->addWidget(stack, 0, 1);
        grid->addWidget(buttons, 2, 0, 1, 2);
        break;

    case QxtConfigDialog::East:
        // +-------+---+
        // |       | I |
        // |       | c |
        // | Stack | o |
        // |       | n |
        // |       | s |
        // +-------+---+
        // |  Buttons  |
        // +-----------+
        grid->addWidget(stack, 0, 0);
        grid->addWidget(list, 0, 1);
        grid->addWidget(buttons, 2, 0, 1, 2);
        break;

    default:
        qWarning("QxtConfigDialogPrivate::relayout(): unknown position");
        break;
    }

    if (pos == QxtConfigDialog::North)
    {
        list->setFlow(QListView::LeftToRight);
        list->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    }
    else
    {
        list->setFlow(QListView::TopToBottom);
        list->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    }
    list->invalidate();

    // defrost
    grid->setEnabled(true);
}

/*!
    \class QxtConfigDialog QxtConfigDialog
    \ingroup QxtGui
    \brief A configuration dialog.

    QxtConfigDialog provides a convenient interface for building
	common configuration dialogs. QxtConfigDialog consists of a
	list of icons and a stack of pages.

	Example usage:
	\code
	QxtConfigDialog dialog;
    dialog.addPage(new ConfigurationPage(&dialog), QIcon(":/images/config.png"), tr("Configuration"));
	dialog.addPage(new UpdatePage(&dialog), QIcon(":/images/update.png"), tr("Update"));
	dialog.addPage(new QueryPage(&dialog), QIcon(":/images/query.png"), tr("Query"));
	dialog.exec();
	\endcode

    \image html qxtconfigdialog.png "QxtConfigDialog with page icons on the left (QxtConfigDialog::West)."
 */

/*!
    \enum IconPosition::IconPosition

    This enum describes the page icon position.

    \sa QxtCheckComboBox::iconPosition
 */

/*!
    \var QxtConfigDialog::IconPosition QxtConfigDialog::North

    The icons are located above the pages.
 */

/*!
    \var QxtConfigDialog::IconPosition QxtConfigDialog::West

    The icons are located to the left of the pages.
 */

/*!
    \var QxtConfigDialog::IconPosition QxtConfigDialog::East

    The icons are located to the right of the pages.
 */

/*!
    \fn QxtConfigDialog::currentIndexChanged(int index)

    This signal is emitted whenever the current page \a index changes.

    \sa currentIndex()
 */

/*!
    Constructs a new QxtConfigDialog with \a parent and \a flags.
 */
QxtConfigDialog::QxtConfigDialog(QWidget* parent, Qt::WindowFlags flags)
        : QDialog(parent, flags)
{
    QXT_INIT_PRIVATE(QxtConfigDialog);
    qxt_d().init();
}

/*!
    Constructs a new QxtConfigDialog with icon \a position, \a parent and \a flags.
 */
QxtConfigDialog::QxtConfigDialog(QxtConfigDialog::IconPosition position, QWidget* parent, Qt::WindowFlags flags)
        : QDialog(parent, flags)
{
    QXT_INIT_PRIVATE(QxtConfigDialog);
    qxt_d().init(position);
}

/*!
    Destructs the config dialog.
 */
QxtConfigDialog::~QxtConfigDialog()
{}

/*!
    \return The dialog button box.

    The default buttons are \b QDialogButtonBox::Ok and \b QDialogButtonBox::Cancel.

    \note QDialogButtonBox is available in Qt 4.2 or newer.

    \sa setDialogButtonBox()
*/
#if QT_VERSION >= 0x040200
QDialogButtonBox* QxtConfigDialog::dialogButtonBox() const
{
    return qxt_d().buttons;
}
#endif // QT_VERSION

/*!
    Sets the dialog \a buttonBox.

    \sa dialogButtonBox()
*/
#if QT_VERSION >= 0x040200
void QxtConfigDialog::setDialogButtonBox(QDialogButtonBox* buttonBox)
{
    if (qxt_d().buttons != buttonBox)
    {
        if (qxt_d().buttons && qxt_d().buttons->parent() == this)
        {
            delete qxt_d().buttons;
        }
        qxt_d().buttons = buttonBox;
        qxt_d().relayout();
    }
}
#endif // QT_VERSION

/*!
    \property QxtConfigDialog::hoverEffect
    \brief This property holds whether a hover effect is shown for page icons

    The default value is \b true.

    \note Hovered (but not selected) icons are highlighted with lightened \b QPalette::Highlight
    (whereas selected icons are highlighted with \b QPalette::Highlight). In case lightened
    \b QPalette::Highlight ends up same as \b QPalette::Base, \b QPalette::AlternateBase is used
    as a fallback color for the hover effect. This usually happens when \b QPalette::Highlight
    already is a light color (eg. light gray).
 */
bool QxtConfigDialog::hasHoverEffect() const
{
    return qxt_d().list->hasHoverEffect();
}

void QxtConfigDialog::setHoverEffect(bool enabled)
{
    qxt_d().list->setHoverEffect(enabled);
}

/*!
    \property QxtConfigDialog::iconPosition
    \brief This property holds the position of page icons
 */
QxtConfigDialog::IconPosition QxtConfigDialog::iconPosition() const
{
    return qxt_d().pos;
}

void QxtConfigDialog::setIconPosition(QxtConfigDialog::IconPosition position)
{
    if (qxt_d().pos != position)
    {
        qxt_d().pos = position;
        qxt_d().relayout();
    }
}

/*!
    \property QxtConfigDialog::iconSize
    \brief This property holds the size of page icons
 */
QSize QxtConfigDialog::iconSize() const
{
    return qxt_d().list->iconSize();
}

void QxtConfigDialog::setIconSize(const QSize& size)
{
    qxt_d().list->setIconSize(size);
}

/*!
    Adds a \a page with \a icon and \a title.

	In case \a title is an empty string, \b QWidget::windowTitle is used.

	\return The index of added page.

    \warning Adding and removing pages dynamically at run time might cause flicker.

    \sa insertPage()
*/
int QxtConfigDialog::addPage(QWidget* page, const QIcon& icon, const QString& title)
{
    return insertPage(-1, page, icon, title);
}

/*!
    Inserts a \a page with \a icon and \a title.

	In case \a title is an empty string, \b QWidget::windowTitle is used.

	\return The index of inserted page.

    \warning Inserting and removing pages dynamically at run time might cause flicker.

    \sa addPage()
*/
int QxtConfigDialog::insertPage(int index, QWidget* page, const QIcon& icon, const QString& title)
{
    if (!page)
    {
        qWarning("QxtConfigDialog::insertPage(): Attempt to insert null page");
        return -1;
    }

    index = qxt_d().stack->insertWidget(index, page);
    const QString label = !title.isEmpty() ? title : page->windowTitle();
    if (label.isEmpty())
        qWarning("QxtConfigDialog::insertPage(): Inserting a page with an empty title");
    QListWidgetItem* item = new QListWidgetItem(icon, label);
    qxt_d().list->insertItem(index, item);
    qxt_d().list->invalidate();
    return index;
}

/*!
   Removes the page at \a index.

   \note Does not delete the page widget.
*/
void QxtConfigDialog::removePage(int index)
{
    if (QWidget* page = qxt_d().stack->widget(index))
    {
        qxt_d().stack->removeWidget(page);
        delete qxt_d().list->takeItem(index);
        qxt_d().list->invalidate();
    }
    else
    {
        qWarning("QxtConfigDialog::removePage(): Unknown index");
    }
}

/*!
    \property QxtConfigDialog::count
    \brief This property holds the number of pages
*/
int QxtConfigDialog::count() const
{
    return qxt_d().stack->count();
}

/*!
    \property QxtConfigDialog::currentIndex
    \brief This property holds the index of current page
*/
int QxtConfigDialog::currentIndex() const
{
    return qxt_d().stack->currentIndex();
}

void QxtConfigDialog::setCurrentIndex(int index)
{
    qxt_d().list->setCurrentRow(index);
    qxt_d().stack->setCurrentIndex(index);
}

/*!
    \return The current page.

    \sa currentIndex(), setCurrentPage()
*/
QWidget* QxtConfigDialog::currentPage() const
{
    return qxt_d().stack->currentWidget();
}

/*!
    Sets the current \a page.

    \sa currentPage(), currentIndex()
*/
void QxtConfigDialog::setCurrentPage(QWidget* page)
{
    setCurrentIndex(qxt_d().stack->indexOf(page));
}

/*!
    \return The index of \a page or \b -1 if the page is unknown.
*/
int QxtConfigDialog::indexOf(QWidget* page) const
{
    return qxt_d().stack->indexOf(page);
}

/*!
    \return The page at \a index or \b 0 if the \a index is out of range.
*/
QWidget* QxtConfigDialog::page(int index) const
{
    return qxt_d().stack->widget(index);
}

/*!
    \return \b true if the page at \a index is enabled; otherwise \b false.

    \sa setPageEnabled(), QWidget::isEnabled()
*/
bool QxtConfigDialog::isPageEnabled(int index) const
{
    const QListWidgetItem* item = qxt_d().list->item(index);
    return (item && (item->flags() & Qt::ItemIsEnabled));
}

/*!
    Sets the page at \a index \a enabled. The corresponding
	page icon is also \a enabled.

    \sa isPageEnabled(), QWidget::setEnabled()
*/
void QxtConfigDialog::setPageEnabled(int index, bool enabled)
{
    QWidget* page = qxt_d().stack->widget(index);
    QListWidgetItem* item = qxt_d().list->item(index);
    if (page && item)
    {
        page->setEnabled(enabled);
        if (enabled)
            item->setFlags(item->flags() | Qt::ItemIsEnabled);
        else
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
    else
    {
        qWarning("QxtConfigDialog::setPageEnabled(): Unknown index");
    }
}

/*!
    \return \b true if the page at \a index is hidden; otherwise \b false.

    \sa setPageHidden(), QWidget::isVisible()
*/
bool QxtConfigDialog::isPageHidden(int index) const
{
    const QListWidgetItem* item = qxt_d().list->item(index);
#if QT_VERSION >= 0x040200
    return (item && item->isHidden());
#else // QT_VERSION
    return (item && qxt_d().list->isItemHidden(item));
#endif // QT_VERSION
}

/*!
    Sets the page at \a index \a hidden. The corresponding
	page icon is also \a hidden.

    \sa isPageHidden(), QWidget::setVisible()
*/
void QxtConfigDialog::setPageHidden(int index, bool hidden)
{
    QListWidgetItem* item = qxt_d().list->item(index);
    if (item)
    {
#if QT_VERSION >= 0x040200
        item->setHidden(hidden);
#else
        qxt_d().list->setItemHidden(item, hidden);
#endif // QT_VERSION
    }
    else
    {
        qWarning("QxtConfigDialog::setPageHidden(): Unknown index");
    }
}

/*!
    \return The icon of page at \a index.

    \sa setPageIcon()
*/
QIcon QxtConfigDialog::pageIcon(int index) const
{
    const QListWidgetItem* item = qxt_d().list->item(index);
    return (item ? item->icon() : QIcon());
}

/*!
    Sets the \a icon of page at \a index.

    \sa pageIcon()
*/
void QxtConfigDialog::setPageIcon(int index, const QIcon& icon)
{
    QListWidgetItem* item = qxt_d().list->item(index);
    if (item)
    {
        item->setIcon(icon);
    }
    else
    {
        qWarning("QxtConfigDialog::setPageIcon(): Unknown index");
    }
}

/*!
    \return The title of page at \a index.

    \sa setPageTitle()
*/
QString QxtConfigDialog::pageTitle(int index) const
{
    const QListWidgetItem* item = qxt_d().list->item(index);
    return (item ? item->text() : QString());
}

/*!
    Sets the \a title of page at \a index.

    \sa pageTitle()
*/
void QxtConfigDialog::setPageTitle(int index, const QString& title)
{
    QListWidgetItem* item = qxt_d().list->item(index);
    if (item)
    {
        item->setText(title);
    }
    else
    {
        qWarning("QxtConfigDialog::setPageTitle(): Unknown index");
    }
}

/*!
    \return The tooltip of page at \a index.

    \sa setPageToolTip()
*/
QString QxtConfigDialog::pageToolTip(int index) const
{
    const QListWidgetItem* item = qxt_d().list->item(index);
    return (item ? item->toolTip() : QString());
}

/*!
    Sets the \a tooltip of page at \a index.

    \sa pageToolTip()
*/
void QxtConfigDialog::setPageToolTip(int index, const QString& tooltip)
{
    QListWidgetItem* item = qxt_d().list->item(index);
    if (item)
    {
        item->setToolTip(tooltip);
    }
    else
    {
        qWarning("QxtConfigDialog::setPageToolTip(): Unknown index");
    }
}

/*!
    \return The what's this of page at \a index.

    \sa setPageWhatsThis()
*/
QString QxtConfigDialog::pageWhatsThis(int index) const
{
    const QListWidgetItem* item = qxt_d().list->item(index);
    return (item ? item->whatsThis() : QString());
}

/*!
    Sets the \a whatsthis of page at \a index.

    \sa pageWhatsThis()
*/
void QxtConfigDialog::setPageWhatsThis(int index, const QString& whatsthis)
{
    QListWidgetItem* item = qxt_d().list->item(index);
    if (item)
    {
        item->setWhatsThis(whatsthis);
    }
    else
    {
        qWarning("QxtConfigDialog::setPageWhatsThis(): Unknown index");
    }
}

/*!
    \return The internal list widget used for showing page icons.

    \sa stackedWidget()
*/
QListWidget* QxtConfigDialog::listWidget() const
{
    return qxt_d().list;
}

/*!
    \return The internal stacked widget used for stacking pages.

    \sa listWidget()
*/
QStackedWidget* QxtConfigDialog::stackedWidget() const
{
    return qxt_d().stack;
}
