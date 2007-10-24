/****************************************************************************
**
** Copyright (C) Qxt Foundation. Some rights reserved.
**
** This file is part of the QxtDesigner module of the Qt eXTension library
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
#include "qxtdesignerplugins.h"
#include "qxtcheckcomboboxplugin.h"
#include "qxtgroupboxplugin.h"
#include "qxtlabelplugin.h"
#include "qxtlistwidgetplugin.h"
#include "qxtprogresslabelplugin.h"
#include "qxtpushbuttonplugin.h"
#include "qxtspansliderplugin.h"
#include "qxtstarsplugin.h"
#include "qxtstringspinboxplugin.h"
#include "qxttablewidgetplugin.h"
#include "qxttreewidgetplugin.h"
#include <QtPlugin>

QList<QDesignerCustomWidgetInterface*> QxtDesignerPlugins::customWidgets() const
{
    QList<QDesignerCustomWidgetInterface*> plugins;
    plugins << new QxtLabelPlugin()
    << new QxtPushButtonPlugin()
    << new QxtCheckComboBoxPlugin()
    << new QxtSpanSliderPlugin()
    << new QxtStringSpinBoxPlugin()
    << new QxtGroupBoxPlugin()
    << new QxtListWidgetPlugin()
    << new QxtTreeWidgetPlugin()
    << new QxtTableWidgetPlugin()
    << new QxtStarsPlugin()
    << new QxtProgressLabelPlugin();
    return plugins;
}

Q_EXPORT_PLUGIN(QxtDesignerPlugins)
