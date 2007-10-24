#include "mainwindow.h"
#include "tab.h"
#include <QxtConfirmationMessage>
#include <QxtProgressLabel>
#include <QxtConfigDialog>
#include <QxtApplication>
#include <QProgressBar>
#include <QMessageBox>
#include <QListWidget>
#include <QCloseEvent>
#include <QTreeView>
#include <QDirModel>
#if QT_VERSION >= 0x040200
#include <QCalendarWidget>
#include <QTimeLine>
#endif // QT_VERSION

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
	createProgressBar();
	ui.tabWidget->setTabContextMenuPolicy(Qt::ActionsContextMenu);
	
	connect(ui.actionQuit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionAddTab, SIGNAL(triggered()), this, SLOT(addTab()));
	connect(ui.actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	connect(ui.actionAboutQxtGui, SIGNAL(triggered()), this, SLOT(aboutQxtGui()));
	connect(ui.actionSwitchLayoutDirection, SIGNAL(triggered()), this, SLOT(switchLayoutDirection()));
	connect(ui.actionConfigure, SIGNAL(triggered()), this, SLOT(configure()));
	
	if (!qxtApp->addHotKey(Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier, Qt::Key_S, this, "toggleVisibility"))
		ui.labelVisibility->hide();
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent* event)
{
#if QT_VERSION >= 0x040200
	static const QString title("QxtConfirmationMessage");
	static const QString text(tr("Are you sure you want to quit?"));
	if (QxtConfirmationMessage::confirm(this, title, text) == QMessageBox::No)
		event->ignore();
#endif // QT_VERSION
}

void MainWindow::aboutQxtGui()
{
	QMessageBox::information(this, tr("About QxtGui"),
		tr("<h3>About QxtGui</h3>"
		"<p>QxtGui is part of Qxt, the Qt eXTension library &lt;"
		"<a href=\"http://libqxt.sf.net\">http://libqxt.sf.net</a>&gt;.</p>"));
}

void MainWindow::addTab()
{
	Tab* tab = new Tab(ui.tabWidget);
	ui.tabWidget->addTab(tab, tr("Tab %1").arg(ui.tabWidget->count() + 1));
	QAction* act = ui.tabWidget->addTabAction(ui.tabWidget->indexOf(tab), tr("Close"), tab, SLOT(close()), tr("Ctrl+W"));
	tab->addAction(act);
	ui.tabWidget->setCurrentWidget(tab);
	connect(tab, SIGNAL(somethingHappened(const QString&)), statusBar(), SLOT(showMessage(const QString&)));
}

void MainWindow::switchLayoutDirection()
{
	qApp->setLayoutDirection(layoutDirection() == Qt::LeftToRight ? Qt::RightToLeft : Qt::LeftToRight);
}

void MainWindow::toggleVisibility()
{
	setVisible(!isVisible());
}

void MainWindow::createProgressBar()
{
	QxtProgressLabel* label = new QxtProgressLabel(statusBar());
	
	QProgressBar* bar = new QProgressBar(statusBar());
	bar->setMaximumWidth(label->sizeHint().width() * 2);
	bar->setRange(0, 120);
	
#if QT_VERSION >= 0x040200
	QTimeLine* timeLine = new QTimeLine(120000, this);
	timeLine->setFrameRange(0, 120);
	
	connect(timeLine, SIGNAL(frameChanged(int)), bar, SLOT(setValue(int)));
	connect(timeLine, SIGNAL(finished()), label, SLOT(restart()));
	connect(bar, SIGNAL(valueChanged(int)), label, SLOT(setValue(int)));
	timeLine->start();
#endif // QT_VERSION
	
	statusBar()->addPermanentWidget(bar);
	statusBar()->addPermanentWidget(label);
}

void MainWindow::configure()
{
	QxtConfigDialog dialog(this);
	dialog.setWindowTitle(tr("QxtConfigDialog"));
	QTreeView* page2 = new QTreeView(&dialog);
	page2->setModel(new QDirModel(page2));
	QListWidget* page3 = new QListWidget(&dialog);
	for (int i = 0; i < 100; ++i)
		page3->addItem(QString::number(i));
	dialog.addPage(page2, QIcon(":tree.png"), "A directory tree");
	dialog.addPage(page3, QIcon(":list.png"), "Some kind of list");
#if QT_VERSION >= 0x040200
	QCalendarWidget* page1 = new QCalendarWidget(&dialog);
	dialog.addPage(page1, QIcon(":calendar.png"), "Calendar");
#endif
	dialog.exec();
}
