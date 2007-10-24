#include <QtTest>
#include <QtGui>
#include <QxtGui>

class TestQxtGui : public QObject
{
	Q_OBJECT
	
private slots:
	void testQxtApplication();
	
	void testQxtCheckComboBox_data();
	void testQxtCheckComboBox();

	void testQxtConfigDialog();
	void testQxtConfirmationMessage();
	void testQxtDesktopWidget();
	void testQxtDockWidget();
	void testQxtGroupBox();
	void testQxtItemDelegate();

	void testQxtLabel();
	
	void testQxtListWidget_data();
	void testQxtListWidget();
	
	void testQxtProgressLabel();
	void testQxtProxyStyle();
	void testQxtPushButton();
	void testQxtSpanSlider();
	void testQxtStars();
	void testQxtStringSpinBox();
	
	void testQxtTableWidget_data();
	void testQxtTableWidget();
	
	void testQxtTabWidget();
	void testQxtToolTip();

	void testQxtTreeWidget_data();
	void testQxtTreeWidget();
};

void TestQxtGui::testQxtApplication()
{
	// See: test/app
}

void TestQxtGui::testQxtCheckComboBox_data()
{
	QTest::addColumn<QTestEventList>("popup");
	QTest::addColumn<QTestEventList>("select");
	QTest::addColumn<QTestEventList>("close");
	QTest::addColumn<QStringList>("expected");
	
	QTestEventList popup1;
	popup1.addKeyClick(Qt::Key_Up); // popup
	
	QTestEventList popup2;
	popup2.addKeyClick(Qt::Key_Down); // popup
	
	QTestEventList close1;
	close1.addKeyClick(Qt::Key_Escape); // close
	
	QTestEventList close2;
	close2.addKeyClick(Qt::Key_Return); // close

	QTestEventList select0;
	QStringList result0;

	QTestEventList select1;
	select1.addKeyClick(Qt::Key_Space); // select first
	select1.addKeyClick(Qt::Key_Down); // move to second
	select1.addKeyClick(Qt::Key_Down); // move to third
	select1.addKeyClick(Qt::Key_Space); // select third
	QStringList result1 = QStringList() << "1" << "3";
	
	QTestEventList select2;
	select2.addKeyClick(Qt::Key_Down); // move to second
	select2.addKeyClick(Qt::Key_Down); // move to third
	select2.addKeyClick(Qt::Key_Up); // move back to second
	select2.addKeyClick(Qt::Key_Space); // select second
	QStringList result2 = QStringList() << "2";
	
	QTest::newRow("-")   << popup1 << select0 << close2 << result0;
	QTest::newRow("1,3") << popup1 << select1 << close1 << result1;
	QTest::newRow("2")   << popup2 << select2 << close2 << result2;
}

void TestQxtGui::testQxtCheckComboBox()
{
	QFETCH(QTestEventList, popup);
	QFETCH(QTestEventList, select);
	QFETCH(QTestEventList, close);
	QFETCH(QStringList, expected);

	QxtCheckComboBox combo;
	combo.addItems(QStringList() << "1" << "2" << "3" << "4");

	QSignalSpy spy(&combo, SIGNAL(checkedItemsChanged(const QStringList&)));
	QVERIFY(spy.isValid());

	popup.simulate(&combo);
	select.simulate(combo.view());
	close.simulate(&combo);

	QCOMPARE(combo.checkedItems(), expected);

	if (!combo.checkedItems().isEmpty())
	{
		QVERIFY(spy.count() > 0);
		while (!spy.isEmpty())
		{
			QList<QVariant> arguments = spy.takeFirst();
			QVERIFY(arguments.at(0).type() == QVariant::StringList);
		}
	}
	else
	{
		QVERIFY(spy.count() == 0);
	}
}

void TestQxtGui::testQxtConfigDialog()
{
	// See: test/app, demos/configdialog
}

void TestQxtGui::testQxtConfirmationMessage()
{
	// See: test/app
}

void TestQxtGui::testQxtDesktopWidget()
{
	// See: demos/qxtsnapshot
	WId activeId = QxtDesktopWidget::activeWindow();
	QString activeTitle = QxtDesktopWidget::windowTitle(activeId);
	WId foundId = QxtDesktopWidget::findWindow(activeTitle);
	QString foundTitle = QxtDesktopWidget::windowTitle(foundId);
	QRect activeRect = QxtDesktopWidget::windowGeometry(activeId);
	WId atId = QxtDesktopWidget::windowAt(activeRect.center());
	QString atTitle = QxtDesktopWidget::windowTitle(atId);
	QVERIFY(activeId == foundId);
	QVERIFY(foundId == atId);
	QVERIFY(activeTitle == foundTitle);
	QVERIFY(foundTitle == atTitle);
}

void TestQxtGui::testQxtDockWidget()
{
	// See: demos/dockwidgets
}

void TestQxtGui::testQxtGroupBox()
{
	// See: test/app
}

void TestQxtGui::testQxtItemDelegate()
{
	// See:
	// - testQxtListWidget()
	// - testQxtTableWidget()
	// - testQxtTreeWidget()
}

void TestQxtGui::testQxtLabel()
{
	QxtLabel label("Text");
	
	QTestEventList events;
	events.addMouseClick(Qt::LeftButton);
	
	QSignalSpy clicked(&label, SIGNAL(clicked()));
	QSignalSpy textChanged(&label, SIGNAL(textChanged(const QString&)));
	QVERIFY(clicked.isValid());
	QVERIFY(textChanged.isValid());
	
	events.simulate(&label);
	label.setText("Changed");
	
	QCOMPARE(clicked.count(), 1);
	QCOMPARE(textChanged.count(), 1);
	
	QList<QVariant> arguments = clicked.takeFirst();
	QVERIFY(arguments.isEmpty());
	
	arguments = textChanged.takeFirst();
	QVERIFY(arguments.at(0).toString() == "Changed");
}

void TestQxtGui::testQxtListWidget_data()
{
	QTest::addColumn<QTestEventList>("events");
	QTest::addColumn<int>("amount");
	
	QTestEventList all;
	for (int i = 0; i < 10; ++i)
	{
		all.addKeyClick(Qt::Key_Down); // select/finish edit
		all.addKeyClick(Qt::Key_Space); // check
		all.addKeyClick(Qt::Key_F2); // start edit
	}
	all.addKeyClick(Qt::Key_Up);
	QTest::newRow("all checked") << all << 10;
	
	QTestEventList second;
	for (int i = 0; i < 10; i += 2)
	{
		second.addKeyClick(Qt::Key_Down); // select
		second.addKeyClick(Qt::Key_Space); // check
		second.addKeyClick(Qt::Key_F2); // start edit
		second.addKeyClick(Qt::Key_Escape); // finish edit
	}
	second.addKeyClick(Qt::Key_Up);
	QTest::newRow("every second checked") << second << 5;
	
	QTestEventList none;
	QTest::newRow("none checked") << none << 0;
}

void TestQxtGui::testQxtListWidget()
{
	QFETCH(QTestEventList, events);
	QFETCH(int, amount);
	
	QxtListWidget listWidget;
	listWidget.setEditTriggers(QAbstractItemView::EditKeyPressed);
	for (int i = 0; i < 10; ++i)
	{
		QxtListWidgetItem* item = new QxtListWidgetItem(QString::number(i), &listWidget);
		item->setFlag(Qt::ItemIsUserCheckable);
		item->setFlag(Qt::ItemIsEditable);
		item->setCheckState(Qt::Unchecked);
	}
	
	qRegisterMetaType<QListWidgetItem*>("QListWidgetItem*");
	qRegisterMetaType<QxtListWidgetItem*>("QxtListWidgetItem*");
	QSignalSpy editStarted(&listWidget, SIGNAL(itemEditingStarted(QListWidgetItem*)));
	QSignalSpy editFinished(&listWidget, SIGNAL(itemEditingFinished(QListWidgetItem*)));
	QSignalSpy checkChanged(&listWidget, SIGNAL(itemCheckStateChanged(QxtListWidgetItem*)));
	QVERIFY(editStarted.isValid());
	QVERIFY(editFinished.isValid());
	QVERIFY(checkChanged.isValid());
	
	events.simulate(listWidget.viewport());
	
	QList<QListWidgetItem*> checkedItems;
	for (int i = 0; i < 10; ++i)
	{
		QListWidgetItem* item = listWidget.item(i);
		if (item && item->data(Qt::CheckStateRole).toInt() == Qt::Checked)
			checkedItems += item;
	}
	
	QCOMPARE(checkChanged.count(), checkedItems.count());
	QCOMPARE(editStarted.count(),  amount);
	QCOMPARE(editFinished.count(), amount);
}

void TestQxtGui::testQxtProgressLabel()
{
	// See: test/app
}

void TestQxtGui::testQxtProxyStyle()
{
	// Nothing to test
}

void TestQxtGui::testQxtPushButton()
{
	// See: test/app
}

void TestQxtGui::testQxtStars()
{
	// See: test/app
}

void TestQxtGui::testQxtSpanSlider()
{
	QxtSpanSlider slider;
	slider.setRange(0, 99);
	
	QSignalSpy spanChanged(&slider, SIGNAL(spanChanged(int, int)));
	QSignalSpy lowerChanged(&slider, SIGNAL(lowerValueChanged(int)));
	QSignalSpy upperChanged(&slider, SIGNAL(upperValueChanged(int)));
	QVERIFY(spanChanged.isValid());
	QVERIFY(lowerChanged.isValid());
	QVERIFY(upperChanged.isValid());
	
	// #1 setSpan() - basic change
	slider.setSpan(4, 75);
	QCOMPARE(slider.lowerValue(), 4);
	QCOMPARE(slider.upperValue(), 75);
	QCOMPARE(spanChanged.count(), 1);
	QCOMPARE(lowerChanged.count(), 1);
	QCOMPARE(upperChanged.count(), 1);
	QList<QVariant> args = spanChanged.takeLast();
	QVERIFY(args.at(0).toInt() == 4);
	QVERIFY(args.at(1).toInt() == 75);
	QVERIFY(lowerChanged.takeLast().at(0).toInt() == 4);
	QVERIFY(upperChanged.takeLast().at(0).toInt() == 75);
	
	// #2 setSpan() - no change
	slider.setSpan(75, 4);
	QCOMPARE(slider.lowerValue(), 4);
	QCOMPARE(slider.upperValue(), 75);
	QCOMPARE(spanChanged.count(), 0);
	QCOMPARE(lowerChanged.count(), 0);
	QCOMPARE(upperChanged.count(), 0);
	
	// #3 setSpan() - inverse span
	slider.setSpan(66, 33);
	QCOMPARE(slider.lowerValue(), 33);
	QCOMPARE(slider.upperValue(), 66);
	QCOMPARE(spanChanged.count(), 1);
	QCOMPARE(lowerChanged.count(), 1);
	QCOMPARE(upperChanged.count(), 1);
	args = spanChanged.takeLast();
	QVERIFY(args.at(0).toInt() == 33);
	QVERIFY(args.at(1).toInt() == 66);
	QVERIFY(lowerChanged.takeLast().at(0).toInt() == 33);
	QVERIFY(upperChanged.takeLast().at(0).toInt() == 66);
	
	// #4 setSpan() - keep span in range
	slider.setSpan(-400, 400);
	QCOMPARE(slider.lowerValue(), 0);
	QCOMPARE(slider.upperValue(), 99);
	QCOMPARE(spanChanged.count(), 1);
	QCOMPARE(lowerChanged.count(), 1);
	QCOMPARE(upperChanged.count(), 1);
	args = spanChanged.takeLast();
	QVERIFY(args.at(0).toInt() == 0);
	QVERIFY(args.at(1).toInt() == 99);
	QVERIFY(lowerChanged.takeLast().at(0).toInt() == 0);
	QVERIFY(upperChanged.takeLast().at(0).toInt() == 99);
	
	// #5 setLowerValue() - basic change
	slider.setLowerValue(3);
	QCOMPARE(slider.lowerValue(), 3);
	QCOMPARE(slider.upperValue(), 99);
	QCOMPARE(spanChanged.count(), 1);
	QCOMPARE(lowerChanged.count(), 1);
	QCOMPARE(upperChanged.count(), 0);
	args = spanChanged.takeLast();
	QVERIFY(args.at(0).toInt() == 3);
	QVERIFY(args.at(1).toInt() == 99);
	QVERIFY(lowerChanged.takeLast().at(0).toInt() == 3);
	
	// #6 setLowerValue() - no change
	slider.setLowerValue(3);
	QCOMPARE(slider.lowerValue(), 3);
	QCOMPARE(slider.upperValue(), 99);
	QCOMPARE(spanChanged.count(), 0);
	QCOMPARE(lowerChanged.count(), 0);
	QCOMPARE(upperChanged.count(), 0);
	
	// #7 setLowerValue() - keep span in range
	slider.setLowerValue(-3);
	QCOMPARE(slider.lowerValue(), 0);
	QCOMPARE(slider.upperValue(), 99);
	QCOMPARE(spanChanged.count(), 1);
	QCOMPARE(lowerChanged.count(), 1);
	QCOMPARE(upperChanged.count(), 0);
	args = spanChanged.takeLast();
	QVERIFY(args.at(0).toInt() == 0);
	QVERIFY(args.at(1).toInt() == 99);
	QVERIFY(lowerChanged.takeLast().at(0).toInt() == 0);
	
	// #8 setUpperValue() - basic change
	slider.setUpperValue(77);
	QCOMPARE(slider.lowerValue(), 0);
	QCOMPARE(slider.upperValue(), 77);
	QCOMPARE(spanChanged.count(), 1);
	QCOMPARE(lowerChanged.count(), 0);
	QCOMPARE(upperChanged.count(), 1);
	args = spanChanged.takeLast();
	QVERIFY(args.at(0).toInt() == 0);
	QVERIFY(args.at(1).toInt() == 77);
	QVERIFY(upperChanged.takeLast().at(0).toInt() == 77);
	
	// #9 setUpperValue() - no change
	slider.setUpperValue(77);
	QCOMPARE(slider.lowerValue(), 0);
	QCOMPARE(slider.upperValue(), 77);
	QCOMPARE(spanChanged.count(), 0);
	QCOMPARE(lowerChanged.count(), 0);
	QCOMPARE(upperChanged.count(), 0);
	
	// #10 setUpperValue() - keep span in range
	slider.setUpperValue(111);
	QCOMPARE(slider.lowerValue(), 0);
	QCOMPARE(slider.upperValue(), 99);
	QCOMPARE(spanChanged.count(), 1);
	QCOMPARE(lowerChanged.count(), 0);
	QCOMPARE(upperChanged.count(), 1);
	args = spanChanged.takeLast();
	QVERIFY(args.at(0).toInt() == 0);
	QVERIFY(args.at(1).toInt() == 99);
	QVERIFY(upperChanged.takeLast().at(0).toInt() == 99);
	
	// #11 setLowerValue(), setUpperValue() - inverse span
	slider.setLowerValue(66); // a: lower->66,upper=99
	slider.setUpperValue(33); // b: lower->33,upper->66
	slider.setLowerValue(77); // c: lower->66,upper->77
	QCOMPARE(spanChanged.count(), 3);
	QCOMPARE(lowerChanged.count(), 3);
	QCOMPARE(upperChanged.count(), 2);
	// a
	args = spanChanged.takeFirst();
	QVERIFY(args.at(0).toInt() == 66);
	QVERIFY(args.at(1).toInt() == 99);
	QVERIFY(lowerChanged.takeFirst().at(0).toInt() == 66);
	// b
	args = spanChanged.takeFirst();
	QVERIFY(args.at(0).toInt() == 33);
	QVERIFY(args.at(1).toInt() == 66);
	QVERIFY(lowerChanged.takeFirst().at(0).toInt() == 33);
	QVERIFY(upperChanged.takeFirst().at(0).toInt() == 66);
	// c
	args = spanChanged.takeFirst();
	QVERIFY(args.at(0).toInt() == 66);
	QVERIFY(args.at(1).toInt() == 77);
	QVERIFY(lowerChanged.takeFirst().at(0).toInt() == 66);
	QVERIFY(upperChanged.takeFirst().at(0).toInt() == 77);
	
	// # 12 change of range
	slider.setRange(68, 72);
	QCOMPARE(slider.lowerValue(), 68);
	QCOMPARE(slider.upperValue(), 72);
	QCOMPARE(spanChanged.count(), 1);
	QCOMPARE(lowerChanged.count(), 1);
	QCOMPARE(upperChanged.count(), 1);
	args = spanChanged.takeLast();
	QVERIFY(args.at(0).toInt() == 68);
	QVERIFY(args.at(1).toInt() == 72);
	QVERIFY(lowerChanged.takeLast().at(0).toInt() == 68);
	QVERIFY(upperChanged.takeLast().at(0).toInt() == 72);
}

void TestQxtGui::testQxtStringSpinBox()
{
	QStringList strings;
	for (int i = 0; i < 10; ++i)
		strings += QString::number(i);
	
	QxtStringSpinBox spinBox;
	spinBox.setStrings(strings);
	
	QTestEventList up;
	up.addKeyClick(Qt::Key_Up);
	
	QTestEventList down;
	down.addKeyClick(Qt::Key_Down);
	
	for (int i = 0; i < 10; ++i)
	{
		QCOMPARE(spinBox.cleanText(), QString::number(i));
		up.simulate(&spinBox);
		QCOMPARE(spinBox.cleanText(), QString::number(qMin(i+1, 9)));
	}
	
	for (int i = 9; i >= 0; --i)
	{
		QCOMPARE(spinBox.cleanText(), QString::number(i));
		down.simulate(&spinBox);
		QCOMPARE(spinBox.cleanText(), QString::number(qMax(i-1, 0)));
	}
}

void TestQxtGui::testQxtTableWidget_data()
{
	QTest::addColumn<QTestEventList>("events");
	QTest::addColumn<int>("amount");
	
	QTestEventList all;
	all.addKeyClick(Qt::Key_Tab); // select first
	all.addKeyClick(Qt::Key_F2); // start editing
	all.addKeyClick(Qt::Key_Tab); // select second
	all.addKeyClick(Qt::Key_F2); // start editing
	all.addKeyClick(Qt::Key_Tab); // select third
	all.addKeyClick(Qt::Key_F2); // start editing
	all.addKeyClick(Qt::Key_Tab); // select fourth
	all.addKeyClick(Qt::Key_F2); // start editing
	all.addKeyClick(Qt::Key_Tab); // finish editing
	QTest::newRow("all edited") << all << 4;
	
	QTestEventList second;
	second.addKeyClick(Qt::Key_Tab); // select second
	second.addKeyClick(Qt::Key_Tab);
	second.addKeyClick(Qt::Key_F2); // edit
	second.addKeyClick(Qt::Key_Tab); // select fourth
	second.addKeyClick(Qt::Key_Tab);
	second.addKeyClick(Qt::Key_F2); // edit
	second.addKeyClick(Qt::Key_Tab); // abort
	QTest::newRow("every second edited") << second << 2;
	
	QTestEventList none;
	QTest::newRow("none edited") << none << 0;
}

void TestQxtGui::testQxtTableWidget()
{
	QFETCH(QTestEventList, events);
	QFETCH(int, amount);
	
	QxtTableWidget tableWidget(2, 2);
	tableWidget.setEditTriggers(QAbstractItemView::EditKeyPressed);
	
	qRegisterMetaType<QTableWidgetItem*>("QTableWidgetItem*");
	QSignalSpy editStarted(&tableWidget, SIGNAL(itemEditingStarted(QTableWidgetItem*)));
	QSignalSpy editFinished(&tableWidget, SIGNAL(itemEditingFinished(QTableWidgetItem*)));
	QVERIFY(editStarted.isValid());
	QVERIFY(editFinished.isValid());
	
	events.simulate(tableWidget.viewport());
	
	QCOMPARE(editStarted.count(),  amount);
	QCOMPARE(editFinished.count(), amount);
}

void TestQxtGui::testQxtTabWidget()
{
	QxtTabWidget tabWidget;
	tabWidget.setTabContextMenuPolicy(Qt::ActionsContextMenu);
	tabWidget.addTab(new QLabel("1"), "1");
	tabWidget.addTab(new QLabel("2"), "2");
	tabWidget.addTab(new QLabel("3"), "3");
	
	QPointer<QAction> act1a = new QAction("1a", &tabWidget);
	QPointer<QAction> act1b = new QAction("1b", &tabWidget);
	QPointer<QAction> act2 = new QAction("2", &tabWidget);
	
	tabWidget.addTabAction(1, act2);
	QCOMPARE(tabWidget.tabActions(0).count(), 0);
	QCOMPARE(tabWidget.tabActions(1).count(), 1);
	QCOMPARE(tabWidget.tabActions(2).count(), 0);
	
	QPointer<QAction> act3 = tabWidget.addTabAction(2, "3");
	QCOMPARE(tabWidget.tabActions(0).count(), 0);
	QCOMPARE(tabWidget.tabActions(1).count(), 1);
	QCOMPARE(tabWidget.tabActions(2).count(), 1);
	
	QList<QAction*> actions;
	actions << act1a << act1b;
	tabWidget.addTabActions(0, actions);
	QCOMPARE(tabWidget.tabActions(0).count(), 2);
	QCOMPARE(tabWidget.tabActions(1).count(), 1);
	QCOMPARE(tabWidget.tabActions(2).count(), 1);
	
	tabWidget.clearTabActions(0);
	QCOMPARE(tabWidget.tabActions(0).count(), 0);
	QCOMPARE(tabWidget.tabActions(1).count(), 1);
	QCOMPARE(tabWidget.tabActions(2).count(), 1);
	QVERIFY(act1a == 0 && act1b == 0); // must have been deleted
	
	tabWidget.removeTabAction(1, act2);
	QCOMPARE(tabWidget.tabActions(0).count(), 0);
	QCOMPARE(tabWidget.tabActions(1).count(), 0);
	QCOMPARE(tabWidget.tabActions(2).count(), 1);
	QVERIFY(act2 != 0); // must not have been deleted
	
	tabWidget.insertTabAction(2, act3, act2);
	QCOMPARE(tabWidget.tabActions(0).count(), 0);
	QCOMPARE(tabWidget.tabActions(1).count(), 0);
	QCOMPARE(tabWidget.tabActions(2).count(), 2);
	QVERIFY(tabWidget.tabActions(2).first() == act2);
	QVERIFY(tabWidget.tabActions(2).last() == act3);
}

void TestQxtGui::testQxtToolTip()
{
	// See demos/qxtsnapshot
}

void TestQxtGui::testQxtTreeWidget_data()
{
	QTest::addColumn<QTestEventList>("events");
	QTest::addColumn<int>("amount");
	
	QTestEventList all;
	for (int i = 0; i < 10; ++i)
	{
		all.addKeyClick(Qt::Key_Down); // select next
		all.addKeyClick(Qt::Key_Space); // check
		all.addKeyClick(Qt::Key_F2); // start editing
	}
	all.addKeyClick(Qt::Key_Up);
	QTest::newRow("all checked") << all << 10;
	
	QTestEventList second;
	for (int i = 0; i < 10; i += 2)
	{
		second.addKeyClick(Qt::Key_Down); // select
		second.addKeyClick(Qt::Key_Space); // check
		second.addKeyClick(Qt::Key_F2); // start edit
		second.addKeyClick(Qt::Key_Escape); // finish edit
	}
	second.addKeyClick(Qt::Key_Up);
	QTest::newRow("every second checked") << second << 5;
	
	QTestEventList none;
	QTest::newRow("none checked") << none << 0;
}

void TestQxtGui::testQxtTreeWidget()
{
	QFETCH(QTestEventList, events);
	QFETCH(int, amount);
	
	QxtTreeWidget treeWidget;
	treeWidget.setColumnCount(1);
	treeWidget.setEditTriggers(QAbstractItemView::EditKeyPressed);
	for (int i = 0; i < 10; ++i)
	{
		QxtTreeWidgetItem* item = new QxtTreeWidgetItem(&treeWidget, QStringList(QString::number(i)));
		item->setFlag(Qt::ItemIsEditable);
		item->setFlag(Qt::ItemIsUserCheckable);
		item->setCheckState(0, Qt::Unchecked);
	}

	qRegisterMetaType<QTreeWidgetItem*>("QTreeWidgetItem*");
	qRegisterMetaType<QxtTreeWidgetItem*>("QxtTreeWidgetItem*");
	QSignalSpy editStarted(&treeWidget, SIGNAL(itemEditingStarted(QTreeWidgetItem*)));
	QSignalSpy editFinished(&treeWidget, SIGNAL(itemEditingFinished(QTreeWidgetItem*)));
	QSignalSpy checkChanged(&treeWidget, SIGNAL(itemCheckStateChanged(QxtTreeWidgetItem*)));
	QVERIFY(editStarted.isValid());
	QVERIFY(editFinished.isValid());
	QVERIFY(checkChanged.isValid());
	
	events.simulate(treeWidget.viewport());
	
	QList<QTreeWidgetItem*> checkedItems;
	for (int i = 0; i < 10; ++i)
	{
		QTreeWidgetItem* item = treeWidget.topLevelItem(i);
		if (item && item->data(0, Qt::CheckStateRole).toInt() == Qt::Checked)
			checkedItems += item;
	}
	
	QCOMPARE(checkChanged.count(), checkedItems.count());
	QCOMPARE(editStarted.count(),  amount);
	QCOMPARE(editFinished.count(), amount);
}

QTEST_MAIN(TestQxtGui)
#include "main.moc"
