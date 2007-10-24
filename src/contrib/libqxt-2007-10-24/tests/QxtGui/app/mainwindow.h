#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent* event);
	
private slots:
	void aboutQxtGui();
	void addTab();
	void switchLayoutDirection();
	void toggleVisibility();
	void configure();
	
private:
	void createProgressBar();
	Ui::MainWindow ui;
};

#endif // MAINWINDOW_H
