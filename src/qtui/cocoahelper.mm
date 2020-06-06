/* 
 * Copyright (c) 2018-2020 Laurent Cimon <laurent@nilio.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QVector>
#include <QMenu>
#include <QMenuBar>

class CocoaHelper {
	public:
		static void configure(long winId = -1);
		static QMenu *makeEditMenu(QMenuBar *, QObject *);
		static void setDarkTitlebar(bool);
		static bool getDarkTitlebar(void);
};

#include <Cocoa/Cocoa.h>
#include <QGuiApplication>
#include <QWindow>

#include "cocoahelper.h"

static bool darkTitlebarSetting = 1;
static void initNativeConfig(NSWindow *nw);
static void setTitlebarColour(NSWindow *nw);
static QVector<long> getWinIds();

void
CocoaHelper::configure(long winId)
{
	QVector<long> winIds;

	if(winId == -1) {
		winIds = getWinIds();
	} else {
		winIds.append(winId);
	}

	for (long id : winIds) {
		NSView *nativeView = reinterpret_cast<NSView *>(id);
		NSWindow *nativeWindow = [nativeView window];
		initNativeConfig(nativeWindow);
	}

	return;
}

QMenu *
CocoaHelper::makeEditMenu(QMenuBar *menubar, QObject *parent)
{
	QMenu *menu = menubar->addMenu(QObject::tr("&Edit"));

	QAction *undoAct = new QAction(QObject::tr("&Undo"), parent);
	undoAct->setShortcuts(QKeySequence::Undo);

	QAction *redoAct = new QAction(QObject::tr("&Redo"), parent);
	redoAct->setShortcuts(QKeySequence::Redo);

	QAction *cutAct = new QAction(QObject::tr("Cu&t"), parent);
	cutAct->setShortcuts(QKeySequence::Cut);
	cutAct->setStatusTip(QObject::tr("Cut the current selection's"
	    " contents to the clipboard"));

	QAction *copyAct = new QAction(QObject::tr("&Copy"), parent);
	copyAct->setShortcuts(QKeySequence::Copy);
	copyAct->setStatusTip(QObject::tr("Copy the current selection's"
	    " contents to the clipboard"));

	QAction *pasteAct = new QAction(QObject::tr("&Paste"), parent);
	pasteAct->setShortcuts(QKeySequence::Paste);
	pasteAct->setStatusTip(QObject::tr("Paste the clipboard's"
	    " contents into the current selection"));

	menu->addAction(undoAct);
	menu->addAction(redoAct);
	menu->addSeparator();
	menu->addAction(cutAct);
	menu->addAction(copyAct);
	menu->addAction(pasteAct);
	return menu;
}

void
CocoaHelper::setDarkTitlebar(bool setting)
{
	darkTitlebarSetting = setting;
	return;
}

bool
CocoaHelper::getDarkTitlebar()
{
	return darkTitlebarSetting;
}

static void
setTitlebarColour(NSWindow *nw)
{
	if (darkTitlebarSetting) {
		[nw setAppearance:
		    [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
	} else {
		[nw setAppearance:
		    [NSAppearance appearanceNamed:NSAppearanceNameAqua]];
		[nw setBackgroundColor: [NSColor blackColor]];
	}

	return;
}

static void
initNativeConfig(NSWindow *nw)
{
	setTitlebarColour(nw);
	return;
}

static QVector<long>
getWinIds()
{
	QVector<long> winIds;
	QWindowList windows = QGuiApplication::allWindows();
	if (windows.empty())
		/* There is no window, abort */
		return winIds;

	for (QWindow *qwin : windows)
		winIds.append(qwin->winId());

	return winIds;
}
