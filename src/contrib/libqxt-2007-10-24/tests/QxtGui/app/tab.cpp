#include "tab.h"
#include <QHeaderView>
#include <QxtItemDelegate>

Tab::Tab(QWidget* parent) : QWidget(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	fillItemViews();
}

Tab::~Tab()
{
}

void Tab::on_qxtCheckComboBox_checkedItemsChanged(const QStringList& items)
{
	QString what = QString("QxtCheckComboBox::checkedItemsChanged(%1)");
	what = what.arg(items.join(ui.qxtCheckComboBox->separator()));
	emit somethingHappened(what);
}

void Tab::on_qxtGroupBox_toggled(bool on)
{
	QString what = QString("QxtGroupBox::toggled(%1)").arg(on ? tr("true") : tr("false"));
	emit somethingHappened(what);
}

void Tab::on_qxtLabel_clicked()
{
	emit somethingHappened("QxtLabel::clicked()");
}

void Tab::on_qxtListWidget_itemEditingStarted(QListWidgetItem* item)
{
	QString what = QString("QxtListWidget::itemEditingStarted(%1)").arg(item->text());
	emit somethingHappened(what);
}

void Tab::on_qxtListWidget_itemEditingFinished(QListWidgetItem* item)
{
	QString what = QString("QxtListWidget::itemEditingFinished(%1)").arg(item->text());
	emit somethingHappened(what);
}

void Tab::on_qxtListWidget_itemCheckStateChanged(QxtListWidgetItem* item)
{
	QString what = QString("QxtListWidget::itemCheckStateChanged(%1, %2)");
	what = what.arg(item->text()).arg(item->checkState() == Qt::Unchecked ? "Qt::Unchecked" : "Qt::Checked");
	emit somethingHappened(what);
}

void Tab::on_qxtSpanSliderHor_spanChanged(int lower, int upper)
{
	QString what = QString("QxtSpanSlider::spanChanged(%1,%2)").arg(lower).arg(upper);
	emit somethingHappened(what);
}

void Tab::on_qxtSpanSliderVer_lowerValueChanged(int value)
{
	QString what = QString("QxtSpanSlider::lowerValueChanged(%1)").arg(value);
	emit somethingHappened(what);
}

void Tab::on_qxtSpanSliderVer_upperValueChanged(int value)
{
	QString what = QString("QxtSpanSlider::upperValueChanged(%1)").arg(value);
	emit somethingHappened(what);
}

void Tab::on_qxtStarsHor_valueChanged(int value)
{
	QString what = QString("QxtStars::valueChanged(%1)").arg(value);
	emit somethingHappened(what);
}

void Tab::on_qxtStarsVer_valueChanged(int value)
{
	QString what = QString("QxtStars::valueChanged(%1)").arg(value);
	emit somethingHappened(what);
}

void Tab::on_qxtStringSpinBox_valueChanged(const QString& value)
{
	QString what = QString("QxtStringSpinBox::valueChanged(%1)").arg(value);
	emit somethingHappened(what);
}

void Tab::on_qxtTableWidget_itemEditingStarted(QTableWidgetItem* item)
{
#if QT_VERSION >= 0x040200
	int row = item->row();
	int col = item->column();
#else // QT_VERSION < 0x040200
	int row = item->tableWidget()->row(item);
	int col = item->tableWidget()->column(item);
#endif // QT_VERSION
	QString what = QString("QxtTableWidget::itemEditingStarted(%1,%2)").arg(row).arg(col);
	emit somethingHappened(what);
}

void Tab::on_qxtTableWidget_itemEditingFinished(QTableWidgetItem* item)
{
#if QT_VERSION >= 0x040200
	int row = item->row();
	int col = item->column();
#else // QT_VERSION < 0x040200
	int row = item->tableWidget()->row(item);
	int col = item->tableWidget()->column(item);
#endif // QT_VERSION
	QString what = QString("QxtTableWidget::itemEditingFinished(%1,%2)").arg(row).arg(col);
	emit somethingHappened(what);
}

void Tab::on_qxtTableWidget_itemCheckStateChanged(QxtTableWidgetItem* item)
{
#if QT_VERSION >= 0x040200
	int row = item->row();
	int col = item->column();
#else // QT_VERSION < 0x040200
	int row = item->tableWidget()->row(item);
	int col = item->tableWidget()->column(item);
#endif // QT_VERSION
	QString what = QString("QxtTableWidget::itemCheckStateChanged(%1, %2, %3)").arg(row).arg(col);
	what = what.arg(item->checkState() == Qt::Unchecked ? "Qt::Unchecked" : "Qt::Checked");
	emit somethingHappened(what);
}

void Tab::on_qxtTreeWidget_itemEditingStarted(QTreeWidgetItem* item)
{
	QString what = QString("QxtTreeWidget::itemEditingStarted(%1)").arg(item->text(0));
	emit somethingHappened(what);
}

void Tab::on_qxtTreeWidget_itemEditingFinished(QTreeWidgetItem* item)
{
	QString what = QString("QxtTreeWidget::itemEditingFinished(%1)").arg(item->text(0));
	emit somethingHappened(what);
}

void Tab::on_qxtTreeWidget_itemCheckStateChanged(QxtTreeWidgetItem* item)
{
	QString what = QString("QxtTreeWidget::itemCheckStateChanged(%1, %2)");
	what = what.arg(item->text(0)).arg(item->checkState(0) == Qt::Unchecked ? "Qt::Unchecked" : "Qt::Checked");
	emit somethingHappened(what);
}

void Tab::fillItemViews()
{
	ui.qxtTreeWidget->header()->hide();
#if QT_VERSION >= 0x040200
	ui.qxtTreeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);
#endif
	QxtTreeWidgetItem* treeItem = new QxtTreeWidgetItem(ui.qxtTreeWidget, QStringList() << tr("Phasellus"));
	treeItem = new QxtTreeWidgetItem(treeItem, QStringList() << tr("Faucibus"));
	treeItem->setFlag(Qt::ItemIsUserCheckable);
	treeItem->setFlag(Qt::ItemIsEditable);
	treeItem->setCheckState(0, Qt::Unchecked);
	treeItem->setData(1, QxtItemDelegate::ProgressRole, 75);
	treeItem = new QxtTreeWidgetItem(ui.qxtTreeWidget, QStringList() << tr("Curabitur"));
	treeItem = new QxtTreeWidgetItem(treeItem, QStringList() << tr("Mauris"));
	treeItem->setFlag(Qt::ItemIsUserCheckable);
	treeItem->setFlag(Qt::ItemIsEditable);
	treeItem->setCheckState(0, Qt::Unchecked);
	treeItem->setData(1, QxtItemDelegate::ProgressRole, 98);
	treeItem = new QxtTreeWidgetItem(ui.qxtTreeWidget, QStringList() << tr("Quisque"));
	treeItem = new QxtTreeWidgetItem(treeItem, QStringList() << tr("Vestibulum"));
	treeItem->setFlag(Qt::ItemIsUserCheckable);
	treeItem->setFlag(Qt::ItemIsEditable);
	treeItem->setCheckState(0, Qt::Unchecked);
	treeItem->setData(1, QxtItemDelegate::ProgressRole, 0);
	treeItem = new QxtTreeWidgetItem(treeItem, QStringList() << tr("Pellentesque"));
	treeItem->setFlag(Qt::ItemIsUserCheckable);
	treeItem->setFlag(Qt::ItemIsEditable);
	treeItem->setCheckState(0, Qt::Unchecked);
	treeItem->setData(1, QxtItemDelegate::ProgressRole, 99);
	
	QxtListWidgetItem* listItem = new QxtListWidgetItem(tr("Phasellus"), ui.qxtListWidget);
	listItem->setFlag(Qt::ItemIsUserCheckable);
	listItem->setFlag(Qt::ItemIsEditable);
	listItem->setCheckState(Qt::Unchecked);
	listItem = new QxtListWidgetItem(tr("Faucibus"), ui.qxtListWidget);
	listItem->setFlag(Qt::ItemIsUserCheckable);
	listItem->setFlag(Qt::ItemIsEditable);
	listItem->setCheckState(Qt::Unchecked);
	listItem = new QxtListWidgetItem(tr("Curabitur"), ui.qxtListWidget);
	listItem->setFlag(Qt::ItemIsUserCheckable);
	listItem->setFlag(Qt::ItemIsEditable);
	listItem->setCheckState(Qt::Unchecked);
	listItem = new QxtListWidgetItem(tr("Mauris"), ui.qxtListWidget);
	listItem->setFlag(Qt::ItemIsUserCheckable);
	listItem->setFlag(Qt::ItemIsEditable);
	listItem->setCheckState(Qt::Unchecked);
	listItem = new QxtListWidgetItem(tr("Quisque"), ui.qxtListWidget);
	listItem->setFlag(Qt::ItemIsUserCheckable);
	listItem->setFlag(Qt::ItemIsEditable);
	listItem->setCheckState(Qt::Unchecked);
	listItem = new QxtListWidgetItem(tr("Vestibulum"), ui.qxtListWidget);
	listItem->setFlag(Qt::ItemIsUserCheckable);
	listItem->setFlag(Qt::ItemIsEditable);
	listItem->setCheckState(Qt::Unchecked);
	listItem = new QxtListWidgetItem(tr("Pellentesque"), ui.qxtListWidget);
	listItem->setFlag(Qt::ItemIsUserCheckable);
	listItem->setFlag(Qt::ItemIsEditable);
	listItem->setCheckState(Qt::Unchecked);
	
	ui.qxtTableWidget->setColumnCount(2);
	ui.qxtTableWidget->setRowCount(3);
	QxtTableWidgetItem* tableItem = new QxtTableWidgetItem(tr("Phasellus"));
	tableItem->setFlag(Qt::ItemIsUserCheckable);
	tableItem->setFlag(Qt::ItemIsEditable);
	tableItem->setCheckState(Qt::Unchecked);
	ui.qxtTableWidget->setItem(0, 0, tableItem);
	tableItem = new QxtTableWidgetItem(tr("Faucibus"));
	tableItem->setFlag(Qt::ItemIsUserCheckable);
	tableItem->setFlag(Qt::ItemIsEditable);
	tableItem->setCheckState(Qt::Unchecked);
	ui.qxtTableWidget->setItem(0, 1, tableItem);
	tableItem = new QxtTableWidgetItem(tr("Curabitur"));
	tableItem->setFlag(Qt::ItemIsUserCheckable);
	tableItem->setFlag(Qt::ItemIsEditable);
	tableItem->setCheckState(Qt::Unchecked);
	ui.qxtTableWidget->setItem(1, 0, tableItem);
	tableItem = new QxtTableWidgetItem(tr("Mauris"));
	tableItem->setFlag(Qt::ItemIsUserCheckable);
	tableItem->setFlag(Qt::ItemIsEditable);
	tableItem->setCheckState(Qt::Unchecked);
	ui.qxtTableWidget->setItem(1, 1, tableItem);
	tableItem = new QxtTableWidgetItem(tr("Quisque"));
	tableItem->setFlag(Qt::ItemIsUserCheckable);
	tableItem->setFlag(Qt::ItemIsEditable);
	tableItem->setCheckState(Qt::Unchecked);
	ui.qxtTableWidget->setItem(2, 0, tableItem);
	tableItem = new QxtTableWidgetItem(tr("Vestibulum"));
	tableItem->setFlag(Qt::ItemIsUserCheckable);
	tableItem->setFlag(Qt::ItemIsEditable);
	tableItem->setCheckState(Qt::Unchecked);
	ui.qxtTableWidget->setItem(2, 1, tableItem);
}
