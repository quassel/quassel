#ifndef TAB_H
#define TAB_H

#include "ui_tab.h"

class Tab : public QWidget
{
	Q_OBJECT
	
public:
	Tab(QWidget* parent = 0);
	~Tab();
	
signals:
	void somethingHappened(const QString& what);
	
private slots:
	void on_qxtCheckComboBox_checkedItemsChanged(const QStringList& items);
	void on_qxtGroupBox_toggled(bool on);
	void on_qxtLabel_clicked();
	void on_qxtListWidget_itemEditingStarted(QListWidgetItem* item);
	void on_qxtListWidget_itemEditingFinished(QListWidgetItem* item);
	void on_qxtListWidget_itemCheckStateChanged(QxtListWidgetItem* item);
	void on_qxtSpanSliderHor_spanChanged(int lower, int upper);
	void on_qxtSpanSliderVer_lowerValueChanged(int value);
	void on_qxtSpanSliderVer_upperValueChanged(int value);
	void on_qxtStarsHor_valueChanged(int value);
	void on_qxtStarsVer_valueChanged(int value);
	void on_qxtStringSpinBox_valueChanged(const QString& value);
	void on_qxtTableWidget_itemEditingStarted(QTableWidgetItem* item);
	void on_qxtTableWidget_itemEditingFinished(QTableWidgetItem* item);
	void on_qxtTableWidget_itemCheckStateChanged(QxtTableWidgetItem* item);
	void on_qxtTreeWidget_itemEditingStarted(QTreeWidgetItem* item);
	void on_qxtTreeWidget_itemEditingFinished(QTreeWidgetItem* item);
	void on_qxtTreeWidget_itemCheckStateChanged(QxtTreeWidgetItem* item);
	
private:
	void fillItemViews();
	Ui::Tab ui;
};

#endif // TAB_H
